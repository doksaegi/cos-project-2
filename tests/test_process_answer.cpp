#include "../edge/setting.h"
#include "../edge/edge.h"

#include <iostream>
#include <ctime>

#include "../edge/byte_op.h"

#define BUFLEN 1024

using namespace std;

int main(int argc, char *argv[])
{
  // 변수 선언
  DataReceiver *dr;
  DataSet *ds;
  HouseData *house;
  TemperatureData *tdata;
  HumidityData *hdata;
  PowerData *pdata;
  int num, tmp;
  time_t curr, ts;
  double max_temp, avg_temp, min_temp;
  double max_humid, avg_humid, min_humid;
  double power, sum_power, avg_power, max_power, min_power;
  unsigned char buf[BUFLEN];
  unsigned char *p;

  curr = 1609459200; // 타임스템프 기준 시간 설정 (2021-01-01 00:00:00)
  dr = new DataReceiver(); // 센서 데이터 수신 객체 생성 (Dynamic Allocation)
  ds = dr->getDataSet(curr); // curr에 해당하는 센서 데이터셋을 받아옴 (dataset.h)
  
  // 1. Write a statement to get the timestamp value to 'ts' and print out the value (please refer to dataset.h)
  ts = ds->getTimestamp(); // ts에 타임스템프 값 저장
  cout << "timestamp: " << ts << endl; // 타임스템프 값 출력

  // 2. Write a statement to get the number of house data that contains the private information and the power value to 'num' (dataset.h)
  num = ds->getNumHouseData(); // num에 house data의 개수 저장
  cout << "# of house data: " << num << endl; // house data의 개수 출력

  // 3. Write a statement to get the first house data to 'house' (please refer to dataset.h) 
  house = ds->getHouseData(0); // house에 첫 번째 house data 저장
  
  // Write a statement to get the 10th house data to 'house' (dataset.h)
  house = ds->getHouseData(9); // house에 10번째 house data 저장
  
  // Get the power data to 'pdata' (house_data.h)
  pdata = house->getPowerData(); // pdata에 10번째 가구의 power data 저장
  
  // Get the daily power value to 'power' and print out the value (power_data.h)
  power = pdata->getValue(); // 전력값을 double 형태로 가져옴
  cout << "Power: " << power << endl; // daily power value 출력
  
  // Explicitly cast the type from double to int and assign it to 'tmp', and print out the value
  tmp = (int)pdata->getValue(); // 실수인 전력값을 int로 형변환(소수점 버림)
  cout << "Power (casted): " << tmp << endl; // 형변환된 전력값 출력
  
  // Compute the value averaged over all the power data by using 'sum_power' and 'num', 
  // assign the average value to 'avg_power', and print out the value
  sum_power = 0;
  for (int i=0; i<num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    sum_power += pdata->getValue(); // num 개의 전력값을 sum_power에 모두 더함
  }
  avg_power = sum_power / num; // 평균 전력값 계산
  cout << "Power (avg): " << avg_power << endl; // 평균 전력값 출력
  
  // Find the maximum value among all the power data 
  max_power = -1; // max_power 초기값 설정
  for (int i=0; i<num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    power = pdata->getValue(); // power에 num 개의 전력값을 하나씩 저장

    if (power > max_power)
      max_power = power; // power가 max_power보다 크면 max_power에 power값 저장
  }
  cout << "Power (max): " << max_power << endl; // 최대 전력값 출력
  
  // Find the minimum value among all the power data
  min_power = 10000; // min_power 초기값 설정
  for (int i=0; i<num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    power = pdata->getValue(); // power에 num 개의 전력값을 하나씩 저장

    if (power < min_power)
      min_power = power; // power가 min_power보다 작으면 min_power에 power값 저장
  }
  cout << "Power (min): " << min_power << endl; // 최소 전력값 출력

  // 4. Write a statement to get the temperature data to 'tdata' (dataset.h)
  tdata = ds->getTemperatureData(); // tdata에 온도 데이터 저장
  
  // Get the maximum value of the daily temperature (temperature_data.h)
  max_temp = tdata->getMax(); // max_temp에 일일 온도 최대값 저장
  cout << "Temperature (max): " << max_temp << endl; // 일일 온도 최대값 출력
  
  // Get the average value of the daily temperature (temperature_data.h)
  avg_temp = tdata->getValue(); // avg_temp에 일일 온도 평균값 저장
  cout << "Temperature (avg): " << avg_temp << endl; // 일일 온도 평균값 출력
  
  // Get the minimum value of the daily temperature (temperature_data.h)
  min_temp = tdata->getMin(); // min_temp에 일일 온도 최소값 저장
  cout << "Temperature (min): " << min_temp << endl; // 일일 온도 최소값 출력
  
  // Explicitly cast the type of the maximum value from double to int, assign the resultant value to 'tmp', and print it out
  tmp = (int)tdata->getMax(); // 일일 온도 최대값을 int로 형변환하여 tmp에 저장
  cout << "Temperature (max, casted): " << tmp << endl; // 형변환된 일일 온도 최대값 출력

  // 5. Write a statement to get the humidity data to 'hdata' (dataset.h)
  hdata = ds->getHumidityData(); // hdata에 습도 데이터 저장
  
  // Get the maximum value of the daily humidity (humidity_data.h)
  max_humid = hdata->getMax(); // max_humid에 일일 습도 최대값 저장
  cout << "Humidity (max): " << max_humid << endl; // 일일 습도 최대값 출력
  
  // Get the average value of the daily humidity (humidity_data.h)
  avg_humid = hdata->getValue(); // avg_humid에 일일 습도 평균값 저장
  cout << "Humidity (avg): " << avg_humid << endl; // 일일 습도 평균값 출력
  
  // Get the minimum value of the daily humidity (humidity_data.h)
  min_humid = hdata->getMin(); // min_humid에 일일 습도 최소값 저장
  cout << "Humidity (min): " << min_humid << endl; // 일일 습도 최소값 출력
  
  // Explicitly cast the type of the minimum value from double to int, assign the resultant value to 'tmp', and print it out
  tmp = (int)hdata->getMin(); // 일일 습도 최소값을 int로 형변환하여 tmp에 저장
  cout << "Humidity (min, casted): " << tmp << endl; // 형변환된 일일 습도 최소값 출력

  // 6. Initialize the buffer 'buf' with zeros (its length is defined as BUFLEN) (use the memset() function, please google it!)
  memset(buf, 0, BUFLEN); // buf를 0으로 초기화 (BUFLEN 길이만큼)

  // 7. Write statements to save the values into 'buf' using 'p' as follows:
  // # of house data (2 bytes) || maximum power (integer) (4 bytes) || maximum temperature (integer) (2 bytes)
  // Print out the buffer
  // Please use the macros defined in edge/byte_op.h
  p = buf; // p를 buf의 시작 주소로 초기화
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(num, p); // num을 2바이트 Big Endian 형식으로 buf에 저장하고 p를 2바이트만큼 이동 (가구 데이터 개수 패킹)
  tmp = (int)max_power; // max_power를 int로 형변환하여 tmp에 저장 (기존 코드는 min_power 이였지만, 문제가 요구하는 것이 maximum power이므로 max_power로 수정하였습니다.)
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(tmp, p); // tmp를 4바이트 Big Endian 형식으로 buf에 저장하고 p를 4바이트만큼 이동 (최대 전력값 패킹)
  tmp = (int)max_temp; // max_temp를 int로 형변환하여 tmp에 저장
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(tmp, p); // tmp를 2바이트 Big Endian 형식으로 buf에 저장하고 p를 2바이트만큼 이동 (최대 온도값 패킹) (기존 코드는 4바이트로 패킹하려 했지만, 문제가 요구하는 것이 2바이트이므로 2바이트로 수정하였습니다.)

  tmp = p - buf; // tmp에 p와 buf의 차이 (저장된 총 바이트 수) 저장
  PRINT_MEM(buf, tmp); // buf의 내용을 tmp 바이트만큼 출력

	return 0;
}
