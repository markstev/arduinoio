#=============================================================================#
# Author: MarkStevens
# Date:   2012/04/17
#                                                                             #
# Description: Playing around with serial communication protocol              #
#                                                                             #
#=============================================================================#

cmake_minimum_required(VERSION 2.8)
#====================================================================#
#  Settings                                                          #
#====================================================================#

set(CMAKE_SOURCE_DIR /home/pi/arduinoio) # Source dir
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake) # Arduino Toolchain

project(ArduinoExample C CXX)
set(ARDUINO_SDK /usr/share/arduino-1.0.6)
set(FIRMWARE_NAME example)


set(${FIRMWARE_NAME}_BOARD uno)               # Arduino Target board
set(${FIRMWARE_NAME}_SRCS  example.cc)
#set(${FIRMWARE_NAME}_HDRS  lib/serial_module.h)

set(${FIRMWARE_NAME}_PORT /dev/ttyACM0)            # Serial upload port

#====================================================================#
#  Target generation                                                 #
#====================================================================#
generate_arduino_firmware(${FIRMWARE_NAME})
