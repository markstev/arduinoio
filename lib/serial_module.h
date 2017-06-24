#ifndef JDUINO_UC_SERIAL_MODULE_H_
#define JDUINO_UC_SERIAL_MODULE_H_


#include <SoftwareSerial.h>

#include "message.h"
#include "timed_callback.h"
#include "uc_module.h"

namespace arduinoio {

const unsigned char READY = 0x52;
const unsigned char ERROR = 0x45;

const int MESSAGE_SIZE = 20;

class SerialRXModule : public UCModule {
 public:
  SerialRXModule(SoftwareSerial *software_serial, int address)
    : software_serial_(software_serial), address_(address), sending_(false),
    last_message_time_(micros()), state_(READY) {
  }

  virtual const Message* Tick() {
    if (sending_) {
      timed_callback_->Update();
      return NULL;
    }
    if (clear_on_next_tick_) {
      clear_on_next_tick_ = false;
      message_.Clear();
      SendReady();
      return NULL;
    }
    while (Serial.available()) {  //software_serial_->available()) {
      last_message_time_ = micros();
      bool finished = message_.AddByte(
          (unsigned char)Serial.read() & 0xff);
      if (finished) {
        clear_on_next_tick_ = true;
        state_ = READY;
        return &message_;
      } else if (message_.error()) {
        message_.Clear();
        state_ = ERROR;
      }
    }
    if (micros() - last_message_time_ > 10000) {  // 10ms
      SendReady();
      message_.Clear();
    }
    // TODO: clear if the message does not complete quickly.
    return NULL;
  }

  void SendReady() {
    if (!sending_) {
      sending_ = true;
      timed_callback_ = new TimedCallback<SerialRXModule>(0, this,
          &SerialRXModule::WriteState);
    }
  }

  void WriteState() {
    Serial.write(state_);
    sending_ = false;
    state_ = READY;
    last_message_time_ = micros();
  }

  virtual bool AcceptMessage(const Message &message) {
    if (!sending_) {
      int length;
      const unsigned char* command = message.command(&length);
      if (message.address() != address_) {
        bytes_sending_[0] = message.address_length();
        bytes_sending_[1] = length;
        bytes_sending_[2] = 0;
        bytes_sending_[3] = message.address();
        for (int i = 0; i < length; ++i) {
          bytes_sending_[4 + i] = command[i];
        }
        length_sending_ = length + 4;
        const bool kSendChecksums = false;
        if (kSendChecksums) {
          length_sending_ += 2;
          bytes_sending_[length + 4] = message.second_checksum();
          bytes_sending_[length + 4 + 1] = message.first_checksum();
        }
        sending_ = true;
        timed_callback_ = new TimedCallback<SerialRXModule>(0, this,
            &SerialRXModule::SendBytes);
        return true;
      }
    }
    return false;
  }

  void SendBytes() {
    for (int i = 0; i < length_sending_; ++i) {
      Serial.write(bytes_sending_[i]);
    }
    sending_ = false;
    last_message_time_ = micros();
  }

  virtual ~SerialRXModule() {}
 private:
  SoftwareSerial *software_serial_;
  int address_;
  bool sending_;
  unsigned long last_message_time_;
  unsigned char state_;
  Message message_;
  bool clear_on_next_tick_;
  unsigned char bytes_sending_[MAX_BUFFER_SIZE];
  int length_sending_;
  //int index_sending_;
  TimedCallback<SerialRXModule> *timed_callback_;

};

}  // namespace arduinoio

#endif  // JDUINO_UC_SERIAL_MODULE_H_
