import socket
import threading
import argparse
import logging

OPCODE_REPLY = 2

def protocol_execution(sock):
    # 1. Alice -> Bob: length of the name (4 bytes) || name (length bytes)
    # Get the length information (4 bytes)
    buf = sock.recv(4)  # 4 bytes를 받음
    length = int.from_bytes(buf, "big") # Big Endian으로 인코딩된 정보를 int로 변환
    logging.info("[*] Length received: {}".format(length)) # 로그로 받은 길이 정보 출력

    # Get the name (Alice)
    buf = sock.recv(length) # length 바이트만큼 이름 정보를 받음
    logging.info("[*] Name received: {}".format(buf.decode())) # 로그로 받은 이름 정보 출력 (바이트로 받은 정보를 문자열로 디코딩하여 출력)

    # 2. Bob -> Alice: length of the name (4 bytes) || name (length bytes)
    # Send the length information (4 bytes)
    name = "Bob" # 이름: Bob, 길이: 3
    length = len(name)
    logging.info("[*] Length to be sent: {}".format(length)) # 로그로 보낼 길이 정보 출력
    sock.send(int.to_bytes(length, 4, "big")) # 길이 정보를 4바이트로 인코딩하여 전송 (Big Endian)

    # Send the name (Bob)
    logging.info("[*] Name to be sent: {}".format(name)) # 로그로 보낼 이름 정보 출력
    sock.send(name.encode()) # 이름 정보를 바이트로 인코딩하여 전송 (문자열을 바이트로 변환하여 전송)

    # Implement following the instructions below
    # 3. Alice -> Bob: opcode (4 bytes) || arg1 (4 bytes) || arg2 (4 bytes)
    # The opcode should be 1
    buf = sock.recv(12) # 12바이트를 받음

    # The values are encoded in the big-endian style and should be translated into the little-endian style (because my machine follows the little-endian style)
    opcode = int.from_bytes(buf[0:4], "little") # 0~3번째 바이트는 opcode 정보, Big Endian으로 인코딩된 정보를 int로 변환
    arg1 = int.from_bytes(buf[4:8], "little") # 4~7번째 바이트는 arg1 정보, Big Endian으로 인코딩된 정보를 int로 변환
    arg2 = int.from_bytes(buf[8:12], "little") # 8~11번째 바이트는 arg2 정보, Big Endian으로 인코딩된 정보를 int로 변환

    logging.info("[*] Opcode: {}".format(opcode)) # 로그로 받은 opcode 정보 출력
    logging.info("[*] Arg1: {}".format(arg1)) # 로그로 받은 arg1 정보 출력
    logging.info("[*] Arg2: {}".format(arg2)) # 로그로 받은 arg2 정보 출력

    # 4. Bob -> Alice: opcode (4 bytes) || result (4 bytes)
    # The opcode should be 2
    result = arg1 + arg2 # arg1과 arg2의 합을 계산하여 result에 저장
    logging.info("[*] Result: {}".format(result)) # 로그로 계산된 result 정보 출력
    opcode = 2 # opcode는 2로 설정
    sock.send(int.to_bytes(OPCODE_REPLY, 4, "big")) # opcode 정보를 4바이트로 인코딩하여 전송 (Big Endian)
    sock.send(int.to_bytes(result, 4, "big")) # result 정보를 4바이트로 인코딩하여 전송 (Big Endian)

    sock.close() # 소켓을 닫음

def run(addr, port):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((addr, port))

    server.listen(2)
    logging.info("[*] Server is Listening on {}:{}".format(addr, port))

    while True:
        client, info = server.accept()

        logging.info("[*] Server accept the connection from {}:{}".format(info[0], info[1]))

        client_handle = threading.Thread(target=protocol_execution, args=(client,))
        client_handle.start()

def command_line_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--addr", metavar="<server's IP address>", help="Server's IP address", type=str, default="0.0.0.0")
    parser.add_argument("-p", "--port", metavar="<server's open port>", help="Server's port", type=int, required=True)
    parser.add_argument("-l", "--log", metavar="<log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)>", help="Log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)", type=str, default="INFO")
    args = parser.parse_args()
    return args

def main():
    args = command_line_args()
    log_level = args.log
    logging.basicConfig(level=log_level)

    run(args.addr, args.port)

if __name__ == "__main__":
    main()
