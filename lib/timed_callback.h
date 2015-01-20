#ifndef JDUINO_UC_TIMED_CALLBACK_H_
#define JDUINO_UC_TIMED_CALLBACK_H_

namespace arduinoio {

template<class T>
class TimedCallback {
 public:
  TimedCallback(unsigned long msec_delay, T* object, void (T::*member_func)())
    : msec_delay_(msec_delay), object_(object), member_func_(member_func) {
    start_time_ = millis();
  }

  void Update() {
    unsigned long now = millis();
    if (now - start_time_ > msec_delay_) {
      (object_->*member_func_)();
      delete this;
    }
  }

  ~TimedCallback() {
  }
 private:
  T* object_;
  void (T::*member_func_)();
  unsigned long start_time_;
  unsigned long msec_delay_;
};

}  // namespace arduinoio

#endif  // JDUINO_UC_TIMED_CALLBACK_H_
