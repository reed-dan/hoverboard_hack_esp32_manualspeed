//*************************//
//PRE RELEASE//
//*************************//

//Hoverboard Manual Speed
//designed for esp32
//based on https://github.com/RoboDurden/Hoverboard-Firmware-Hack-Gen2.x-GD32/tree/main/Arduino%20Examples/TestSpeed
//version 0.20240225 //added adc potentiometer support


#define _DEBUG      // debug output to first hardware serial port
//#define DEBUG_RX    // additional hoverboard-rx debug output
#define REMOTE_UARTBUS

#define SEND_MILLIS 100   // send commands to hoverboard every SEND_MILLIS millisesonds

#include "util.h"
#include "hoverserial.h"

//input method
//serial
#define input_serial
//ble
//rc receiver (PPM)
//servo (PWM)
//WiFi?
//#define input_ADC //potentiometer/twisth throthle (ADC)
//MQTT


//array for motors
//how many motors do you have
const size_t motor_count_total = 6;
//identify the motors by their slave number
int motors_all[motor_count_total] = {1, 2, 3, 4, 5, 6};
//how many right motors
const size_t motor_count_right = 3;
//identify the motors by their slave number
int motors_right[motor_count_right] = {0, 2, 4};
//how many left motors
const size_t motor_count_left = 3;
//identify the motors by their slave number
int motors_left[motor_count_left] = {1, 3, 5};
//array for speed
int motor_speed[motor_count_total];
//array for istate
int slave_state[motor_count_total];
//offset
int motoroffset = motors_all[0] - 0;



//
int slaveidin;
int iSpeed;
int ispeedin;
int istatein;
int count=0;
String command;



  #define oSerialHover Serial2    // ESP32

SerialHover2Server oHoverFeedback;



void setup()
{
  #ifdef _DEBUG
    Serial.begin(115200);
    Serial.println("Hello Hoverbaord V2.x :-)");
  #endif
  

#ifdef input_serial
 HoverSetupEsp32(oSerialHover,19200,16,17);
 #else
 #endif

#ifdef input_ADC
  int min_speed  = -1000;
  int max_speed = 1000;
  //set both pins the same if you only have 1 adc
  int adc_input_pin_right = 36;
  int adc_input_pin_left = 37;
   HoverSetupEsp32(oSerialHover,19200,16,17);

  #endif
}


unsigned long iLast = 0;
unsigned long iNext = 0;
unsigned long iTimeNextState = 3000;
uint8_t  wState = 1;   // 1=ledGreen, 2=ledOrange, 4=ledRed, 8=ledUp, 16=ledDown, 32=Battery3Led, 64=Disable, 128=ShutOff
//id for messages being sent
uint8_t  iSendId = 0;   // only ofr UartBus

void loop()
{
  unsigned long iNow = millis();
  //digitalWrite(39, (iNow%500) < 250);
  //digitalWrite(37, (iNow%500) < 100);

//look for incoming serial command
//hover|slave/motorid|speed|state
//speed can be from -1000 reverse full speed to 1000 forward full speed
//state can be 1=ledGreen, 2=ledOrange, 4=ledRed, 8=ledUp, 16=ledDown, 32=Battery3Led, 64=Disable, 128=ShutOff


#ifdef input_serial
//read serial input
  if (Serial.available()) { // if there is data comming
    String command = Serial.readStringUntil('\n'); // read string until newline character
// check if input starts with hover
//if the hover command is present parse the input data
    if (command.substring(0,6) == "hover|") {
      //Serial.println("Command hover parse the data");
      //Serial.println(command);
      //remove the command
      //start at 0 remove 6 char for hover|
      command.remove(0,6);
      //Serial.println(command);

        if (command.substring(0,4) == "all|") //all
        {
          //remove all|
          command.remove(0,4);
          //parse the date
          int numParsed = sscanf(command.c_str(), "%d|%d", &ispeedin, &istatein);
          //verify we parsed 2 numbers
          if (numParsed == 2)
          {
            //set the data
            count = 0;
            while (count < motor_count_total)
            {
              //set motor speed
            motor_speed[count] = ispeedin;
             //set motor/mcu state
            slave_state[count] = istatein;  
              //increase count
              count++;
            } 
          }
          else
          {
            //present an error
            Serial.println("The command doesn't meet the criteria"); 
          }

        }
        else if (command.substring(0,6) == "right|") //right
        { 
          //remove right|
          command.remove(0,6);
          //parse the date
          int numParsed = sscanf(command.c_str(), "%d|%d", &ispeedin, &istatein);  
          //verify we parsed 2 numbers
          if (numParsed == 2)
          {
            //set the data
            count = 0;
            while (count < motor_count_right)
            {
              //set motor speed
              motor_speed[motors_right[count]-motoroffset] = ispeedin;
             //set motor/mcu state
            slave_state[motors_right[count]-motoroffset] = istatein;                
              //increase count
              count++;
            } 
          }
          else
          {
            //present an error
            Serial.println("The command doesn't meet the criteria"); 
          }

        }
        else if (command.substring(0,5) == "left|") //left
        { 
          //remove left|
          command.remove(0,5);
          //parse the date
          int numParsed = sscanf(command.c_str(), "%d|%d", &ispeedin, &istatein);
          //verify we parsed 2 numbers
          if (numParsed == 2)
          {
            //set the data
            count = 0;
            while (count < motor_count_left)
            {
              //set motor speed
              motor_speed[motors_left[count]-motoroffset] = ispeedin;
             //set motor/mcu state
            slave_state[motors_left[count]-motoroffset] = istatein;                
              //increase count
              count++;
            } 
          }
          else
          {
            //present an error
            Serial.println("The command doesn't meet the criteria"); 
          }
        }
        else //single number motor specified
        {
        int numParsed = sscanf(command.c_str(), "%d|%d|%d", &slaveidin, &ispeedin, &istatein);
  
            if (numParsed == 3) {
            //set motor speed
            motor_speed[slaveidin-motoroffset] = ispeedin; 
             //set motor/mcu state
            slave_state[slaveidin-motoroffset] = istatein; 
            } 
            else {
            // Handle parsing error
            Serial.println("The command doesn't meet the criteria");
            }
        }
} 
  else if (command.substring(0,6) == "stop")
  {
     //set the data
            count = 0;
            while (count < motor_count_total)
            {
              //set motor speed
              motor_speed[count] = 0;
             //set motor/mcu state
            slave_state[count] = istatein;                
              //increase count
              count++;
            } 
  }
else {
      Serial.println("Command not hover/stop");
      Serial.println(command);
  
    }
  #ifdef _DEBUG
int i = 0;
  while (i < motor_count_total)
  {
    Serial.print("Motor ");
    Serial.print(motors_all[i]);
    Serial.print(" Speed is set to ");
    Serial.print(motor_speed[i]);
    Serial.print(" Slave state is set to ");
    Serial.println(slave_state[i]);
    i++;
  }
#endif
  }


#endif

#ifdef input_adc
    // read the input on analog pin:
  int analogValueRight = analogRead(adc_input_pin_right);
  int analogValueLeft = analogRead(adc_input_pin_left);
  // Rescale to potentiometer's
  int deadband = 100; //this the the range in the middle that will output zero
  if (abs(analogValueRight-(4095/2))<deadband){//make it easier to get to 0
//1997.5 < input < 2097.5
    float adcspeedinright = 0;
  }
  else{
  float adcspeedinright = map(analogValueRight,0,4095,min_speed,max_speed);
  }
  if (abs(analogValueLeft-(4095/2))<deadband){//make it easier to get to 0
//1997.5 < input < 2097.5
    float adcspeedinleft = 0;
  }
  else{
  float adcspeedinright = map(analogValueLeft,0,4095,min_speed,max_speed);
  }

  
  #ifdef _DEBUG
    // print out the value you read:
    Serial.print("Analog Right: ");
    Serial.print(analogValueRight);
    Serial.print(", Speed: ");
    Serial.println(adcspeedinright);
    delay(1000);
    Serial.print("Analog Left: ");
    Serial.print(analogValueLeft);
    Serial.print(", Speed: ");
    Serial.println(adcspeedinleft);
    delay(1000);
  #endif
//set the values

   //right
            count = 0;
            while (count < motor_count_right)
            {
              //set motor speed
              motor_speed[motors_right[count]-motoroffset] = adcspeedinright;
             //set motor/mcu state
            //slave_state[motors_right[count]-motoroffset] = istatein;                
              //increase count
              count++;
            } 
  //left
            count = 0;
            while (count < motor_count_left)
            {
              //set motor speed
              motor_speed[motors_left[count]-motoroffset] = adcspeedinleft;
             //set motor/mcu state
            //slave_state[motors_left[count]-motoroffset] = istatein;                
              //increase count
              count++;
            } 
  
  #endif
  

  int iSteer =0;   // repeats from +100 to -100 to +100 :-)
  //int iSteer = 0;
  //int iSpeed = 500;
  //int iSpeed = 200;
  //iSpeed = iSteer = 0;

  if (iNow > iTimeNextState)
  {
    iTimeNextState = iNow + 3000;
    wState = wState << 1;
    if (wState == 64) wState = 1;  // remove this line to test Shutoff = 128
  }
  

  if (iNow > iNext)
  {
    //DEBUGLN("time",iNow)
    
    #ifdef REMOTE_UARTBUS
      count = 0;
      while (count < motor_count_total){
         HoverSend(oSerialHover,motors_all[count],motor_speed[count],slave_state[count]);
//           #ifdef _DEBUG
//                Serial.print("Sent Motor ");
//                Serial.print(motors_all[count]);
//                Serial.print(" Speed ");
//                Serial.print(motor_speed[count]);
//                Serial.print (" and Slave State ");
//                Serial.println(slave_state[count]);
//            #endif
                   count ++;  
  boolean bReceived;   
  while (bReceived = Receive(oSerialHover,oHoverFeedback))
  {
    DEBUGT("millis",iNow-iLast);
    DEBUGT("iSpeed",iSpeed);
    //DEBUGT("iSteer",iSteer);
    HoverLog(oHoverFeedback);
    iLast = iNow;
  }

        
      }


      iNext = iNow + SEND_MILLIS/2;
    #else
      //if (bReceived)  // Reply only when you receive data
        HoverSend(oSerialHover,iSteer,iSpeed,wState,wState);
      
      iNext = iNow + SEND_MILLIS;
    #endif
  }


}
