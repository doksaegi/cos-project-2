import socket
import requests
import threading
import argparse
import logging
import json
import sys

OPCODE_DATA = 1
OPCODE_WAIT = 2
OPCODE_DONE = 3
OPCODE_QUIT = 4


class Server:
    def __init__(self, name, algorithm, dimension, index, port, caddr, cport, ntrain, ntest):
        logging.info("[*] Initializing the server module to receive data from the edge device")
        self.name = name
        self.algorithm = algorithm
        self.dimension = dimension
        self.index = index
        self.port = port
        self.caddr = caddr
        self.cport = cport
        self.ntrain = ntrain
        self.ntest = ntest
        self.base_url = f"http://{caddr}:{cport}"

        if self.connecter():
            self.listener()

    def connecter(self):
        # ai.py가 request.get_json() 결과를 다시 json.loads() 하므로
        # dict가 아니라 JSON 문자열을 json= 로 전달해야 함
        config = {
            "algorithm": self.algorithm,
            "dimension": self.dimension,
            "index": self.index
        }
        payload = json.dumps(config)

        try:
            r = requests.post(f"{self.base_url}/{self.name}", json=payload, timeout=10)
            r.raise_for_status()
            result = r.json()
        except Exception as e:
            logging.error(f"[-] Error connecting to AI module: {e}")
            return False

        if result.get("opcode") == "success":
            logging.info(f"[+] Model '{self.name}' created successfully.")
            return True

        logging.error(f"[-] Failed to create model: {result.get('reason', 'unknown reason')}")
        return False

    def listener(self):
        server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_sock.bind(("", self.port))
        server_sock.listen(5)
        logging.info(f"[*] Control Server is listening on port {self.port}...")

        try:
            while True:
                client_sock, addr = server_sock.accept()
                logging.info(f"[+] Edge device connected from {addr}")
                t = threading.Thread(target=self.handler, args=(client_sock,), daemon=True)
                t.start()
        except KeyboardInterrupt:
            pass
        finally:
            server_sock.close()

    # 지정한 바이트 수(size)만큼 정확히 수신
    def recv_exact(self, sock, size):
        data = b""
        while len(data) < size:
            chunk = sock.recv(size - len(data))
            if not chunk:
                return None
            data += chunk
        return data

    def send_instance(self, values, is_training):
        endpoint = "training" if is_training else "testing"
        url = f"{self.base_url}/{self.name}/{endpoint}"

        body = {"value": values}
        payload = json.dumps(body)

        try:
            # ai.py 우회: json 문자열을 json=로 전달
            r = requests.put(url, json=payload, timeout=10)
            r.raise_for_status()
            resp = r.json()
        except Exception as e:
            logging.error(f"[-] Failed to send instance to AI module: {e}")
            sys.exit(1)

        if "opcode" not in resp:
            logging.error("[-] Invalid response from AI module: no opcode")
            sys.exit(1)

        if resp.get("opcode") == "failure":
            logging.error(f"[-] AI module failure: {resp.get('reason', 'unknown')}")
            sys.exit(1)

    # [업데이트] payload 포맷(총 6B):
    # humid_avg(1), temp_min(1), temp_avgm(1), power_daily(2, Big Endian), month(1)
    # 이번 버전은 temp_min/temp_avgm 포함 전부 unsigned 해석(signed=False)
    def parse_data(self, payload, is_training):
        humid_avg = int.from_bytes(payload[0:1], byteorder="big", signed=False)
        temp_min  = int.from_bytes(payload[1:2], "big", signed=True)
        temp_avgm = int.from_bytes(payload[2:3], "big", signed=True)
        power_daily = int.from_bytes(payload[3:5], byteorder="big", signed=False)
        month = int.from_bytes(payload[5:6], byteorder="big", signed=False)

        # AI 모듈 입력 포맷: [humid_avg, temp_min, temp_avgm, power_daily, month]
        values = [humid_avg, temp_min, temp_avgm, power_daily, month]
        logging.info(f"[*] Parsed data: {values}")
        self.send_instance(values, is_training)

    def print_result(self, result):
        logging.info(f"=== Result of Prediction ({self.name}) ===")
        logging.info(f"   # of instances: {result.get('num')}")
        logging.info(f"   correct predictions: {result.get('correct')}")
        logging.info(f"   incorrect predictions: {result.get('incorrect')}")
        logging.info(f"   accuracy: {result.get('accuracy')}%")

    def handler(self, sock):
        # 세션별 로컬 카운터 사용(스레드 간 self 공유값 오염 방지)
        ntrain = self.ntrain
        ntest = self.ntest

        try:
            # 1) Training phase
            while ntrain > 0:
                op = self.recv_exact(sock, 1)
                if op is None:
                    logging.error("[*] Connection closed during training phase.")
                    return

                opcode = int.from_bytes(op, byteorder="big")
                if opcode != OPCODE_DATA:
                    logging.error(f"[*] Invalid opcode in training phase: {opcode}")
                    return

                # 6바이트 payload 수신
                payload = self.recv_exact(sock, 6)
                if payload is None:
                    logging.error("[*] Failed to receive training payload.")
                    return

                self.parse_data(payload, True)
                ntrain -= 1

                if ntrain > 0:
                    sock.send(bytes([OPCODE_DONE]))
                else:
                    sock.send(bytes([OPCODE_WAIT]))

            # 2) Trigger model training
            try:
                tr = requests.post(f"{self.base_url}/{self.name}/training", timeout=30)
                tr.raise_for_status()
                trj = tr.json()
                if trj.get("opcode") == "failure":
                    logging.error(f"[-] Model training failed: {trj.get('reason', 'unknown')}")
                    return
            except Exception as e:
                logging.error(f"[-] Error while triggering model training: {e}")
                return

            # WAIT 해제 후 testing 시작 신호
            sock.send(bytes([OPCODE_DONE]))

            # 3) Testing phase
            while ntest > 0:
                op = self.recv_exact(sock, 1)
                if op is None:
                    logging.error("[*] Connection closed during testing phase.")
                    return

                opcode = int.from_bytes(op, byteorder="big")
                if opcode != OPCODE_DATA:
                    logging.error(f"[*] Invalid opcode in testing phase: {opcode}")
                    return

                # 6바이트 payload 수신
                payload = self.recv_exact(sock, 6)
                if payload is None:
                    logging.error("[*] Failed to receive testing payload.")
                    return

                self.parse_data(payload, False)
                ntest -= 1

                if ntest > 0:
                    sock.send(bytes([OPCODE_DONE]))
                else:
                    sock.send(bytes([OPCODE_QUIT]))

            # 4) Final result
            try:
                rr = requests.get(f"{self.base_url}/{self.name}/result", timeout=10)
                rr.raise_for_status()
                result = rr.json()
            except Exception as e:
                logging.error(f"[-] Error getting result: {e}")
                return

            if "opcode" not in result:
                logging.error("[-] Invalid result response: no opcode")
                return

            if result["opcode"] == "failure":
                logging.error(f"[-] Failed to get result: {result.get('reason', 'unknown')}")
                return

            if result["opcode"] == "success":
                self.print_result(result)
            else:
                logging.error(f"[-] Unknown result opcode: {result['opcode']}")

        finally:
            sock.close()


def command_line_args():
    p = argparse.ArgumentParser()
    p.add_argument("-a", "--algorithm", type=str, required=True)
    p.add_argument("-d", "--dimension", type=int, default=1)
    p.add_argument("-b", "--caddr", type=str, required=True)
    p.add_argument("-c", "--cport", type=int, required=True)
    p.add_argument("-p", "--lport", type=int, required=True)
    p.add_argument("-n", "--name", type=str, default="model")
    p.add_argument("-x", "--ntrain", type=int, default=10)
    p.add_argument("-y", "--ntest", type=int, default=10)
    p.add_argument("-z", "--index", type=int, default=0)
    p.add_argument("-l", "--log", type=str, default="INFO")
    return p.parse_args()


def main():
    args = command_line_args()
    logging.basicConfig(level=args.log, format="%(message)s")

    if args.ntrain <= 0 or args.ntest <= 0:
        logging.error("Number of instances for training/testing should be greater than 0")
        sys.exit(1)

    Server(
        args.name, args.algorithm, args.dimension, args.index,
        args.lport, args.caddr, args.cport, args.ntrain, args.ntest
    )


if __name__ == "__main__":
    main()
