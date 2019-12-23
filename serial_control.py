import enum
import logging
import os
import serial
import sys
import time
from threading import Lock

logging.basicConfig(level=logging.INFO,
    format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
    datefmt='%m-%d %H:%M:%S')


class SlaveState(enum.Enum):
  READY = 1
  ERROR = 2
  UNKNOWN = 3


class Message:
  def __init__(self, expected_length=-1):
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

    self.expected_length = expected_length

  def AddByte(self, byte):
    if not self.has_address_length:
      if (byte != 1):
        self.error = True
      self.address_length = byte
      self.has_address_length = True
    elif not self.has_command_length:
      self.command_length = byte
      self.has_command_length = True
      if self.expected_length > -1 and self.address_length + self.command_length != self.expected_length:
        logging.error("Expected length on read did not match.")
        self.error = True
        self.command_length = self.expected_length - 1
        self.address_length = 1
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
      if (self.byte_index - self.address_length == self.command_length) and self.no_checksums:
        return True
    elif self.byte_index - self.address_length == self.command_length:
      self.byte_index += 1
      if byte != self.second_checksum:
        logging.info("Checksums enabled: %s", not self.no_checksums)
        logging.error("Error -- bad checksum.")
        self.error = True
      return False
    elif self.byte_index - self.address_length == self.command_length + 1:
      self.byte_index += 1
      if byte != self.first_checksum:
        self.error = True
      return not self.error
    else:
      logging.error("Error -- message not properly processed")
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
  def __init__(self, device_basename="ttyACM", baud=115200):
    self.last_port_lookup_time = 0
    self.old_ports = []
    self.last_read_time = time.time()
    self.baud = baud
    self.message = Message()
    self.device_basename = device_basename
    print "Connecting to %s at %s" % (device_basename, baud)
    self.ReconnectIfNeeded()
    self.reading = False
    self.empty_reads = 0
    self.write_mutex = Lock()

  def Connect(self):
    self.ser = serial.Serial(self.port, self.baud, timeout=0, rtscts=True)  # 1s timeout
    #self.ser = serial.Serial(self.port, 50000, timeout=0)  # 1s timeout
    time.sleep(2.0)
    print "Connected."

  def ReconnectIfNeeded(self):
    if time.time() > self.last_port_lookup_time + 30.0:
      serial_ports = ["/dev/" + x for x in os.listdir("/dev/") if self.device_basename in x]
      new_ports = [x for x in serial_ports if x not in self.old_ports]
      if new_ports:
        self.port = new_ports[0]
        logging.info("changing port to %s" % self.port)
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
    with self.write_mutex:
      t = time.time()
      is_stuck = False
      self.ReconnectIfNeeded()
      for retry in range(1000):
        while self.GetSlaveState() != SlaveState.READY:
          pass
        if retry > 0:
          logging.info("RETRY MESSAGE: %s", command)
        for byte in self.MessageToBytes(address, command, timeout):
          if time.time() - t > 1:
            logging.error("Stuck on Write, retry=%d", retry)
            t = time.time()
          #logging.info("Write: %s", str(byte))
          self.ser.write(str(byte))
        #logging.info("Check %s", command)
        while True:
          if time.time() - t > 1:
            logging.error("Stuck on Ack, retry=%d", retry)
            t = time.time()
            is_stuck = True
          slave_state = self.GetSlaveState()
          if slave_state == SlaveState.READY:
            if retry > 0:
              logging.info("Recovered.")
            return
          elif slave_state == SlaveState.ERROR:
            logging.error("Error; resending")
            break
      #logging.info("Write complete (fall through).")
      raise Exception("failed to write")

  def GetSlaveState(self):
    byte = None
    state = SlaveState.UNKNOWN
    while state == SlaveState.UNKNOWN or byte:
      byte = self.ser.read()
      if byte:
        if byte == "R":
          state = SlaveState.READY
        if byte == "E":
          state = SlaveState.ERROR
    logging.info("Resulting state = %s", state)
    return state

  def Read(self, no_checksums=False, expected_length=-1, verbose=False):
    if time.time() - self.last_read_time > 0.1:  # seconds
      logging.info("Empty reads: %d", self.empty_reads)
      self.message = Message(expected_length=expected_length)
    self.message.no_checksums = no_checksums
    byte = self.ser.read()
    if not (byte and (self.reading or byte not in ("R", "E"))):
      self.empty_reads += 1
    else:
      self.empty_reads = 0
      self.reading = True
      #logging.info("Time gap: %f", time.time() - self.last_read_time)
      self.last_read_time = time.time()
      if verbose:
        print ord(byte)
      complete = self.message.AddByte(ord(byte))
      if complete:
        complete_message = self.message
        self.message = Message(expected_length=expected_length)
        self.reading = False
        if verbose:
          print "Good message."
        return complete_message
      elif self.message.Error():
        if verbose:
          print "Bad message."
        self.message = Message(expected_length=expected_length)
        self.reading = False
    return None

  # Note: 0.01s timeout on read, and 0.02s delay before read works well.

  def Clear(self):
    self.message = Message()
