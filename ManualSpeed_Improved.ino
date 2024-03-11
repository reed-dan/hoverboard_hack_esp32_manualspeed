// must be before include (ask how I knwo!)
#define REMOTE_UARTBUS
#define _DEBUG // debug output to first hardware serial port
#define DEBUG_RX

// Includes
#include "util.h"
#include "hoverserial.h"


// Variable Declarations
const size_t numMotors = 1;      // Number of motors/slaves connected
const size_t numRightMotors = 1; // Number of right motors/slaves connected
const size_t numLeftMotors = 1;  // Number of left motors/slaves connected
int motorIds[numMotors] = {1};   // Motor IDs
int motorIdsRight[numRightMotors] = {1}; // Motor IDs for right motors
int motorIdsLeft[numLeftMotors] = {1};   // Motor IDs for left motors
int slaveCurrentSpeed[numMotors] = {
    0}; // Current speeds of motors (initialized to 0)
int slaveDesiredSpeed[numMotors] = {0};                         // Desired speeds of motors (initialized to 0)
const size_t speedIncrement = 10; // Increment speed for gradual acceleration
int sendSpeed = 0; // The speed sent to the slave/motor, typically equals
                   // desiredSpeed unless desired != current
int slaveDesiredState[numMotors] = {
    1}; // Desired states of slaves (initialized to 1)
int slaveCurrentState[numMotors] = {
    0}; // Current states of slaves (initialized to 0)
int slaveCurrentVolt[numMotors] = {0};      // Current volts of slaves
int slaveCurrentAmp[numMotors] = {0};       // Current amps of slaves
int slaveCurrentHallSteps[numMotors] = {0}; // Current hall steps of motors
int count = 0;

// Constants
#define debugSerialBaud 115200 // Baud Rate for ESP32 to computer serial comm
#define serialHoverBaud 19200  // Baud Rate for ESP32 to slave comm
#define serialHoverRxPin 16    // RX pin for ESP32 to slave serial comm
#define serialHoverTxPin 17    // TX pin for ESP32 to slave serial comm
#define serialHoverTimeout                                                     \
  1000 // Timeout period in milliseconds for response from hover
#define oSerialHover Serial2 // ESP32 Serial object for communication with hover
  SerialHover2Server oHoverFeedback;

// Control Method
#define input_serial

// Debug flag
#define DEBUGx

void setup() {
  // put your setup code here, to run once:
  Serial.begin(debugSerialBaud);
  Serial.println("Hello DanBot Hoverbaord controler");
  SerialHover2Server oHoverFeedback;
  HoverSetupEsp32(oSerialHover, serialHoverBaud, serialHoverRxPin,
                  serialHoverTxPin);
  HoverSend(oSerialHover, 1, 300, 1); // send command
}

void loop() {
  // put your main code here, to run repeatedly:

// input processing
#ifdef input_serial
  // read serial input
  if (Serial.available()) { // if there is data comming
    String command =
        Serial.readStringUntil('\n'); // read string until newline character
    serialInput(command);
  }
#endif

  // Send commands to hoverboard
  count = 0;
  while (count <
         numMotors) { // loop through the motor/slaves sending command for each
    HoverSend(oSerialHover,
              motorIds[findPosition(motorIds, numMotors, motorIds[count])],
              smoothAdjustment(count),
              slaveDesiredState[count]); // send command
#ifdef DEBUGx
    Serial.print("Sent Motor ");
    Serial.print(motorIds[count]);
    Serial.print(" Speed ");
    Serial.print(slaveDesiredSpeed[count]);
    Serial.print(" and Slave State ");
    Serial.println(slaveDesiredState[count]);
#endif

    count++;

    // Wait for response
    if (waitForResponse()) {
// Response received, process it
#ifdef DEBUGx
      Serial.println("Response received!");
#endif
      count++; // increase cound
    } else {
// No response received within timeout, send next command
#ifdef DEBUGx
      Serial.println("Timeout! Sending next command...");
#endif
      count++; // increase count
    }
    // Repeat the process for other commands...
  }
}

// wait for a repsonse to sent command
bool waitForResponse() {
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
            //Serial.println(oHoverFeedback);
#endif
            HoverLog(oHoverFeedback);
            return true;
        }
    }
    // Timeout reached, no response received
    return false;

// adjust speed uniformly
int smoothAdjustment(int count) {
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

  if (slaveCurrentSpeed[count] !=
      slaveDesiredSpeed[count]) { // if current speed is not desired speed
    if (slaveDesiredSpeed[count] >
        slaveCurrentSpeed[count]) { // if desired speed is greater than current
                                    // increase speed
      sendSpeed = slaveCurrentSpeed[count] +
                  speedIncrement;           // increase speed by speedIncrement
      return sendSpeed;                     // output the speed
      slaveCurrentSpeed[count] = sendSpeed; // set current speed to send speed
    } else if (slaveDesiredSpeed[count] <
               slaveCurrentSpeed[count]) { // if desired speed is less than
                                           // current decrease speed
      sendSpeed = slaveCurrentSpeed[count] -
                  speedIncrement;           // decrease speed by speedIncrement
      return sendSpeed;                     // output the speed
      slaveCurrentSpeed[count] = sendSpeed; // set current speed to send speed
    }
  } else { // if current speed is equal to desired speed
    sendSpeed = slaveDesiredSpeed[count]; // send desired speed
    return sendSpeed;                     // output the speed
  }
      return sendSpeed;                     // output the speed
}

// find the postion of a value in an array
int findPosition(int array[], int size, int value) {
  /*
The `findPosition` function is a utility function used to find the position of a
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
    count++;
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
      // Similar implementation for left motors
    } else if (command.startsWith("right|")) {
      // Handle right motors
      // Similar implementation for right motors
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
