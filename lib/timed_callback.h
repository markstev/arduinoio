#ifndef JDUINO_UC_TIMED_CALLBACK_H_
#define JDUINO_UC_TIMED_CALLBACK_H_

namespace arduinoio {

template<class T>
class TimedCallback {
 public:
  TimedCallback(unsigned long msec_delay, T* object, void (T::*member_func)())
    : usec_delay_(msec_delay * 1000), object_(object), member_func_(member_func) {
    start_time_ = micros();
  }

  TimedCallback(bool unused_bool, unsigned long usec_delay, T* object, void (T::*member_func)())
    : usec_delay_(usec_delay), object_(object), member_func_(member_func) {
    start_time_ = micros();
  }

  void Update() {
    unsigned long now = micros();
    if (now - start_time_ > usec_delay_) {
      (object_->*member_func_)();
      delete this;
    }
  }

  ~TimedCallback() {
  }
 private:
  unsigned long usec_delay_;
  T* object_;
  void (T::*member_func_)();
  unsigned long start_time_;
};

}  // namespace arduinoio

#endif  // JDUINO_UC_TIMED_CALLBACK_H_
