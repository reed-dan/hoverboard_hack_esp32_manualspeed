# hoverboard_hack_esp32_manualspeed
ESP32 script to control hacked hoverboard

Can control
https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x
or
https://github.com/AILIFE4798/Hoverboard-Firmware-Hack-Gen2.x-MM32

Current Control Methods
  Serial

Future Control Methods
The possibilities are endless
  ble/bluetooth
  rc receiver (PPM)
  servo (PWM)
  WiFi?
  potentiometer/twisth throthle (ADC)
  MQTT

Current Inputs/Commands
  format: hover|motorid|speed|state

Control 1 single motor
  hover|0|300|1

Control ALL Motors
  hover|all|300|1

Control Right Motors
  hover|right|300|1

Control Left Motors
  hover|left|300|1

Set all motors to 0
  stop
