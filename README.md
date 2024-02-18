# Hoverboard Hack ESP32 Manual Speed
ESP32 script to control hacked hoverboard

## Can control

* https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x
* https://github.com/AILIFE4798/Hoverboard-Firmware-Hack-Gen2.x-MM32

Based on https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x-GD32/tree/main/Arduino%20Examples/TestSpeed

## Current Control Methods
  * Serial

## Future Control Methods
### The possibilities are endless
* ble/bluetooth
* rc receiver (PPM)
* servo (PWM)
* WiFi?
* potentiometer/twisth throthle (ADC)
* MQTT

## Current Inputs/Commands
###  format: hover|motorid|speed|state

### Control 1 single motor
* hover|0|300|1

### Control ALL Motors
* hover|all|300|1

### Control Right Motors
* hover|right|300|1

### Control Left Motors
* hover|left|300|1

### Set all motors to 0
* stop
