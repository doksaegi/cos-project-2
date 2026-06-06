#include "process_manager.h"
#include "opcode.h"
#include "byte_op.h"
#include "setting.h"
#include <cstring>
#include <iostream>
#include <ctime>
#include <cstdint>
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
  int tmp, power_sum, power_avg, temp_sum, temp_avg;
  int month;
  double temp_raw;
  time_t ts;
  struct tm *tm;

  tdata = ds->getTemperatureData();
  hdata = ds->getHumidityData();
  num   = ds->getNumHouseData();

  // F0: temp_avg 구하기
  temp_avg = tdata->getValue();



  // F1
  ts    = ds->getTimestamp();
  tm    = localtime(&ts);
  month = tm->tm_mon + 1;

    // F2: power_avg (하우스 평균)
  power_sum = 0;
  for (int i = 0; i < num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    tmp = (int)pdata->getValue();
    power_sum += tmp;
  }
  power_avg = (num > 0) ? power_sum / num : 0;

  // 직렬화 (총 6 bytes)
  memset(ret, 0, BUFLEN);
  *dlen = 0;
  p = ret;


  VAR_TO_MEM_1BYTE_BIG_ENDIAN(temp_avg, p);        *dlen += 1;  // F0 
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(month, p);             *dlen += 1;  // F1
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(power_avg, p);        *dlen += 2;  // F2
  return ret;
}

//

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
