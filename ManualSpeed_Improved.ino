// Control Method
#define input_serial //input commands via serial terminal
//#define input_adc //input commands via potentialometers
  #ifdef input_adc
      //set both pins the same if you only have 1 adc
    int adc_input_pin_right = 36; //analog input pin for the right potentiometer
    int adc_input_pin_left = 37; //analog input pin for the left potentiometer
    int adc_deadband = 100; //this the the range in the middle that will output zero
  #endif
#define wifi_control
 


// must be before include (ask how I know!)
#define REMOTE_UARTBUS
#define _DEBUG // debug output to first hardware serial port
//#define DEBUG_RX

// Includes
#include "util.h"
#include "hoverserial.h"
#ifdef wifi_control
	#include <WiFi.h>
	#include <WebServer.h>
	#include <WiFiManager.h> // Include WiFiManager library
  #include <ArduinoJson.h>
#endif

  #ifdef wifi_control
	WebServer server(80);
	WiFiManager wifiManager;
  #endif 

// Variable Declarations
//settings
const size_t numMotors = 1;      // Number of motors/slaves connected
const size_t numRightMotors = 1; // Number of right motors/slaves connected
const size_t numLeftMotors = 1;  // Number of left motors/slaves connected
int motorIds[numMotors] = {1};   // Motor IDs
int motorIdsRight[numRightMotors] = {1}; // Motor IDs for right motors
int motorIdsLeft[numLeftMotors] = {1};   // Motor IDs for left motors
const size_t speedIncrement = 10; // Increment speed for gradual acceleration
int minSpeed = -1000; //the min speed that is avialable
int maxSpeed = 1000; //The max speed that is avialable
int minStartSpeed = 300; //the minimum speed the motor needs to spin (otherwise we always read current speed 0 and never get anywhere)
int wifiControlIncrement = 100; //the increase or decrease in speed when pressing forward or backwards
int wifiControlTurn = 50; //the amount the motors on the direction you want to turn will be decreased


//don't change these
int slaveCurrentSpeed[numMotors] = {0}; // Current speeds of motors (initialized to 0)
int slaveSendSpeed[numMotors] = {0}; // Current speeds of motors (initialized to 0)
int slaveDesiredSpeed[numMotors] = {0}; // Desired speeds of motors (initialized to 0)
int slaveDesiredState[numMotors] = {1}; // Desired states of slaves (initialized to 1)
int slaveCurrentState[numMotors] = {0}; // Current states of slaves (initialized to 0)
int slaveCurrentVolt[numMotors] = {0};      // Current volts of slaves
int slaveCurrentAmp[numMotors] = {0};       // Current amps of slaves
int slaveCurrentOdometer[numMotors] = {0}; // Current hall steps of motors
int count = 0;
int speed;
//int sendSpeed = 0; // The speed sent to the slave/motor, typically equals desiredSpeed unless desired != current


// Debug flag
#define DEBUGx
//#define DEBUGsmooth
//#define debug_adcinput
//#define debug_updateArrays

// Constants
#define debugSerialBaud 115200 // Baud Rate for ESP32 to computer serial comm
#define serialHoverBaud 19200  // Baud Rate for ESP32 to slave comm
#define serialHoverRxPin 16    // RX pin for ESP32 to slave serial comm
#define serialHoverTxPin 17    // TX pin for ESP32 to slave serial comm
#define serialHoverTimeout                                                     \
  1000 // Timeout period in milliseconds for response from hover
#define oSerialHover Serial2 // ESP32 Serial object for communication with hover
  SerialHover2Server oHoverFeedback;





//
void setup() {
  // put your setup code here, to run once:
  Serial.begin(debugSerialBaud);
  Serial.println("Hello DanBot Hoverbaord controler");
  SerialHover2Server oHoverFeedback;
  HoverSetupEsp32(oSerialHover, serialHoverBaud, serialHoverRxPin,
                  serialHoverTxPin);
#ifdef wifi_control
  // Set up WiFi network in AP mode by default
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32AP");

  // Print IP address
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); // Print AP IP address

  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/mode", HTTP_POST, handleMode);
  server.on("/control", HTTP_POST, handleControl); // Endpoint for receiving control commands

  server.begin();
#endif

}

void loop() {
  // put your main code here, to run repeatedly:
// input processing
//
//serial
#ifdef input_serial
  // read serial input
  if (Serial.available()) { // if there is data comming
    String command =
        Serial.readStringUntil('\n'); // read string until newline character
    serialInput(command);
  }
#endif
//adc
#ifdef input_adc
ADCInput();
#endif

//
  // Send commands to hoverboard
  count = 0;
  while (count < numMotors) { // loop through the motor/slaves sending command for each
    HoverSend(oSerialHover, motorIds[findPosition(motorIds, numMotors, motorIds[count])], smoothAdjustment(count), slaveDesiredState[count]); // send command
//    HoverSend(oSerialHover, motorIds[findPosition(motorIds, numMotors, motorIds[count])], slaveDesiredSpeed[count], slaveDesiredState[count]); // send command

#ifdef DEBUGx
    Serial.print("Sent Motor ");
    Serial.print(motorIds[count]);
    Serial.print(" Speed ");
    Serial.print(slaveSendSpeed[count]);
    Serial.print(" and Slave State ");
    Serial.println(slaveDesiredState[count]);
#endif

    count++;

    // Wait for response
    if (waitForResponse()) {
// Response received, process it
#ifdef DEBUGx
      //Serial.println("Response received!");
#endif
      count++; // increase cound
    } else {
// No response received within timeout, send next command
#ifdef DEBUGx
      //Serial.println("Timeout! Sending next command...");
#endif
      count++; // increase count
    }
    // Repeat the process for other commands...
  }
#ifdef wifi_control
  server.handleClient();
#endif
}

// wait for a repsonse to sent command
bool waitForResponse() {
 //Description
     /*
    The `waitForResponse` function is a utility function used to wait for a response
    from a serial communication port within a specified timeout period. It is
    commonly used in scenarios where the program needs to wait for a response from a
    peripheral device, such as a motor controller or sensor, after sending a
    command.

    Here's how the function works:
    1. Timing: The function records the start time using the `millis` function,
    which returns the number of milliseconds since the Arduino board began running
    the current program.
    2. Loop: It enters a loop that continues until either a response is received or
    the specified timeout period elapses.
    3. Data Availability: Within the loop, it checks whether data is available to
    read from the serial port using the `available` function.
    4. Response Received: If data is available, it indicates that a response has
    been received, and the function discards any available data from the serial port
    buffer.
    5. Timeout: If no response is received within the specified timeout period, the
    function exits the loop and returns `false` to indicate a timeout.

    Let's illustrate this with an example:
    Suppose we send a command to a motor controller using serial communication and
    need to wait for a response.
    1. Timing: The function records the start time.
    2. Loop: It enters a loop and checks for data availability.
    3. Data Availability: If data becomes available (i.e., a response is received),
    the function exits the loop and returns `true`.
    4. Timeout: If no response is received within the timeout period, the function
    exits the loop and returns `false`.

    This function ensures that the program does not send commands to fast/hang
    indefinitely while waiting for a response, thereby providing a mechanism to
    handle communication timeouts effectively.
    */

    unsigned long startTime = millis(); // Record the start time
    while (millis() - startTime < serialHoverTimeout) {
        boolean bReceived;   
        if (Receive(oSerialHover, oHoverFeedback) != 0 || (bReceived = Receive(oSerialHover, oHoverFeedback))) {
#ifdef DEBUGx
            // Serial.println(oHoverFeedback);
  #endif
      // Print hover response
      HoverLog(oHoverFeedback);  
      // Update arrays with the new data
      updateArrays(oHoverFeedback);
              return true;
          }
      }
      // Timeout reached, no response received
      return false;
}

// adjust speed uniformly
int smoothAdjustment(int count) {
   //Description   
   /*
    The `smoothAdjustment` function is designed to gradually adjust the speed of
    motors from their current speed to the desired speed. It ensures that the speed
    transition is smooth and avoids sudden changes, which can be jarring or cause
    instability, especially in motorized systems like hoverboards.

    Here's how the function works:
    1. Comparison: It first compares the current speed of the motor with the desired
    speed.
    2. Increment/Decrement: If the desired speed is higher than the current speed,
    it increases the speed by a predefined increment value (speedIncrement).
    Conversely, if the desired speed is lower, it decreases the speed by the same
    increment.
    3. Return: The function returns the adjusted speed value.

    Let's illustrate this with an example:
    Suppose the current speed of a motor is 40, and the desired speed is 80. Let's
    assume speedIncrement is set to 10.

    1. Comparison: Current speed (40) is less than desired speed (80).
    2. Increment: It increases the speed by the increment value (10). So, the new
    speed becomes 50.
    3. Return: The function returns the adjusted speed, which is 50.

    The process continues until the current speed matches the desired speed. If the
    desired speed is lower than the current speed, the function decrements the speed
    until they match. If they're already equal, it returns the current speed.

    This gradual adjustment helps in achieving smoother acceleration or deceleration
    of motors, which is beneficial for stability and user experience, especially in
    dynamic systems like hoverboards.
    */

  while (slaveSendSpeed[count] != slaveDesiredSpeed[count]) { // if current speed is not desired speed
      #ifdef DEBUGsmooth
      Serial.print("SmoothAdjustment: Sent Speed is not equal to Desired Speed. Sent Speed:  ");
      Serial.print(slaveSendSpeed[count]);
      Serial.print(" & Desired Speed: ");
      Serial.println(slaveDesiredSpeed[count]);
      #endif
   
    if (slaveDesiredSpeed[count] > slaveSendSpeed[count]) { // if desired speed is greater than current
      #ifdef DEBUGsmooth
      Serial.print("SmoothAdjustment: Sent Speed is less than to Desired Speed increase speed. Sent Speed:  ");
      Serial.print(slaveSendSpeed[count]);
      Serial.print(" & Desired Speed: ");
      Serial.println(slaveDesiredSpeed[count]);
      #endif
                                    // increase speed
                                    Serial.print("SendSpeed =");
                                    Serial.print(slaveSendSpeed[count]);
                                    Serial.println(" increment");
      slaveSendSpeed[count] = slaveSendSpeed[count] + speedIncrement;  
                                          Serial.print("SendSpeed now =");
                                    Serial.println(slaveSendSpeed[count]);
      return slaveSendSpeed[count];                     // output the speed

    } else if (slaveDesiredSpeed[count] < slaveSendSpeed[count]) { // if desired speed is less than
                                           // current decrease speed
                                                 #ifdef DEBUGsmooth
      Serial.print("SmoothAdjustment: Current Speed is greater than Sent Speed.  Decrease speed. Sent Speed:  ");
      Serial.print(slaveSendSpeed[count]);
      Serial.print(" & Desired Speed: ");
      Serial.println(slaveDesiredSpeed[count]);
      #endif
      slaveSendSpeed[count] = slaveSendSpeed[count] - speedIncrement;           // decrease speed by speedIncrement
      return slaveSendSpeed[count];                     // output the speed
    }
  } 
   // if current speed is equal to desired speed
          #ifdef DEBUGsmooth
      Serial.print("SmoothAdjustment: Sent Speed is equal to Desired Speed. Send desired speed. Sent Speed:  ");
      Serial.print(slaveSendSpeed[count]);
      Serial.print(" & Desired Speed: ");
      Serial.println(slaveDesiredSpeed[count]);
      #endif
    return slaveSendSpeed[count];                     // output the speed
}

// find the postion of a value in an array
int findPosition(int array[], int size, int value) {
  //Description
    /*The `findPosition` function is a utility function used to find the position of a
    specific value within an array. It is commonly used to locate the index of a
    particular motor ID within an array of motor IDs.

    Here's how the function works:
    1. Loop: It iterates through each element of the array.
    2. Comparison: For each element, it compares the value with the target value.
    3. Match: If a match is found, it returns the index (position) of that element
    in the array.
    4. No Match: If no match is found after iterating through the entire array, it
    returns -1 to indicate that the value was not found.

    Let's illustrate this with an example:
    Suppose we have an array of motor IDs: {1, 2, 3, 4, 5}.
    We want to find the position of motor ID 4 within this array.

    1. Loop: The function iterates through each element of the array.
    2. Comparison: It compares each element with the target value (4).
      - Index 0: Element is 1, not a match.
      - Index 1: Element is 2, not a match.
      - Index 2: Element is 3, not a match.
      - Index 3: Element is 4, match found.
    3. Match: It returns the index 3, indicating that motor ID 4 is found at
    position 3 in the array.

    If the target value is not present in the array, the function returns -1. For
    example, if we search for motor ID 6 in the same array, the function would
    return -1, indicating that the value was not found.

    This function is useful for tasks that involve searching for specific elements
    within arrays, such as locating motor IDs, sensor values, or any other data
    stored in array format.
    */

  for (int i = 0; i < size; ++i) {
    if (array[i] == value) {
      return i; // Return the position if the value is found
    }
  }
  return -1; // Return -1 if the value is not found in the array
}

// set motor speeds
void setDesiredMotorSpeed(int motorIds[], int motorSpeeds[], int numMotors,
                          int desiredSpeed, String motorType) {
  /*
  The function starts by checking the motorType parameter to determine which
  motors' speeds to set. If motorType is "all," it sets the desired speed for
  all motors in the array. If it's "right" or "left," it sets the speed for
  right or left motors accordingly. Finally, if motorType is a specific motor
  ID, it sets the desired speed for that motor. The function utilizes the
  findPosition function to locate the position of motors in the motorIds array
  and sets the desired speed accordingly. Examples of using the function are
  provided as comments at the top of the function.

  // Example of setting desired speed 100 for all motors
  //setdesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, 100, "all");

  // Example of setting desired speed 50 for right motors
  //setdesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, 50, "right");

  // Example of setting desired speed 75 for left motors
  //setdesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, 75, "left");

  // Example of setting desired speed 90 for a specific motor (e.g., motor with
  ID 2)
  //setdesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, 90, "2");
  */

  int motorIndex; // Variable to store the found position of the motor in the
                  // motorIds array
  if (motorType == "all") {
    // Set the desired speed for all motors
    for (int i = 0; i < numMotors; ++i) {
      slaveDesiredSpeed[i] = desiredSpeed;
    }
  } else if (motorType == "right") {
    // Set the desired speed for right motors
    for (int i = 0; i < numRightMotors; ++i) {
      motorIndex = findPosition(
          motorIds, numMotors,
          motorIdsRight[i]); // Find the position of motorIdsRight[i] in the
                             // motorsIds array
      if (motorIndex != -1) {
        slaveDesiredSpeed[motorIndex] = desiredSpeed;
      }
    }
  } else if (motorType == "left") {
    // Set the desired speed for left motors
    for (int i = 0; i < numLeftMotors; ++i) {
      motorIndex =
          findPosition(motorIds, numMotors,
                       motorIdsLeft[i]); // Find the position of motorIdsLeft[i]
                                         // in the motorsIds array
      if (motorIndex != -1) {
        slaveDesiredSpeed[motorIndex] = desiredSpeed;
      }
    }
  } else {
    // Set the desired speed for a specific motor
    motorIndex = findPosition(
        motorIds, numMotors,
        motorType.toInt()); // Find the position of the specific motor by number
                            // in the motorsIds array
    if (motorIndex != -1) {
      slaveDesiredSpeed[motorIndex] = desiredSpeed;
    }
  }
}

// stop motors
void emergencyStop() {
  /*
 This function implements an emergency stop by sending a command to set the
speed of all motors to 0 without smoothing. The emergencyStop function is
designed to halt all motors in case of an emergency. It achieves this by sending
a command to set the speed of each motor to 0 without any smoothing or waiting
for responses. The function loops through all motors using a while loop and
sends a command to set the speed to 0 for each motor using the HoverSend
function. It ensures that all motors come to an immediate stop, which is crucial
in emergency situations to prevent any further movement or potential harm.
*/

  // Loop through all motors and send a command to set their speed to 0
  count = 0;
  while (count < numMotors) {
    // Send command to set speed to 0 for each motor
    HoverSend(oSerialHover, motorIds[count], 0, slaveDesiredState[count]);
    slaveDesiredSpeed[count]=0;
    slaveSendSpeed[count]=0;
    count++;
  }
  
}
void updateArrays(const SerialHover2Server& oHoverFeedback) {
    // Find the position of iSlave in the motorIds array
    int slavePosition = findPosition(motorIds, numMotors, oHoverFeedback.iSlave);
    
    // If the iSlave is found in the motorIds array
    if (slavePosition != -1) {

        // Update the arrays with the data from oHoverFeedback
        slaveCurrentSpeed[slavePosition] = (oHoverFeedback.iSpeed * 10);
        slaveCurrentAmp[slavePosition] = oHoverFeedback.iAmp;
        slaveCurrentVolt[slavePosition] = oHoverFeedback.iVolt;
        
        #ifdef debug_updateArrays
        Serial.print("Slave position ");
        Serial.println(slavePosition);
        // Print debug information
        Serial.print("Updated data for iSlave ");
        Serial.println(oHoverFeedback.iSlave);
        Serial.print("Current Speed: ");
        Serial.print(oHoverFeedback.iSpeed);
        Serial.print(" array output after update ");
        Serial.println(slaveCurrentSpeed[slavePosition]);
        Serial.print("Current Amp: ");
        Serial.print(oHoverFeedback.iAmp);
        Serial.print(" array output after update ");
        Serial.println(slaveCurrentAmp[slavePosition]);
        Serial.print("Current Volt: ");
        Serial.print(oHoverFeedback.iVolt);
        Serial.print(" array output after update ");
        Serial.println(slaveCurrentVolt[slavePosition]);
        #endif
    } else {
        // If iSlave is not found in the motorIds array
        Serial.println("Error: iSlave not found in motorIds array");
    }
}

// input functions
void serialInput(String command) {
  // Check if input starts with "stop"
  if (command.startsWith("stop")) {
    // Trigger emergency stop
    emergencyStop();
    return;
  }

  // Check if input starts with "hover|"
  if (command.startsWith("hover|")) {
    // Remove "hover|" from the command
    command.remove(0, 6);

    // Parse the command based on the specified motor or all motors
    if (command.startsWith("all|")) {
      // Handle all motors
      command.remove(0, 4);
      int speed, state;
      // Parse speed and state from the command
      int numParsed = sscanf(command.c_str(), "%d|%d", &speed, &state);
      if (numParsed == 2) {
        // Set speed and state for all motors
        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "all");
      } else {
        Serial.println("Invalid command format");
      }
    } else if (command.startsWith("left|")) {
// Handle left motors
      command.remove(0, 5);
      int speed, state;
      // Parse speed and state from the command
      int numParsed = sscanf(command.c_str(), "%d|%d", &speed, &state);
      if (numParsed == 2) {
        // Set speed and state for all motors
        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "left");
      } else {
        Serial.println("Invalid command format");
      }
    } else if (command.startsWith("right|")) {
// Handle right motors
      command.remove(0, 6);
      int speed, state;
      // Parse speed and state from the command
      int numParsed = sscanf(command.c_str(), "%d|%d", &speed, &state);
      if (numParsed == 2) {
        // Set speed and state for all motors
        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "right");
      } else {
        Serial.println("Invalid command format");
      }
    } else {
      // Specific motor
      int motorId, speed, state;
      // Parse motor ID, speed, and state from the command
      int numParsed =
          sscanf(command.c_str(), "%d|%d|%d", &motorId, &speed, &state);
      if (numParsed == 3) {
        // Set speed and state for the specified motor
        if (motorId >= 0 && motorId < numMotors) {
          setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                               String(motorId));
        } else {
          Serial.println("Invalid motor ID");
        }
      } else {
        Serial.println("Invalid command format");
      }
    }
  } else {
    Serial.println("Invalid command");
  }
}

// Function to read input from ADC (potentiometers) and set desired speed for motors
#ifdef input_adc
void ADCInput() {
    //Description
      /*  
        This function reads the analog input from both the right and left potentiometers, maps the input values to the 
        desired speed range (specified by minSpeed and maxSpeed), applies a deadband to eliminate small variations around 
        the center position, and then sets the desired speed for the right and left motors accordingly using the 
        setDesiredMotorSpeed function.
      */
    // Read analog input from right potentiometer
    int rightPotValue = analogRead(adc_input_pin_right);
    
    // Read analog input from left potentiometer
    int leftPotValue = analogRead(adc_input_pin_left);
    
    // Convert analog input values to desired speed within the specified range
    int desiredSpeedRight = map(rightPotValue, 0, 4095, minSpeed, maxSpeed);
    int desiredSpeedLeft = map(leftPotValue, 0, 4095, minSpeed, maxSpeed);
    
    // Apply deadband
    if (abs(desiredSpeedRight) < adc_deadband) {
        desiredSpeedRight = 0;
    }
    if (abs(desiredSpeedLeft) < adc_deadband) {
        desiredSpeedLeft = 0;
    }
    
    // Set desired speed for right motors
    setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, desiredSpeedRight, "right");
    
    // Set desired speed for left motors
    setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, desiredSpeedLeft, "left");

#ifdef debug_adcinput
    // Debugging steps
    Serial.print("Right Potentiometer Value: ");
    Serial.println(rightPotValue);
    Serial.print("Left Potentiometer Value: ");
    Serial.println(leftPotValue);
    Serial.print("Desired Speed Right: ");
    Serial.println(desiredSpeedRight);
    Serial.print("Desired Speed Left: ");
    Serial.println(desiredSpeedLeft);
#endif
}
#endif

//wifi and webserver
#ifdef wifi_control
	void handleRoot() {
	  String page = "<html><head><title>ESP32 Control</title></head><body>"
					"<h1>ESP32 Control</h1>"
					"<div onclick='sendCommand(\"forward\")' style='cursor: pointer; padding: 10px; background-color: #4CAF50; color: white; margin-bottom: 10px;'>Forward</div>"
					"<div onclick='sendCommand(\"backward\")' style='cursor: pointer; padding: 10px; background-color: #2196F3; color: white; margin-bottom: 10px;'>Backward</div>"
					"<div onclick='sendCommand(\"right\")' style='cursor: pointer; padding: 10px; background-color: #f44336; color: white; margin-bottom: 10px;'>Right</div>"
					"<div onclick='sendCommand(\"left\")' style='cursor: pointer; padding: 10px; background-color: #FFA500; color: white; margin-bottom: 10px;'>Left</div>"
					"<div onclick='sendCommand(\"sync\")' style='cursor: pointer; padding: 10px; background-color: #808080; color: white; margin-bottom: 10px;'>Sync</div>"
					"<div onclick='sendCommand(\"stop\")' style='cursor: pointer; padding: 10px; background-color: #FF0000; color: white;'>Stop</div>"
					"<br>"
					"<a href='/settings'>Settings</a>"
					"<script>"
					"function sendCommand(command) {"
					"  fetch('/control', {"
					"    method: 'POST',"
					"    headers: { 'Content-Type': 'application/json' },"
					"    body: JSON.stringify({ command: command })"
					"  });"
					"}"
					"</script>"
					"</body></html>";

	  server.send(200, "text/html", page);
	}

	void handleSettings() {
	  String page = "<html><head><title>ESP32 Settings</title></head><body>"
					"<h1>ESP32 Settings</h1>"
					"<p>WiFi Mode: ";
	  page += (WiFi.getMode() == WIFI_AP ? "Access Point (AP)" : "Station (STA)");
	  page += "</p><p>IP Address: ";
	  page += (WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
	  page += "</p><div onclick='switchMode(\"ap\")' style='cursor: pointer; padding: 10px; background-color: #4CAF50; color: white; margin-bottom: 10px;'>Switch to AP mode</div>"
			  "<div onclick='switchMode(\"config\")' style='cursor: pointer; padding: 10px; background-color: #2196F3; color: white; margin-bottom: 10px;'>WiFi Manager Config</div>"
			  "<div onclick='switchMode(\"sta\")' style='cursor: pointer; padding: 10px; background-color: #f44336; color: white;'>Switch to STA mode</div>"
			  "<a href='/'>Control</a>"
			  "<script>"
			  "function switchMode(mode) {"
			  "  fetch('/mode', {"
			  "    method: 'POST',"
			  "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
			  "    body: 'mode=' + mode"
			  "  }).then(() => location.reload());"
			  "}"
			  "</script>"
			  "</body></html>";

	  server.send(200, "text/html", page);
	}

	void handleMode() {
	  String mode = server.arg("mode");
	  server.stop(); // Stop the web server before switching modes
	  if (mode == "ap") {
		WiFi.disconnect(true); // Disconnect from current network before switching to AP mode
		WiFi.mode(WIFI_AP);
		delay(1000); // Delay to ensure WiFi mode change
		wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
		server.begin(); // Restart the web server after configuring the Access Point
	  } else if (mode == "sta") {
		WiFi.disconnect(true); // Disconnect from current network before switching to STA mode
		WiFi.mode(WIFI_STA);
		delay(1000); // Delay to ensure WiFi mode change
		if (!wifiManager.autoConnect("ESP32AP")) {
		  Serial.println("Failed to connect and hit timeout");
		  // Handle failure to connect to WiFi
		  // For example, retry connection or switch back to AP mode
		}
		server.begin(); // Restart the web server after configuring Station mode
	  } else if (mode == "config") {
		WiFi.disconnect(true); // Disconnect from current network before switching to AP mode
		// WiFi.mode(WIFI_AP);
		delay(1000); // Delay to ensure WiFi mode change
		WiFiManager wm;

		//reset settings - for testing
		//wm.resetSettings();

		// set configportal timeout
		wm.setConfigPortalTimeout(120);

		if (!wm.startConfigPortal("OnDemandAP")) {
		  Serial.println("failed to connect and hit timeout");
		  delay(3000);
		  //reset and try again, or maybe put it to deep sleep
		  ESP.restart();
		  delay(5000);
		}

		//if you get here you have connected to the WiFi
		Serial.println("connected...yeey :)");
	  }
	  handleSettings();
	}

	void handleControl() {
		 // Handle control commands sent from the web page
  if (server.hasArg("plain")) {
    String jsonCommand = server.arg("plain");
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    const char* command = doc["command"];

    // Process the command here
    if (strcmp(command, "forward") == 0) {
      // Perform action for forward command
      Serial.println("Moving forward...");
	  			Serial.println("forward process started");
					//sync the motors/slaves
					speed = slaveDesiredSpeed[0];//retrieve the speed of the first motor
					setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "all"); //set all of the motors to said speed
					//increment speed			
					speed = speed + wifiControlIncrement;
					//check is speed is less than max
					if (speed < maxSpeed ){
					//increase the speed by the increment
			        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "all");
					}
					else if (speed >= maxSpeed)	{
											if (speed < maxSpeed ){
					//send maxspeed
			        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, maxSpeed,
                             "all");
					}
					}
    } else if (strcmp(command, "backward") == 0) {
      // Perform action for backward command
      Serial.println("Moving backward...");
	  //sync the motors/slaves
					speed = slaveDesiredSpeed[0];//retrieve the speed of the first motor
					setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "all"); //set all of the motors to said speed
					//decrement speed			
					speed = speed - wifiControlIncrement;
					//check is speed is less than min
					if (speed > minSpeed ){
					//decrease the speed by the decrement
			        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "all");
					}
					else if (speed <= minSpeed)	{
										
					//send minspeed
			        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, minSpeed,
                             "all");
					}
    } else if (strcmp(command, "right") == 0) {
      // Perform action for right command
      Serial.println("Turning right...");
	  //sync the motors/slaves
					speed = slaveDesiredSpeed[0];//retrieve the speed of the first motor
					Serial.println(speed);
					setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "all"); //set all of the motors to said speed
					//decrement speed by wifiControlTurn			
					speed = speed - wifiControlTurn;
										Serial.print("new speed right ");
					Serial.println(speed);
					//send decremented speed to right side
			        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "right");							 
//note for self ifgure out how to handle turns in reverse and turns at max min speed
		
    } else if (strcmp(command, "left") == 0) {
      // Perform action for left command
      Serial.println("Turning left...");
	  //sync the motors/slaves
					speed = slaveDesiredSpeed[0];//retrieve the speed of the first motor
					Serial.println(speed);
					setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "all"); //set all of the motors to said speed
					//decrement speed by wifiControlTurn			
					speed = speed - wifiControlTurn;
					Serial.print("new speed left ");
					Serial.println(speed);
					//send decremented speed to left side
			        setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "left");							 
//note for self ifgure out how to handle turns in reverse and turns at max min speed
		
    } else if (strcmp(command, "sync") == 0) {
      // Perform action for sync command
      Serial.println("Syncing...");
	  //sync the motors/slaves
					speed = slaveDesiredSpeed[0];//retrieve the speed of the first motor
					setDesiredMotorSpeed(motorIds, slaveDesiredSpeed, numMotors, speed,
                             "all"); //set all of the motors to said speed

    } else if (strcmp(command, "stop") == 0) {
      // Perform action for stop command
      Serial.println("Stopping...");
	  emergencyStop();
    } else {
      // Unknown command
      Serial.println("Unknown command");
    }

    // Send response back to client
    server.send(200, "application/json", "{\"status\":\"OK\",\"command\":\"" + String(command) + "\"}");
  } else {
    // No command received
    Serial.println("No command received.");
    server.send(400, "application/json", "{\"error\":\"No command received\"}");
  }
		
	  }


#endif
