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

        # AI 모듈에 넘겨줄 설정값들
        config = {
            "algorithm": self.algorithm, #알고리즘 이름
            "dimension": self.dimension, #입력 데이터 차원 크기
            "index": self.index #모델 고유번호
        }
        payload = json.dumps(config) # json.dumps()로 dict -> JSON 문자열 변환

        try:
            r = requests.post(f"{self.base_url}/{self.name}", json=payload, timeout=10) #json=payload 로 설정값을 담고, 10초(timeout) 안에 답이 없으면 연결을 끊음 (HTTP 통신 (REST API [POST]))
            r.raise_for_status() # 서버가 에러 코드(404, 500) 반환 했는지 확인 (에러일 시 except로 넘어감)
            result = r.json() # JSON 응답을 dict로 변환하고 result 변수에 저장
        except Exception as e:
            logging.error(f"[-] Error connecting to AI module: {e}") # 연결 실패 에러 메시지 출력 + return False (이때의 에러 처리는 통신/시스템 에러에 대한 처리)
            return False

        if result.get("opcode") == "success":
            logging.info(f"[+] Model '{self.name}' created successfully.") # 모델 생성 성공 시 메시지 출력 + return True
            return True

        logging.error(f"[-] Failed to create model: {result.get('reason', 'unknown reason')}") # opcode가 success가 아닐 경우 AI 모듈이 반환한 reason 메시지 출력 + return False (이때의 에러 처리는 모델 생성 실패에 대한 처리)
        return False

    def listener(self):
        server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # TCP 소켓 생성 (AF_INET: IPv4, SOCK_STREAM: TCP)
        server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # 소켓 옵션 설정: SO_REUSEADDR는 서버가 종료된 후에도 일정 시간 동안 해당 포트를 재사용할 수 있도록 허용
        server_sock.bind(("", self.port)) # 포트에 바인딩, ""는 모든 인터페이스에서 연결 허용
        server_sock.listen(5) # 최대 5개의 대기 연결 허용
        logging.info(f"[*] Control Server is listening on port {self.port}...") # 서버가 포트에서 연결을 기다리고 있음을 알리는 로그 출력

        try:
            while True: # 무한 루프
                client_sock, addr = server_sock.accept() # 클라이언트 연결 수락, client_sock은 클라이언트와 통신할 소켓 객체 생성, addr은 클라이언트의 주소 정보
                logging.info(f"[+] Edge device connected from {addr}") # 클라이언트가 연결되었음을 알리는 로그 출력
                t = threading.Thread(target=self.handler, args=(client_sock,), daemon=True)  # 클라이언트 연결마다 handler 함수를 실행하는 새로운 스레드 생성, daemon=True로 설정하여 메인 서버가 종료될 때 스레드도 함께 종료되도록 함
                t.start() # 스레드 시작하여 클라이언트 요청 처리 (handler 함수에서 클라이언트와의 통신을 담당)
        except KeyboardInterrupt:
            pass # Ctrl+C로 서버 종료 시 루프 탈출
        finally:
            server_sock.close() # 서버 소켓 닫기

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
        ntrain = self.ntrain # self.ntrain 으로 ntrain 초기화
        ntest = self.ntest # self.ntest 으로 ntest 초기화

        try:
            # 1) Training phase
            while ntrain > 0: # 받아야 할 학습 데이터 횟수 (ntrain)가 0보다 큰 동안 반복
                op = self.recv_exact(sock, 1) # edge가 보낸 1바이트 opcode 수신
                if op is None:
                    logging.error("[*] Connection closed during training phase.") # edge가 연결을 끊었을 때 에러 메시지 출력 + 루프 종료
                    return

                opcode = int.from_bytes(op, byteorder="big") # 수신한 opcode를 정수로 변환 (Big Endian)
                if opcode != OPCODE_DATA:
                    logging.error(f"[*] Invalid opcode in training phase: {opcode}") # opcode가 기대한 OPCODE_DATA가 아닐 때 에러 메시지 출력 + 루프 종료
                    return

                # 6바이트 payload 수신
                payload = self.recv_exact(sock, 6) # edge가 보낸 정확히 6바이트인 payload 수신
                if payload is None:
                    logging.error("[*] Failed to receive training payload.") # payload 수신 실패 시 에러 메시지 출력 + 루프 종료
                    return

                self.parse_data(payload, True) # 수신한 payload를 parse_data 함수로 전달하여 AI 모듈에 학습 데이터로 전송 (is_training=True)
                ntrain -= 1 # 학습 데이터 1회 수신 후 ntrain 감소

                # 다음 학습 데이터 수신 신호: 남았으면 OPCODE_DONE, 더이상 없으면 OPCODE_WAIT
                if ntrain > 0:
                    sock.send(bytes([OPCODE_DONE])) 
                else:
                    sock.send(bytes([OPCODE_WAIT]))

            # 2) Trigger model training
            try:
                tr = requests.post(f"{self.base_url}/{self.name}/training", timeout=30) # AI 모듈에 모델 학습 시작 신호를 보내는 POST 요청, 30초(timeout) 안에 답이 없으면 연결을 끊음 (HTTP 통신 (REST API [POST]))
                tr.raise_for_status() # 서버가 에러 코드(404, 500 등) 반환 했는지 확인 (에러일 시 except로 넘어감)
                trj = tr.json() # JSON 응답을 dict로 변환하고 trj 변수에 저장
                if trj.get("opcode") == "failure":
                    logging.error(f"[-] Model training failed: {trj.get('reason', 'unknown')}") # 모델 학습 실패 시 AI 모듈이 반환한 reason 메시지 출력 + 루프 종료 (이때의 에러 처리는 모델 학습 실패에 대한 처리)
                    return
            except Exception as e:
                logging.error(f"[-] Error while triggering model training: {e}") # 모델 학습 시작 신호 전송 실패 시 에러 메시지 출력 + 루프 종료 (이때의 에러 처리는 통신/시스템 에러에 대한 처리)
                return

            # WAIT 해제 후 testing 시작 신호
            sock.send(bytes([OPCODE_DONE]))

            # 3) Testing phase
            while ntest > 0: # 받아야 할 테스트 데이터 횟수 (ntest)가 0보다 큰 동안 반복
                op = self.recv_exact(sock, 1) # edge가 보낸 1바이트 opcode 수신
                if op is None:
                    logging.error("[*] Connection closed during testing phase.") # edge가 연결을 끊었을 때 에러 메시지 출력 + 루프 종료
                    return

                opcode = int.from_bytes(op, byteorder="big") # 수신한 opcode를 정수로 변환 (Big Endian)
                if opcode != OPCODE_DATA:
                    logging.error(f"[*] Invalid opcode in testing phase: {opcode}") # opcode가 기대한 OPCODE_DATA가 아닐 때 에러 메시지 출력 + 루프 종료
                    return

                # 6바이트 payload 수신
                payload = self.recv_exact(sock, 6) # edge가 보낸 정확히 6바이트인 payload 수신
                if payload is None:
                    logging.error("[*] Failed to receive testing payload.") # payload 수신 실패 시 에러 메시지 출력 + 루프 종료
                    return

                self.parse_data(payload, False) # 수신한 payload를 parse_data 함수로 전달하여 AI 모듈에 테스트 데이터로 전송 (is_training=False)
                ntest -= 1 # 테스트 데이터 1회 수신 후 ntest 감소

                # 다음 테스트 데이터 수신 신호: 남았으면 OPCODE_DONE, 더이상 없으면 OPCODE_QUIT
                if ntest > 0:
                    sock.send(bytes([OPCODE_DONE]))
                else:
                    sock.send(bytes([OPCODE_QUIT]))

            # 4) Final result
            try:
                rr = requests.get(f"{self.base_url}/{self.name}/result", timeout=10) # AI 모듈에 최종 결과 요청하는 GET 요청, 10초(timeout) 안에 답이 없으면 연결을 끊음 (HTTP 통신 (REST API [GET]))
                rr.raise_for_status() # 서버가 에러 코드(404, 500 등) 반환 했는지 확인 (에러일 시 except로 넘어감)
                result = rr.json() # JSON 응답을 dict로 변환하고 result 변수에 저장
            except Exception as e:
                logging.error(f"[-] Error getting result: {e}") # 최종 결과 요청 실패 시 에러 메시지 출력 + 루프 종료 (이때의 에러 처리는 통신/시스템 에러에 대한 처리)
                return

            if "opcode" not in result:
                logging.error("[-] Invalid result response: no opcode") # AI 모듈이 반환한 최종 결과에 opcode가 없을 때 에러 메시지 출력 + 루프 종료
                return

            if result["opcode"] == "failure":
                logging.error(f"[-] Failed to get result: {result.get('reason', 'unknown')}") # AI 모듈이 반환한 최종 결과의 opcode가 failure일 때 AI 모듈이 반환한 reason 메시지 출력 + 루프 종료 (이때의 에러 처리는 모델 평가 실패에 대한 처리)
                return

            if result["opcode"] == "success":
                self.print_result(result) # AI 모듈이 반환한 최종 결과의 opcode가 success일 때 print_result 함수로 결과 출력
            else:
                logging.error(f"[-] Unknown result opcode: {result['opcode']}") # AI 모듈이 반환한 최종 결과의 opcode가 success도 failure도 아닐 때 에러 메시지 출력 + 루프 종료

        finally:
            sock.close() # 클라이언트 소켓 닫기 (세션 종료)


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
