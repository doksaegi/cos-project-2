#ifndef __HUMIDITY_DATA_H__
#define __HUMIDITY_DATA_H__

#include "sensor_data.h"

class HumidityData : public SensorData {
private:
  HumidityData *next;

public:
  HumidityData(time_t timestamp, double min, double max, double avg);

  void          setNext(HumidityData *next);
  HumidityData *getNext();
};

#endif /* __HUMIDITY_DATA_H__ */
