#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif 
#include <SoftwareSerial.h>

#include "lib/uc_module.h"
#include "lib/serial_module.h"
#include "lib/arduinoio.h"

const int pin = 13;
const int inside_output_pin = 7;
const int outside_output_pin = 6;
const int IR_RX_PIN = 2;
const int IR_TX_PIN= 3;
const int SERIAL_RX_PIN = 0;
const int SERIAL_TX_PIN = 1;
const int MOTOR_DIR_PIN = 10;
const int MOTOR_ON_PIN = 8;
const int SONAR_TRIGGER_PIN = 11;
const int SONAR_ECHO_PIN = 12;
const int OPEN_STOP_PIN = 4;
const int CLOSE_STOP_PIN = 5;

const int INSIDE_LIGHTS_ADDRESS = 1;

SoftwareSerial *serial;

arduinoio::ArduinoIO arduino_io;
void setup() {                
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  pinMode(pin, OUTPUT);
  pinMode(inside_output_pin, OUTPUT);
  pinMode(outside_output_pin, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_ON_PIN, OUTPUT);
  pinMode(SONAR_TRIGGER_PIN, OUTPUT);
  pinMode(SONAR_ECHO_PIN, INPUT);
  pinMode(OPEN_STOP_PIN, INPUT);
  pinMode(CLOSE_STOP_PIN, INPUT);


  serial = new SoftwareSerial(SERIAL_RX_PIN, SERIAL_TX_PIN);
  serial->begin(9600);
  arduino_io.Add(new arduinoio::SerialRXModule(serial, 0));
}

void loop() {
  arduino_io.HandleLoopMessages();
}
