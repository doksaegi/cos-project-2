#include "network_manager.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>

#include "opcode.h"
#include "byte_op.h" // Big Endian 변환 매크로(VAR_TO_MEM_XBYTES)를 사용하기 위해 헤더 포함

using namespace std;

// 기본 생성자: 소켓 및 주소 정보 초기화
NetworkManager::NetworkManager() 
{
  this->sock = -1;
  this->addr = NULL;
  this->port = -1;
}

// 매개변수 생성자: 목적지 IP 주소와 포트 번호 설정
NetworkManager::NetworkManager(const char *addr, int port)
{
  this->sock = -1;
  this->addr = addr;
  this->port = port;
}

// IP 주소 설정 함수
void NetworkManager::setAddress(const char *addr)
{
  this->addr = addr;
}

// IP 주소 반환 함수
const char *NetworkManager::getAddress()
{
  return this->addr;
}

// 포트 번호 설정 함수
void NetworkManager::setPort(int port)
{
  this->port = port;
}

// 포트 번호 반환 함수
int NetworkManager::getPort()
{
  return this->port;
}

// 소켓을 생성하고 컨트롤 서버에 연결(connect)하는 초기화 함수
int NetworkManager::init()
{
  struct sockaddr_in serv_addr;

  // IPv4, TCP 프로토콜 소켓 생성
  this->sock = socket(PF_INET, SOCK_STREAM, 0);
  if (this->sock == FAILURE)
  {
    cout << "[*] Error: socket() error" << endl;
    cout << "[*] Please try again" << endl;
    exit(1);
  }

  // 서버 주소 구조체 초기화 및 설정
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(this->addr); // 문자열 형식을 네트워크 바이트 순서의 IP로 변환
  serv_addr.sin_port = htons(this->port);            // 호스트 바이트 순서의 포트를 네트워크 바이트 순서로 변환

  // 서버에 연결 요청
  if (connect(this->sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == FAILURE)
  {
    cout << "[*] Error: connect() error" << endl;
    cout << "[*] Please try again" << endl;
    exit(1);
  }
  
  cout << "[*] Connected to " << this->addr << ":" << this->port << endl;

  return sock;
}

// [업데이트] 에지 -> 서버: 4바이트 센서 집계 데이터 전송 함수
// 송신 포맷(총 4B):
// temp_avg(1), month(1), power_avg(2, Big Endian)
int NetworkManager::sendData(uint8_t *data, int dlen) // NetworkManager의 sendData 함수를 외부에서 구현
{
  // 데이터 집계 담당자가 배열 형태로 넘겨준 데이터 구조
  // data[0]: temp_avg (1B)
  // data[1]: month (1B)
  // data[2~3]: power_avg (2B, high->low)
  if (dlen < 4)
  {
    cout << "[*] Error: invalid data length. need 4 bytes." << endl; // 4 바이트 미만이면 오류 처리
    return -1;
  }

  uint8_t temp_avg   = data[0];
  uint8_t month      = data[1];
  uint16_t power_avg = (static_cast<uint16_t>(data[2]) << 8) | data[3]; // 2바이트를 Big Endian으로 합쳐서 uint16_t로 표현

  // 프로토콜 규격에 맞춘 전송용 버퍼 공간 생성 (총 4바이트)
  uint8_t payload[4];
  uint8_t *p = payload; // byte_op.h 매크로 내부에서 참조하고 자동 이동시킬 포인터 변수

  // byte_op.h 매크로를 사용하여 Big Endian 포맷으로 인코딩
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(temp_avg, p);    // 1 Byte
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(month, p);       // 1 Byte
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(power_avg, p);  // 2 Bytes

  int sock = this->sock; // 소켓
  unsigned char opcode = OPCODE_DATA; // 데이터 스트림 시작 알림용 1바이트 연산 코드
  int sent = 0; // write()로 실제 전송된 바이트 수를 저장하는 변수
  int offset = 0; // 전송된 바이트 수를 누적하여 전체 전송 완료 여부를 판단하는 오프셋 변수

  // [단계 1] 1바이트 OPCODE_DATA 전송 (부분 송신 방지 루프)
  int tbs = 1; // 전송해야 할 총 바이트 수(목표치)
  while (offset < tbs) // offset이 목표치에 도달할 때까지 반복
  {
    sent = write(sock, &opcode + offset, tbs - offset); // 데이터의 현재 전송 위치(&opcode + offset)부터 남은 분량(tbs - offset)만큼의 내용을 에지 소켓에 기록, write()는 실제로 전송된 바이트 수를 반환
    if (sent <= 0) // sent가 0 이하이면 오류 처리
    {
      cout << "[*] Error: Failed to send opcode." << endl; 
      return -1;
    }
    offset += sent;
  }

  // [단계 2] 실제 센서 payload 4바이트 전송
  tbs = 4;
  offset = 0;
  while (offset < tbs) // offset이 목표치에 도달할 때까지 반복
  {
    sent = write(sock, payload + offset, tbs - offset);  // payload의 현재 전송 위치(payload + offset)부터 남은 분량(tbs - offset)만큼의 내용을 에지 소켓에 기록
    if (sent <= 0) // sent가 0 이하이면 오류 처리
    {
      cout << "[*] Error: Failed to send data payload." << endl;
      return -1;
    }
    offset += sent; // offset에 보내진 sent 만큼 누적
  }

  return 0; // 정상 전송 완료
}

// 서버 -> 에지: 서버 측 다음 지시 명령어(Opcode) 수신 함수
uint8_t NetworkManager::receiveCommand() // NetworkManager의 receiveCommand 함수를 외부에서 구현
{
  int sock = this->sock; // 소켓
  uint8_t opcode = OPCODE_WAIT; // 초기 상태를 WAIT로 둔다
  int nread = 0;

  // opcode가 WAIT인 동안 반복
  while (opcode == OPCODE_WAIT)
  {
    nread = read(sock, &opcode, 1); // 서버로부터 1바이트 opcode 읽고 상태를 업데이트
    
    if (nread < 0)
    {
      cout << "[*] Error: read() error inside receiveCommand." << endl; // nread가 음수이면 읽기 오류 처리
      return OPCODE_QUIT;
    }
    else if (nread == 0)
    {
      cout << "[*] Server disconnected." << endl; // nread가 0이면 서버가 연결을 종료한 것으로 간주
      return OPCODE_QUIT;
    }
  }

  // 예상 외 opcode가 오면 경고 출력
  if (opcode != OPCODE_DONE && opcode != OPCODE_QUIT)
  {
    cout << "[*] Warning: Received unexpected opcode from server: " << (int)opcode << endl;
  }

  return opcode; // DONE 또는 QUIT 중 하나를 반환
}