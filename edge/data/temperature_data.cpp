#include "temperature_data.h"

TemperatureData::TemperatureData(time_t timestamp, double min, double max, double avg)
  : SensorData(timestamp, min, max, avg, "celcius"),
    next(NULL)
{
}

void TemperatureData::setNext(TemperatureData *next)
{
  this->next = next;
}

TemperatureData *TemperatureData::getNext()
{
  return this->next;
}
