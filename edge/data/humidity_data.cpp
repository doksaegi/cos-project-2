#include "humidity_data.h"

HumidityData::HumidityData(time_t timestamp, double min, double max, double avg)
  : SensorData(timestamp, min, max, avg, "%"),
    next(NULL)
{
}

void HumidityData::setNext(HumidityData *next)
{
  this->next = next;
}

HumidityData *HumidityData::getNext()
{
  return this->next;
}
