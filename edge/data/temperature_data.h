#ifndef __TEMPERATURE_DATA_H__
#define __TEMPERATURE_DATA_H__

#include "sensor_data.h"

class TemperatureData : public SensorData {
private:
  TemperatureData *next;

public:
  TemperatureData(time_t timestamp, double min, double max, double avg);

  void             setNext(TemperatureData *next);
  TemperatureData *getNext();
};

#endif /* __TEMPERATURE_DATA_H__ */
