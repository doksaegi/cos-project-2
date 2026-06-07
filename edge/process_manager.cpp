
#include <string>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <iostream>
#include "process_manager.h"
#include "opcode.h"
#include "byte_op.h"
#include "setting.h"

using namespace std;

ProcessManager::ProcessManager()
{
  this->num = 0;
}

void ProcessManager::init()
{
}

uint8_t *ProcessManager::processData(DataSet *ds, int *dlen)
{
  
  uint8_t *ret, *p;
  // ret : 직렬화 결과를 쓸 버퍼 (힙 할당)
  // p   : 버퍼 내 현재 쓰기 위치를 가리키는 커서

  int num;
  // num : 하우스(온실) 수 — power 루프 상한

  HouseData  *house;
  // house : 루프 내 개별 하우스 포인터

  SensorData *tdata;
  // tdata : DataSet 레벨 기온 데이터.
  //         getTemperatureData()는 TemperatureData*를 반환하지만
  //         TemperatureData가 SensorData를 상속하므로 SensorData*로 수신 가능.

  PowerData  *pdata;
  // pdata : 개별 하우스 전력 데이터 포인터

  ret = (uint8_t *)malloc(BUFLEN);
  // 결과 버퍼를 힙에 BUFLEN 바이트 할당
  memset(ret, 0, BUFLEN);
  // 버퍼 전체를 0으로 초기화 (쓰레기값 방지)

  int temp_avg;
  // temp_avg : 일 평균 기온 (DataSet 레벨에서 이미 집계된 값)

  int power, power_sum, power_avg;
  // power      : 루프 내 개별 하우스 전력값
  // power_sum  : 전력 합산 누적
  // power_avg  : 하우스 평균 전력

  int month;
  // month : 해당 날짜의 월 (1~12)

  time_t     ts;
  // ts : Unix timestamp (time_t = 초 단위 정수)

  struct tm *tm;
  // tm : localtime()이 반환하는 날짜/시각 구조체 포인터

  num = ds->getNumHouseData();
  // DataSet에 등록된 하우스 수를 가져옴

  tdata = ds->getTemperatureData();
  // DataSet 레벨의 기온 데이터 획득.
  // 반환 타입은 TemperatureData* 이지만 SensorData*로 묵시적 업캐스팅.

  temp_avg = (int)tdata->getValue();
  // SensorData::getValue() 호출 → avg(일 평균 기온) 반환.
  // [상속] virtual dispatch: 런타임에 TemperatureData::getValue()가 실행됨.

  ts = ds->getTimestamp();
  // DataSet에 기록된 해당 날짜의 Unix timestamp를 가져옴

  tm = localtime(&ts);
  // timestamp(time_t)를 지역 시간 기준 struct tm으로 변환.

  month = tm->tm_mon + 1;
  // tm_mon은 0-based(0=1월 ~ 11=12월) → +1로 실제 월 번호로 변환

  power_sum = 0;
  // 전력 합산 초기화

  for (int i = 0; i < num; i++)
  // 하우스 수만큼 반복
  {
    house = ds->getHouseData(i);
    // i번째 HouseData 포인터 획득 (linked list를 인덱스로 순회)

    pdata = house->getPowerData();
    // 해당 하우스의 PowerData 포인터 획득

    power = (int)pdata->getValue();
    // PowerData::getValue()로 해당 하우스의 전력값(kWh)을 정수로 변환.

    power_sum += power;
    // 전력값 누적
  }

  power_avg = (num > 0) ? power_sum / num : 0;
  // 전체 하우스 전력 합을 하우스 수로 나눠 평균 산출.
  // num == 0 이면 0 나눗셈 방지를 위해 0 반환.

  *dlen = 0;
  // 패킷 길이 카운터 초기화

  p = ret;
  // 쓰기 커서를 버퍼 시작 위치로 설정

  VAR_TO_MEM_1BYTE_BIG_ENDIAN(temp_avg, p);   *dlen += 1;
  // [F0] 일 평균 기온을 1 byte big-endian으로 기록, 커서 +1

  VAR_TO_MEM_1BYTE_BIG_ENDIAN(month, p);      *dlen += 1;
  // [F1] 월 값을 1 byte로 기록, 커서 +1

  VAR_TO_MEM_2BYTES_BIG_ENDIAN(power_avg, p); *dlen += 2;
  // [F2] 하우스 평균 전력을 2 byte big-endian으로 기록, 커서 +2.
  //      power_avg가 255를 초과할 수 있어 2 bytes 사용.

  return ret;
  // 직렬화된 패킷(4 bytes) 반환.
}


/* #include "process_manager.h"
#include "opcode.h"
#include "byte_op.h"
#include "setting.h"
#include <cstring>
#include <iostream>
#include <ctime>
using namespace std;
 
ProcessManager::ProcessManager()
{
  this->num = 0;
}
 
void ProcessManager::init()
{
}
 
uint8_t *ProcessManager::processData(DataSet *ds, int *dlen)
{
  uint8_t *ret, *p;
  int num;
  HouseData *house;
  TemperatureData *tdata;
  HumidityData *hdata;
  PowerData *pdata;
  ret = (uint8_t *)malloc(BUFLEN);
  int tmp, power_sum, power_avg, temp_min, humid_min, month;
  time_t ts;
  struct tm *tm;
 
  tdata = ds->getTemperatureData();
  hdata = ds->getHumidityData();
  num   = ds->getNumHouseData();
 
  //temp_min
  temp_min = (int)tdata->getMin();
 
  // humid_min
  humid_min = (int)hdata->getMin();
 
  // power_avg: 하우스 평균
  power_sum = 0;
  for (int i = 0; i < num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    power_sum += (int)pdata->getValue();
  }
  power_avg = (num > 0) ? power_sum / num : 0;
 
  // month
  ts    = ds->getTimestamp();
  tm    = localtime(&ts);
  month = tm->tm_mon + 1;
 
  memset(ret, 0, BUFLEN);
  *dlen = 0;
  p = ret;
  
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(temp_min, p);   *dlen += 1;
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(humid_min, p);  *dlen += 1;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(power_avg, p); *dlen += 2;
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(month, p);      *dlen += 1;
 
  return ret;
} 
*/ 

/*#include "opcode.h"
#include "byte_op.h"
#include "setting.h"
#include <cstring>
#include <iostream>
#include <cmath>
#include <ctime>
using namespace std;
 
ProcessManager::ProcessManager()
{
  this->num = 0;
}
 
void ProcessManager::init()
{
}
 

uint8_t *ProcessManager::processData(DataSet *ds, int *dlen)
{
  uint8_t *ret, *p;
  int num;
  HouseData *house;
  TemperatureData *tdata;
  PowerData *pdata;
  ret = (uint8_t *)malloc(BUFLEN);
  int tmp, power_min, power_sum, power_avg,month;
  time_t ts;
  struct tm *tm;
 
  num   = ds->getNumHouseData();
 

 
  // power_min, power_avg
  power_min = 10000;
  power_sum = 0;
  for (int i = 0; i < num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    tmp   = (int)pdata->getValue();
    if (tmp < power_min) power_min = tmp;
    power_sum += tmp;
  }
  power_avg = (num > 0) ? power_sum / num : 0;
 
  // month
  ts    = ds->getTimestamp();
  tm    = localtime(&ts);
  month = tm->tm_mon + 1;
 
  memset(ret, 0, BUFLEN);
  *dlen = 0;
  p = ret;
 
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(power_min, p); *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(power_avg, p); *dlen += 2;
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(month, p);      *dlen += 1;
 
  return ret;
}
 */
