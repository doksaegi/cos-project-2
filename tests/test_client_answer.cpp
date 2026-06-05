#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdint.h>

#include "../edge/byte_op.h"

#define BUFLEN        1024
#define OPCODE_SUM    1
#define OPCODE_REPLY  2

void protocol_execution(int sock);
void error_handling(const char *message);

void usage(const char *pname)
{
  printf(">> Usage: %s [options]\n", pname);
  printf("Options\n");
  printf("  -a, --addr       Server's address\n");
  printf("  -p, --port       Server's port\n");
  exit(0);
}

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
  char msg[] = "Hello, World!\n";
	char message[30] = {0, };
	int c, port, tmp, str_len;
  char *pname;
  uint8_t *addr;
  uint8_t eflag;

  pname = argv[0];
  addr = NULL;
  port = -1;
  eflag = 0;

  while (1)
  {
    int option_index = 0;
    static struct option long_options[] = {
      {"addr", required_argument, 0, 'a'},
      {"port", required_argument, 0, 'p'},
      {0, 0, 0, 0}
    };

    const char *opt = "a:p:0";

    c = getopt_long(argc, argv, opt, long_options, &option_index);

    if (c == -1)
      break;

    switch (c)
    {
      case 'a':
        tmp = strlen(optarg);
        addr = (uint8_t *)malloc(tmp);
        memcpy(addr, optarg, tmp);
        break;

      case 'p':
        port = atoi(optarg);
        break;

      default:
        usage(pname);
    }
  }

  if (!addr)
  {
    printf("[*] Please specify the server's address to connect\n");
    eflag = 1;
  }

  if (port < 0)
  {
    printf("[*] Please specify the server's port to connect\n");
    eflag = 1;
  }

  if (eflag)
  {
    usage(pname);
    exit(0);
  }

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr((const char *)addr);
	serv_addr.sin_port = htons(port);

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
  printf("[*] Connected to %s:%d\n", addr, port);
  
  protocol_execution(sock);

	close(sock);
	return 0;
}

void protocol_execution(int sock)
{
  char msg[] = "Alice"; // 보낼 메시지 (Alice로 초기화)
  char buf[BUFLEN]; // 데이터를 수신할 버퍼 공간 준비
  int tbs, sent, tbr, rcvd, offset;
  int len;

  // tbs: the number of bytes to send
  // tbr: the number of bytes to receive
  // offset: the offset of the message

  // 1. Alice -> Bob: length of the name (4 bytes) || name (length bytes)
  // Send the length information (4 bytes)
  len = strlen(msg); // 메시지의 길이를 len에 저장
  printf("[*] Length information to be sent: %d\n", len); // 메시지의 길이를 출력

  len = htonl(len); // 길이를 네트워크 바이트 순서로 변환하여 len에 저장 (Big Endian)
  tbs = 4; // 보낼 바이트 수를 4로 설정 (4 bytes)
  offset = 0; // offset를 0으로 초기화

  while (offset < tbs) // offset이 보낼 바이트 수보다 작은 동안 반복
  {
    sent = write(sock, &len + offset, tbs - offset); // 데이터의 현재 전송 위치(&len + offset)부터 남은 분량(tbs - offset)만큼의 내용을 소켓에 기록, write()는 실제로 전송된 바이트 수를 반환
    if (sent > 0)
      offset += sent; // sent가 0보다 크면 보낸 바이트 수만큼 offset을 증가시킴
  }

  // Send the name (Alice)
  tbs = ntohl(len); // 네트워크 바이트 순서로 변환된 길이를 호스트 바이트 순서로 다시 변환하여 tbs에 저장
  offset = 0; // offset를 0으로 초기화

  printf("[*] Name to be sent: %s\n", msg); // 보낼 메시지를 출력
  while (offset < tbs) // offset이 보낼 바이트 수보다 작은 동안 반복
  {
    sent = write(sock, msg + offset, tbs - offset); // 데이터의 현재 전송 위치(msg + offset)부터 남은 분량(tbs - offset)만큼의 내용을 소켓에 기록
    if (sent > 0)
      offset += sent; // sent가 0보다 크면 보낸 바이트 수만큼 offset을 증가시킴
  }

  // 2. Bob -> Alice: length of the name (4 bytes) || name (length bytes)
  // Receive the length information (4 bytes)
  tbr = 4; // 받을 바이트 수를 4로 설정 (4 bytes)
  offset = 0; // offset를 0으로 초기화

  while (offset < tbr) // offset이 받을 바이트 수보다 작은 동안 반복
  {
	  rcvd = read(sock, &len + offset, tbr - offset); // 데이터의 현재 수신 위치(&len + offset)에서 부터 남은 분량(tbr - offset)만큼의 내용을 소켓에서 읽고 저장, read()는 실제로 읽은 바이트 수를 반환
    if (rcvd > 0)
      offset += rcvd; // rcvd가 0보다 크면 받은 바이트 수만큼 offset을 증가시킴
  }
  len = ntohl(len); // 네트워크 바이트 순서로 변환된 길이를 호스트 바이트 순서로 다시 변환하여 len에 저장
  printf("[*] Length received: %d\n", len); // 받은 메시지의 길이를 출력

  // Receive the name (Bob)
  tbr = len; // 받을 바이트 수를 받은 메시지의 길이로 설정
  offset = 0; // offset를 0으로 초기화

  while (offset < tbr) // offset이 받을 바이트 수보다 작은 동안 반복
  {
    rcvd = read(sock, buf + offset, tbr - offset); // 데이터의 현재 수신 위치(buf + offset)에서 부터 남은 분량(tbr - offset)만큼의 내용을 소켓에서 읽고 저장
    if (rcvd > 0)
      offset += rcvd; // rcvd가 0보다 크면 받은 바이트 수만큼 offset을 증가시킴
  }

	printf("[*] Name received: %s \n", buf); // 받은 메시지를 출력

  // Implement following the instructions below
  // Let's assume there are two opcodes:
  //     1: summation request for the two arguments
  //     2: reply with the result
  // 3. Alice -> Bob: opcode (4 bytes) || arg1 (4 bytes) || arg2 (4 bytes)
  // The opcode should be 1

  char *p; // 데이터를 담을 버퍼의 현재 위치를 가리키는 포인터
  int i, arg1, arg2;

  memset(buf, 0, BUFLEN); // 버퍼 공간 초기화
  p = buf;
  arg1 = 2;
  arg2 = 5;

  VAR_TO_MEM_1BYTE_BIG_ENDIAN(OPCODE_SUM, p); // opcode를 Big Endian 포맷으로 인코딩하여 버퍼에 저장, p는 자동으로 다음 위치로 이동
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(arg1, p); // arg1을 Big Endian 포맷으로 인코딩하여 버퍼에 저장, p는 자동으로 다음 위치로 이동
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(arg2, p); // arg2를 Big Endian 포맷으로 인코딩하여 버퍼에 저장, p는 자동으로 다음 위치로 이동
  tbs = p - buf; // 버퍼의 시작 주소(buf)에서 현재 위치(p)까지의 거리를 계산하여 보낼 바이트 수(tbs)에 저장
  offset = 0; // offset를 0으로 초기화

  printf("[*] # of bytes to be sent: %d\n", tbs); // 보낼 바이트 수를 출력
  printf("[*] The following bytes will be sent\n");
  for (i=0; i<tbs; i++)
    printf ("%02x ", buf[i]); // 보낼 데이터를 16진수 형식으로 출력
  printf("\n");

  while (offset < tbs) // offset이 보낼 바이트 수보다 작은 동안 반복
  {
    sent = write(sock, buf + offset, tbs - offset); // 데이터의 현재 전송 위치(buf + offset)부터 남은 분량(tbs - offset)만큼의 내용을 소켓에 기록
    if (sent > 0)
      offset += sent; // sent가 0보다 크면 보낸 바이트 수만큼 offset을 증가시킴
  }

  // 4. Bob -> Alice: opcode (4 bytes) || result (4 bytes)
  // The opcode should be 2

  int opcode, result;

  tbr = 8; offset = 0; // 받을 바이트 수: 8, offset: 0
  memset(buf, 0, BUFLEN); // 버퍼 공간 초기화

  printf("[*] # of bytes to be received: %d\n", tbr);
  while (offset < tbr) // offset이 받을 바이트 수보다 작은 동안 반복
  {
    rcvd = read(sock, buf + offset, tbr - offset); 
    // tbs - offset 였는데 tbr - offset으로 수정 (오타인듯 합니다)   
    // 데이터의 현재 수신 위치(buf + offset)에서 부터 남은 분량(tbr - offset)만큼의 내용을 소켓에서 읽고 저장

    if (rcvd > 0)
      offset += rcvd; // rcvd가 0보다 크면 받은 바이트 수만큼 offset을 증가시킴
  }

  printf("[*] The following bytes is received\n");
  for (i=0; i<tbr; i++)
    printf("%02x ", buf[i]); // 받은 데이터를 16진수 형식으로 출력
  printf("\n");

  p = buf; // 버퍼의 시작 주소를 p에 저장
  MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, opcode); // 버퍼에서 4바이트를 Big Endian 포맷으로 읽어서 opcode에 저장, p는 자동으로 다음 위치로 이동
  printf("[*] Opcode: %d\n", opcode); // 받은 opcode를 출력
  MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, result); // 버퍼에서 4바이트를 Big Endian 포맷으로 읽어서 result에 저장, p는 자동으로 다음 위치로 이동
  printf("[*] Result: %d\n", result); // 받은 결과를 출력
}

void error_handling(const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
