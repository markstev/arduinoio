#include "serial_module.h"
#include "timed_callback.h"
#include "uc_module.h"

namespace arduinoio {

const Message* SerialRXModule::Tick() {
  if (sending_) {
    timed_callback_->Update();
    return NULL;
  }
  if (clear_on_next_tick_) {
    clear_on_next_tick_ = false;
    message_.Clear();
  }
  while (software_serial_->available()) {
    bool finished = message_.AddByte(
        (unsigned char)software_serial_->read() & 0xff);
    if (finished) {
      clear_on_next_tick_ = true;
      return &message_;
    } else if (message_.error()) {
      message_.Clear();
    }
  }
  // TODO: clear if the message does not complete quickly.
  return NULL;
}

bool SerialRXModule::AcceptMessage(const Message &message) {
  if (!sending_) {
    int length;
    const unsigned char* command = message.command(&length);
    //if (command[0] == 'S' && command[1] == 'e' && command[2] == 't') {
    if (message.address() != 0) {
      bytes_sending_[0] = message.address_length();
      bytes_sending_[1] = length;
      bytes_sending_[2] = 0;
      bytes_sending_[3] = message.address();
      for (int i = 0; i < length; ++i) {
        bytes_sending_[4 + i] = command[i];
      }
      length_sending_ = length + 4 + 2;
      bytes_sending_[length + 4] = message.second_checksum();
      bytes_sending_[length + 4 + 1] = message.first_checksum();
      sending_ = true;
      //index_sending_ = 1;
      timed_callback_ = new TimedCallback<SerialRXModule>(0, this,
          &SerialRXModule::SendBytes);
      return true;
    }
  }
  return false;
}

void SerialRXModule::SendBytes() {
  for (int i = 0; i < length_sending_; ++i) {
    software_serial_->write(bytes_sending_[i]);
  }
  sending_ = false;
}

}  // namespace arduinoio
