import os
import serial
import sys
import time


class Message:
  def __init__(self):
    self.address_length = 0
    self.command_length = 0
    self.timeout = 0

    self.address = 0
    self.command = []

    self.has_address_length = False
    self.has_command_length = False
    self.has_timeout = False
    self.byte_index = 0
    self.first_checksum = 0
    self.second_checksum = 0
    self.error = False

  def AddByte(self, byte):
    if not self.has_address_length:
      if (byte != 1):
        self.error = True
      self.address_length = byte
      self.has_address_length = True
    elif not self.has_command_length:
      self.command_length = byte
      self.has_command_length = True
    elif not self.has_timeout:
      self.timeout = byte
      self.has_timeout = True
    elif self.byte_index < self.address_length:
      address_tmp = byte
      self.address |= address_tmp << (self.byte_index * 8)
      self.byte_index += 1
    elif self.byte_index - self.address_length < self.command_length:
      self.command.append(byte)
      self.byte_index += 1
      if self.no_checksums and self.address_length + self.command_length == self.byte_index:
        return True
    elif self.byte_index - self.address_length == self.command_length:
      self.byte_index += 1
      if byte != self.second_checksum:
        print "Error -- bad checksum."
        self.error = True
      return False
    elif self.byte_index - self.address_length == self.command_length + 1:
      self.byte_index += 1
      if byte != self.first_checksum:
        self.error = True
      return not self.error
    else:
      print "Error -- message not properly processed"
      self.error = True
    self.first_checksum = (self.first_checksum + byte) & 0xff
    self.second_checksum = (self.second_checksum + self.first_checksum) & 0xff
    return False

  def Error(self):
    return self.error

  def __str__(self):
    return "Address: %d (len %d)\nCommand: %s (len %d)\nTimeout: %d" % (
        self.address, self.address_length, str(self.command),
        self.command_length, self.timeout)


class Command(object):
  LIGHT_ON = chr(1)
  LIGHT_OFF = chr(2)
  CURTAIN_OPEN = chr(3)
  CURTAIN_CLOSE = chr(4)
  CURTAIN_STOP = chr(5)


class SerialInterface(object):
  def __init__(self):
    self.last_port_lookup_time = 0
    self.old_ports = []
    self.last_read_time = time.time()
    self.message = Message()
    self.ReconnectIfNeeded()

  def Connect(self):
    self.ser = serial.Serial(self.port, 115200, timeout=0)  # 1s timeout
    time.sleep(1.0)

  def ReconnectIfNeeded(self):
    if time.time() > self.last_port_lookup_time + 30.0:
      serial_ports = ["/dev/" + x for x in os.listdir("/dev/") if "ttyACM" in x]
      new_ports = [x for x in serial_ports if x not in self.old_ports]
      if new_ports:
        self.port = new_ports[0]
        print "changing port to %s" % self.port
        self.old_ports.extend(new_ports)
        self.Connect()
        self.last_port_lookup_time = time.time()

  def MessageToBytes(self, address, command, timeout=0):
    command += "\0"
    len_addr = 1
    len_command = len(command)
    as_bytes = []
    as_bytes.append(chr(len_addr))
    as_bytes.append(chr(len_command))
    as_bytes.append(chr(timeout))
    as_bytes.append(chr(address))
    for c in command:
      as_bytes.append(c)
    first_checksum, second_checksum = self.Fletcher16Checksum(as_bytes)
    as_bytes.append(chr(second_checksum))
    as_bytes.append(chr(first_checksum))
    return as_bytes

  def Fletcher16Checksum(self, data):
    first_sum = 0
    second_sum = 0
    for b in data:
      first_sum = (first_sum + ord(b)) & 0xff
      second_sum = (second_sum + first_sum) & 0xff
    return first_sum, second_sum

  def Write(self, address, command, timeout=0):
    self.ReconnectIfNeeded()
    for retry in range(10):
      for byte in self.MessageToBytes(address, command, timeout):
        self.ser.write(str(byte))
      time.sleep(0.01)  # 10 msec
      while True:
        message = self.Read(no_checksums=True)
        if message:
          if message.command[0] == ord("R"):
            return
          elif message.command[0] == ord("E"):
            print "Error; resending"
            break
          else:
            print "Unknown command: %s" % message.command[0]

  def Read(self, no_checksums=False):
    if time.time() - self.last_read_time > 0.01:  # seconds
      #print "Reset read"
      self.message = Message()
    self.message.no_checksums = no_checksums
    byte = self.ser.read()
    if byte:
      self.last_read_time = time.time()
      #print ord(byte)
      complete = self.message.AddByte(ord(byte))
      if complete:
        complete_message = self.message
        self.message = Message()
        # print "Good message."
        return complete_message
      elif self.message.Error():
        # print "Bad message."
        self.message = Message()
    return None

  def Clear(self):
    print "Clearing message."
    self.message = Message()
