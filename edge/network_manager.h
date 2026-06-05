#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include <cstdint>
#include "setting.h"

// 네트워크 통신(소켓 연결/데이터 송수신) 담당 클래스
class NetworkManager {
  private:
    int sock;              // 서버와 연결된 소켓 파일 디스크립터
    const char *addr;      // 서버 IP 주소 문자열
    int port;              // 서버 포트 번호
    uint8_t wbuf[BUFLEN];  // 송신 버퍼(예비)
    uint8_t rbuf[BUFLEN];  // 수신 버퍼(예비)

  public:
    // 기본 생성자/매개변수 생성자
    NetworkManager();
    NetworkManager(const char *addr, int port);

    // 주소/포트 설정 및 조회
    void setAddress(const char *addr);
    const char *getAddress();

    void setPort(int port);
    int getPort();

    // 소켓 생성 및 서버 연결
    int init();

    // 센서 집계 데이터 전송
    // 입력 data 포맷(총 6B):
    // data[0]=humid_avg(1), data[1]=temp_min(1), data[2]=temp_avgm(1),
    // data[3~4]=power_daily(2), data[5]=month(1)
    int sendData(uint8_t *data, int dlen);

    // 서버 지시 opcode 수신 (DONE / QUIT 등)
    uint8_t receiveCommand();
};

#endif /* __NETWORK_MANAGER_H__ */
