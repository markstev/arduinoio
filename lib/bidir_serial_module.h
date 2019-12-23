#ifndef JDUINO_UC_SERIAL_MODULE_H_
#define JDUINO_UC_SERIAL_MODULE_H_


#include "message.h"
#include "timed_callback.h"
#include "uc_module.h"

namespace arduinoio {

const unsigned char READY_AND_SENDING = 0x52;
const unsigned char READY_AND_DONE = 0x53;
const unsigned char SENT_LAST = 0x54;
const unsigned char ERROR = 0x45;

// This communication protocol supports bidirectional communication with
// multiple master devices.
//
// TODO: we can support faster communication by passing more bytes per ack.
//
// Protocol:
// 1) Device A sends a ready char indicating whether it will write part of a message.
// 2) Device A sends the indicated char of the message, as indicated.
// 3) Device B responds with a ready char indicating whether it will write part of a message.
// 4) Device B sends the indicated char of the message, as indicated.
// Repeat...
//
// Error Handshakes:
// 1) Device A sends an error code.
// 2) Device B resets any active sends and replies with a ready code and any message.
//
// Initialization:
// 1) All devices are waiting
// 2) If a device waits for 200ms with no signal received, it will send an error byte,
//   triggering the error handshake.
//
// At any point, one device may respond to the other with an error char. This
// indicates the incoming message was not processed and should resend.
//
// At least one READY_AND_DONE should come after each message sent.
//
// Ready, send nothing char: 0x52
// Ready, send more char: 0x53
// Error, resend: 0x45
class BidirSerialRXModule : public UCModule {
  BidirSerialRXModule(int address, bool send_only)
    : address_(address), sending_(false), receive_state_(READY_AND_DONE),
    last_communication_time_(micros()), transmit_state_(READY_AND_DONE) {
  }

  virtual const Message* Tick() {
    // Receiving
    unsigned char new_rx_state = 0x00;
    if (!ReadFlushingErrors(&new_rx_state)) {
      if (micros() > kCommunicationTimeout) {
        SendError();
        last_communication_time_ = micros();
      }
      return;
    }
    bool finished = false;
    if (new_rx_state == ERROR) {
      ResetRX();
    } else {
      switch (receive_state_) {
        case READY_AND_SENDING: {
          if (new_rx_state != READY_AND_SENDING) {
            SendError();
            return nullptr;
          }
        }
        // FALLTHROUGH_INTENDED
        case READY_AND_DONE: {
          if (new_rx_state == READY_AND_SENDING) {
            unsigned char next_byte = 0x00;
            if (!ReadWithTimeout(&next_byte)) {
              SendError();
              return nullptr;
            }
            finished = message_.AddByte(
                (unsigned char)Serial.read() & 0xff);
            receive_state_ = finished ? SENT_LAST : READY_AND_SENDING;
          } else {
            SendError();
            return nullptr;
          }
          break;
        }
        case SENT_LAST: {
          if (new_rx_state == READY_AND_SENDING) {
            SendError();
            return nullptr;
          }
          receive_state_ = READY_AND_DONE;
          message_.Clear();
          break;
        }
        default: {
          SendError();
          return nullptr;
        }
      }
    }

    // Transmitting
    switch (transmit_state_) {
      case SENT_LAST:
        transmit_state_ = READY_AND_DONE;
        // FALLTHROUGH_INTENDED
      case READY_AND_DONE:
        Serial.write(transmit_state_);
        break;
      case READY_AND_SENDING: {
        Serial.write(transmit_state_);
        Serial.write(bytes_sending_[next_send_index_]);
        ++next_send_index_;
        if (next_send_index_ == length_sending_) {
          transmit_state_ = SENT_LAST;
        }
        break;
      }
      default:
        SendError();
        break;
    }
    last_communication_time_ = micros();
    return finished ? &message_ : nullptr;
  }

  bool ReadFlushingErrors(unsigned char *output) {
    bool read_something = false;
    while (Serial.available()) {
      read_something = true;
      *output = (unsigned char)Serial.read() & 0xff;
      if (*output != ERROR) return true;
    }
    return read_something;
  }

  bool ReadWithTimeout(unsigned char *output) {
    const unsigned long timeout = micros() + 200000LL;
    while (!Serial.available()) {
      if (micros() > timeout) return false;
    }
    int bytes_read = 0;
    while (Serial.available()) {
      ++bytes_read;
      *output = (unsigned char)Serial.read() & 0xff;
    }
    return bytes_read == 1;
  }
//// Byte0 was sent before byte1.
//bool ReadWithTimeout(unsigned char *byte0, unsigned char *byte1, unsigned long timeout) {
//  unsigned long timeout_time = micros() + timeout;
//  int bytes_read = 0;
//  unsigned char first, second;
//  while (Serial.available()) {
//    ++bytes_read;
//    if (bytes_read > 1) {
//      first = second;
//    }
//    second = (unsigned char)Serial.read() & 0xff;
//  }
//  // Let's see what we got.
//  if (bytes_read == 0) return false;
//  if (second != READY_AND_SENDING) {
//    byte0 = second;
//    return true;
//  }
//}

  void SendError() {
    Serial.write(ERROR);
    ResetRX();
  }

  void ResetRX() {
    next_send_index_ = 0;
    message_.Clear();
    receive_state_ = READY_AND_DONE;
    if (transmit_state_ != READY_AND_DONE) {
      transmit_state_ = READY_AND_SENDING;
      next_send_index_ = 0;
    }
  }

  virtual bool AcceptMessage(const Message &message) {
    if (transmit_state_ != READY_AND_DONE) {
      // Already sending something else.
      return false;
    }
    transmit_state_ = READY_AND_SENDING;
    next_send_index_ = 0;
    int length;
    const unsigned char* command = message.command(&length);
    if (message.address() == address_) {
      // Local message doesn't need to be sent.
      return false;
    }
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
    return true;
  }

 private:
  int address_;
  unsigned char transmit_state_;
  unsigned char receive_state_;
  Message incoming_message_;
  unsigned char bytes_sending_[MAX_BUFFER_SIZE];
  int next_send_index_;
  int length_sending_;
  unsigned long last_communication_time_;
};
