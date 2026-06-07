#ifndef __SENSOR_DATA_H__
#define __SENSOR_DATA_H__

#include <string>
#include <ctime>
using namespace std;

class SensorData {
protected:
  time_t timestamp;
  double avg;
  double min;
  double max;
  string unit;

public:
  SensorData(time_t timestamp, double min, double max, double avg, const string &unit)
    : timestamp(timestamp), avg(avg), min(min), max(max), unit(unit) {}

  virtual ~SensorData() = default;

  virtual double getValue()              { return this->avg; }
  virtual void   setValue(double value)  { this->avg = value; }
  virtual double getMin()                { return this->min; }
  virtual void   setMin(double value)    { this->min = value; }
  virtual double getMax()                { return this->max; }
  virtual void   setMax(double value)    { this->max = value; }
  virtual time_t getTimestamp()          { return this->timestamp; }
  virtual void   setTimestamp(time_t ts) { this->timestamp = ts; }
  virtual string getUnit()               { return this->unit; }
};

#endif /* __SENSOR_DATA_H__ */
