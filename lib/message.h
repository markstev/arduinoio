#ifndef JDUINO_UC_MESSAGE_H_
#define JDUINO_UC_MESSAGE_H_

namespace arduinoio {

const int MAX_BUFFER_SIZE = 50;
class Message {
 public:
  Message() {
    Clear();
  }

  Message(unsigned char address, unsigned char command_length,
      const unsigned char* command)
    : address_(address), address_length_(1), command_length_(command_length) {
    for (int i = 0; i < command_length_; ++i) {
      command_[i] = command[i];
    }
  }

  ~Message() {}

  void Reset(unsigned char address, unsigned char command_length,
      const unsigned char* command) {
    address_ = address;
    address_length_ = 1;
    command_length_ = command_length;
    for (int i = 0; i < command_length_; ++i) {
      command_[i] = command[i];
    }
  }
  
  void Clear() {
    has_address_length_ = false;
    has_command_length_ = false;
    has_timeout_ = false;
    byte_index_ = 0;
    address_ = 0;
    command_length_ = 0;
    first_checksum_ = 0;
    second_checksum_ = 0;
    error_ = false;
  }
  
  // Returns true when the message is complete.
  bool AddByte(unsigned char next_byte) {
    if (!has_address_length_) {
      address_length_ = next_byte;
      has_address_length_ = true;
    } else if (!has_command_length_) {
      command_length_ = next_byte;
      has_command_length_ = true;
    } else if (!has_timeout_) {
      timeout_ = next_byte;
      has_timeout_ = true;
    } else if (byte_index_ < address_length_) {
      int address_tmp = next_byte;
      address_ |= address_tmp << (byte_index_ * 8);
      byte_index_++;
    } else if (byte_index_ - address_length_ < command_length_) {
      command_[byte_index_ - address_length_] = next_byte;
      byte_index_++;
    } else if (byte_index_ - address_length_ == command_length_) {
      byte_index_++;
      if (next_byte != second_checksum_) {
        error_ = true;
      }
      return false;
    } else if (byte_index_ - address_length_ == command_length_ + 1) {
      byte_index_++;
      if (next_byte != first_checksum_) {
        error_ = true;
      }
      return !error_;
    } else {
      // error
    }
    first_checksum_ = (first_checksum_ + next_byte) & 0xff;
    second_checksum_ = (second_checksum_ + first_checksum_) & 0xff;
    return false;
  }
  
  int address() const {
    return address_;
  }

  int address_length() const {
    return address_length_;
  }
  
  const unsigned char* command(int *length) const {
    *length = command_length_;
    return command_;
  }

  bool error() const {
    return error_;
  }

  unsigned char first_checksum() const {
    return first_checksum_;
  }

  unsigned char second_checksum() const {
    return second_checksum_;
  }
  
 private:
  int address_length_;
  int command_length_;
  int timeout_;
  
  int address_;
  unsigned char command_[MAX_BUFFER_SIZE];
  
  // Used for constructing messages.
  bool has_address_length_;
  bool has_command_length_;
  bool has_timeout_;
  int byte_index_;
  int first_checksum_;
  int second_checksum_;
  bool error_;
};

}  // namespace arduinoio

#endif  // JDUINO_UC_MESSAGE_H_
