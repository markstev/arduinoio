

set(FIRMWARE_NAME example)

set(${FIRMWARE_NAME}_BOARD uno)               # Arduino Target board
set(${FIRMWARE_NAME}_SRCS  example.cc)

set(${FIRMWARE_NAME}_PORT /dev/ttyACM0)            # Serial upload port

#====================================================================#
#  Target generation                                                 #
#====================================================================#
generate_arduino_firmware(${FIRMWARE_NAME})
