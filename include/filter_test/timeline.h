/*
 * timeline.h
 *
 *  Created on: 15.06.2017
 *      Author: burrimi
 */

#ifndef INCLUDE_FILTER_TEST_TIMELINE_H_
#define INCLUDE_FILTER_TEST_TIMELINE_H_

#include <map>
#include <vector>

#include "filter_test/measurement.h"
#include "filter_test/utils/logging.h"

namespace tsif {

using TimedMeasurements = std::map<int, MeasurementBase*>;
using TimedMeasurement = std::pair<int, MeasurementBase*>;

class Timeline {
 public:
  // If a timeline is not mergeable then we have to apply the measurement before continuing.
  // This is usually the case for traditional measurements in a Kalman filter (they are not mergeable).
  bool mergeable_ = true;
  TimedMeasurements timed_measurements_;
  void addMeasurement(const int timestamp_ns, MeasurementBase* measurement) {
    while (kMaxMeasurmenetBufferSize <= timed_measurements_.size()) {  // remove oldest if buffer is full
      deleteMeasurement(timed_measurements_.begin());
    }
    timed_measurements_.emplace_hint(timed_measurements_.end(), timestamp_ns, measurement);
  }
  void deleteOlderThen(const int timestamp_ns);  // TODO(burrimi):implement.
  ~Timeline() {
    // Clean up measurement buffer.
    for (auto& timed_measurement : timed_measurements_) {
      delete timed_measurement.second;
    }
  }
  inline int getOldestMeasurementTimestamp() const { return timed_measurements_.begin()->first; }

  // Finds the first measurement where measurement timestamp > timestamp_ns. Returns -1 in case no newer measurement exists.
  inline int getNextMeasurementTimestamp(const int timestamp_ns) const {
    const auto it = timed_measurements_.upper_bound(timestamp_ns);
    if (it != timed_measurements_.end()) {
      return it->first;
    }
    return -1;
  }

  inline const MeasurementBase* getMeasurement(const int timestamp_ns) const {
    const auto& timed_measurement = timed_measurements_.find(timestamp_ns);
    if (timed_measurement != timed_measurements_.end()) {
      return timed_measurement->second;
    }
    return NULL;
  }

  void InterpolateAndAddAt(const int timestamp_ns) {
    auto it_next = timed_measurements_.lower_bound(timestamp_ns);
    if(it_next->first != timestamp_ns) {
      auto it_previous = it_next;
      --it_previous;
      MeasurementBase* next_measurement = it_next->second;
      MeasurementBase* m1 = it_previous->second->split(*next_measurement, it_previous->first, timestamp_ns, it_next->first);
      addMeasurement(timestamp_ns, m1);
      TSIF_LOG("Split measurements at " + std::to_string(it_previous->first) + " and " + std::to_string(it_next->first) + " at " + std::to_string(timestamp_ns));
    }
  }

  // This function returns a vector of measurements in the requested range. If measurements at the boarder don't exist they are interpolated and added to the timeline.
  // time      x   x   x   x   x   x   x
  // min/max     ^               ^
  // return      x x   x   x   x x
  std::vector<TimedMeasurement> getMeasurementsInRange(const int timestamp_min_ns, const int timestamp_max_ns) {
    std::vector<TimedMeasurement> timed_measurements;
    if(getOldestMeasurementTimestamp() > timestamp_min_ns || getNewestMeasurementTimestamp() < timestamp_max_ns) {
      return timed_measurements;
    }

    // Create interpolated measurements at beginning of time interval.
    InterpolateAndAddAt(timestamp_min_ns);
    InterpolateAndAddAt(timestamp_max_ns);

    auto it_next = timed_measurements_.lower_bound(timestamp_min_ns);
    while(it_next->first <= timestamp_max_ns) {
      timed_measurements.emplace_back(*it_next);
      ++it_next;
    }

    return timed_measurements;
  }

  inline int getNewestMeasurementTimestamp() const { return timed_measurements_.rbegin()->first; }
  inline int getNumMeasurements() const { return timed_measurements_.size(); }
  inline bool isEmpty() const { return timed_measurements_.empty(); }
  inline void deleteMeasurement(const TimedMeasurements::iterator& iterator) {
    delete iterator->second;              // Free memory.
    timed_measurements_.erase(iterator);  // Remove from buffer.
  }
};

}  // namespace tsif

#endif /* INCLUDE_FILTER_TEST_TIMELINE_H_ */
