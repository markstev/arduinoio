#ifndef JDUINO_UC_ARDUINOIO_H_
#define JDUINO_UC_ARDUINOIO_H_

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif 

#include "message.h"
#include "uc_module.h"

namespace arduinoio {

class ArduinoIO {
 public:
  ArduinoIO() : num_modules_(0),
      first_message_(NULL) {}

  ~ArduinoIO() {
    for (int i = 0; i < num_modules_; ++i) {
      delete modules_[i];
    }
  }

  void Add(UCModule *module) {
    modules_[num_modules_] = module;
    num_modules_++;
  }

  void HandleLoopMessages() {
    for (int i = 0; i < num_modules_; ++i) {
      const Message* message;
      if (first_message_ != NULL) {
        message = first_message_;
      } else {
        message = modules_[i]->Tick();
      }
      if (message != NULL) {
        int length;
        const unsigned char* cmd = message->command(&length);
        for (int j = 0; j < num_modules_; ++j) {
          modules_[j]->AcceptMessage(*message);
        }
      }
      if (first_message_ != NULL) {
        delete first_message_;
        first_message_ = NULL;
      }
    }
  }
 private:
  UCModule* modules_[10];
  int num_modules_;
  Message* first_message_;
};

}  // namespace arduinoio

#endif  // JDUINO_UC_ARDUINOIO_H_
