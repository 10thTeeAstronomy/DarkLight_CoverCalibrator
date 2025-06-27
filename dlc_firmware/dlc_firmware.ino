/*
  Name:         DarkLight Cover Calibrator (DLC)
  Author:       Nathan Woelfle
  Contributors: Taylor J (Initial Dew Heater Integration)
  Date:         6/2/25

  Version: 1.1.1
  *View ReadMe for version change details
  
  Description: DarkLight is a fully ASCOM supported and compliant
              device for use with ICoverCalibratorV1 driver and INDI platform.

              https://sourceforge.net/projects/darklight-cover-calibrator/

  (c) Copyright Nathan Woelfle 2020-present day. All Rights Reserved.

  This project is protected by International Copyright Law.
  Permission is granted for personal and Academic/Educational use only.
  Software and hardware distributed under Creative Commons Attribution-NonCommercial License

  For more information, please refer to the full terms of the Creative Commons Attribution-NonCommercial 4.0 International License:
  https://creativecommons.org/licenses/by-nc/4.0/
*/

//-----------------------------------------------------------
//----- IF UNSURE HOW TO SETUP, SEE MANUAL FOR DETAILS  -----
//-----------------------------------------------------------

//----- (UA) USER-ADJUSTABLE OPTIONS ------
#define COVER_INSTALLED //comment out if not utilized
#define LIGHT_INSTALLED //comment out if not utilized
#define HEATER_INSTALLED //comment out if not utilized
#define ENABLE_SERIAL_CONTROL //comment out if not utilized
#define ENABLE_MANUAL_CONTROL //comment out if not utilized
#define ENABLE_SAVING_TO_MEMORY //comment out if not utilized
//#define SHOW_HEARTBEAT //for debugging purposes only. shows loop is active, uncomment to flash builtin led
const uint32_t serialSpeed = 115200; //values are: (9600, 19200, 38400, 57600, (default 115200), 230400)

//----- (UA) (COVER) -----
const uint32_t timeToMoveCover = 5000;  //(ms) time it takes to move between open/close positions (set between 1000(fast)-10000(slow) ms, recommend (5000 ms))

//----- (UA) (COVER) PRIMARY SERVO PARAMETERS -----
const uint16_t primaryServoMinPulseWidth = 500; //refer to servo manufacture for usec pulses and set accordingly
const uint16_t primaryServoMaxPulseWidth = 2500; //refer to servo manufacture for usec pulses and set accordingly
const uint8_t primaryServoOpenCoverAngle = 0; //position angle servo opens to, value between (0-180), *may need to be adjusted based on the type of servo used
const uint8_t primaryServoCloseCoverAngle = 180; //position angle servo closes to, value between (0-180), *may need to be adjusted based on the type of servo used

//----- (UA) (COVER) SECONDARY SERVO PARAMETERS -----
//#define SECONDARY_SERVO_INSTALLED //uncomment if using additional servo with different angles
const uint16_t secondaryServoMinPulseWidth = 500; //refer to servo manufacture for usec pulses and set accordingly
const uint16_t secondaryServoMaxPulseWidth = 2500; //refer to servo manufacture for usec pulses and set accordingly
const uint8_t secondaryServoOpenCoverAngle = 0; //position angle servo opens to, value between (0-180), *may need to be adjusted based on the type of servo used
const uint8_t secondaryServoCloseCoverAngle = 180; //position angle servo closes to, value between (0-180), *may need to be adjusted based on the type of servo used

//----- (UA) (COVER) SELECT A MOVEMENT -----
//----- UNCOMMENT ONLY ONE OPTION, SEE MANUAL FOR DETAILS -----
#define USE_LINEAR
//#define USE_CIRCULAR
//#define USE_CUBIC
//#define USE_EXPO
//#define USE_QUAD
//#define USE_QUART
//#define USE_QUINT
//#define USE_SINE

//----- (UA) (LIGHT) -----
uint8_t maxBrightness = 255; //choose one of the following max number of steps: (max # of levels:steps between each value) -> ((light will be on or off 1:255), default 5:51, 17:15, 51:5, 85:3, (default 255:1))
bool autoON = false; //adjust only if using manual-only mode, see manual for details 

//----- (UA) (HEATER) -----
const uint32_t heaterShutoff = 3600000; //(ms) max time for manual heating (3600000 = one hour)
const float deltaPoint = 5.0;  //(degrees) target temperature difference above ambient temp for dew control

//----- (UA) (HEATER) NUMBER OF HEATING MODULES -----
//----- UNCOMMENT UP TO TWO (2) HEATERS, SEE MANUAL FOR DETAILS -----
#define HEATER_ONE_INSTALLED
#define HEATER_TWO_INSTALLED

//----- (UA) (HEATER) TEMPERATURE & HUMIDTY SENSOR -----
//----- UNCOMMENT ONLY ONE OPTION, SEE MANUAL FOR DETAILS -----
#define ENABLE_BME280
//#define ENABLE_DHT22

//----- (UA) (BUTTONS) -----
const uint32_t debounceDelay = 150; //(ms) debounce time for the buttons *may need adjusting, see manual for details

//----- END OF (UA) USER-ADJUSTABLE OPTIONS -----
//-----------------------------------------------
//-------------- DO NOT EDIT BELOW --------------
//-----------------------------------------------
//------------ VARIABLE DECLARATION -------------

//----- VERSIONING CONTROL -----
const char* dlcVersion = "v1.1.1";

//----- MEMORY -----
#ifdef ENABLE_SAVING_TO_MEMORY
  #include <EEPROMWearLevel.h>
  #define EEPROM_LAYOUT_VERSION 1
  #define AMOUNT_OF_INDEXES 4
  #define EEPROM_LENGTH_TOUSE 1023
  #define SAVED_COVER_STATE 0
  #define SAVED_PANEL_VALUE 1
  #define SAVED_BROADBAND_VALUE 2
  #define SAVED_NARROWBAND_VALUE 3
#endif

//----- PIN ASSIGNMENT -----
const uint8_t lightPanel = 3;
const uint8_t chOneHeatTempSensor = 4;
const uint8_t chOneHeater = 5;
const uint8_t chTwoHeater = 6;
const uint8_t chTwoHeatTempSensor = 7;
const uint8_t dhtSensor = 8;
const uint8_t primeServo = 9;
const uint8_t secondServo = 10;
//PIN 13 RESERVED for LED_BUILTIN
const uint8_t servoButton = A1;
const uint8_t lightButton = A2;
//PIN A4 RESERVED for BME280 I2C -> SDA
//PIN A5 RESERVED for BME280 I2C -> SCL

//----- COMM FOR ASCOM / INDI -----
uint8_t currentCoverState; //reports # 0:NotPresent, 1:Closed, 2:Moving, 3:Open, 4:Unknown, 5:Error
uint8_t calibratorState = 1; //reports # 0:NotPresent, 1:Off, 2:NotReady, 3:Ready, 4:Unknown, 5:Error
uint8_t heaterState; //reports # 0:NotPresent, 1:Off, 3:On, 4:Unknown, 5:Error, 6:Set (HeatOnClose)

//----- SERIAL -----
#ifdef ENABLE_SERIAL_CONTROL
  const char startMarker = '<'; //signal to process serial command
  const char endMarker = '>'; //signal that serial command is finished
  const uint8_t maxNumChars = 10; //set max num of characters in array
  char receivedChars[maxNumChars]; //set array
  bool commandComplete = false; //flag to process command when end marker received
  char response[maxNumChars];
#endif

//----- COVER -----
#ifdef COVER_INSTALLED
  #include <dlcServo.h>
  uint8_t moveCoverTo; //1:Closed, 3:Open
  uint8_t previousMoveCoverTo; //holds previous start position of cover
  uint32_t startServoTimer; //holds start time for servo
  uint32_t elapsedMoveTime = 0; //holds time servo moved
  bool halt = false; //flag to stop servo movement
  uint32_t startDetachTimer; //holds start time
  bool detachServo = false; //flag to detach servo

  //primary servo
  dlcServo primaryServo; //create primary servo object
  uint16_t primaryServoLastPosition; //holds last servo write angle
  uint16_t primaryServoRemainingDistance; //used to calculate distance left of move
  
  #ifdef SECONDARY_SERVO_INSTALLED
    dlcServo secondaryServo; //create secondary servo object
    uint16_t secondaryServoLastPosition; //holds last servo write angle
    uint16_t secondaryServoRemainingDistance; //used to calculate distance left of move
  #endif

  //validation check: ensure only one option of servo movement is defined
  #if (defined(USE_LINEAR) + defined(USE_CIRCULAR) + defined(USE_CUBIC) + \
      defined(USE_EXPO) + defined(USE_QUAD) + defined(USE_QUART) + \
      defined(USE_QUINT) + defined(USE_SINE)) > 1
  #error "Multiple easing options are defined. Please uncomment only one option."
  #elif !(defined(USE_LINEAR) || defined(USE_CIRCULAR) || defined(USE_CUBIC) || \
        defined(USE_EXPO) || defined(USE_QUAD) || defined(USE_QUART) || \
        defined(USE_QUINT) || defined(USE_SINE))
    #define USE_LINEAR // Default to linear if none are defined
    #pragma message("Warning: No easing option defined. Defaulting to USE_LINEAR.")
  #endif
#endif

//----- LIGHT -----
#ifdef LIGHT_INSTALLED
  const uint8_t brightnessSteps = 255 / maxBrightness; //set steps size based on maxBrightness
  uint32_t startLightTimer; //holds start time for light
  uint32_t stabilizeTime; //set delay to give light time to settle
  uint8_t lightValue = 0; //lightValue received and adjusted for Arduino
  uint8_t broadbandValue; //holds saved EEPROM value
  uint8_t narrowbandValue; //holds saved EEPROM value
  uint8_t previousLightPanelValue; //holds last ON value
#endif

//----- MANUAL OPERATION -----
#ifdef ENABLE_MANUAL_CONTROL
  const uint32_t doublePressTime = 400;
  const uint32_t longPressTime = 1000;
  uint32_t lastServoButtonPressTime = 0;
  uint32_t lastLightButtonPressTime = 0;
  uint8_t lastServoButtonState = 1;
  uint8_t lastLightButtonState = 1;
#endif

//----- HEATER -----
#ifdef HEATER_INSTALLED
  #include <Wire.h>
  #include <OneWire.h>
  #include <DallasTemperature.h>
  bool autoHeat = false; //true if always on auto control
  bool manualHeat = false; //true if activated
  bool heatOnClose = false; //if true turns heater on after closing from open position
  bool heaterError = false; //flag turns true if sensor error occurs
  bool heaterUnknown = false; //flag
  uint8_t errorCounter = 0; //counter to track consecutive errors
  const uint8_t maxErrorCount = 5; //threshold for consecutive errors
  const float maxPWM = 255.0;    //max PWM value for heater control
  uint32_t previousDewMillis; //timing for dew control
  uint32_t startHeaterTimer;
  float outsideTemp, humidityLevel; //sensors to monitor outdoor environment

  //dew heater system constants - DO NOT MODIFY
  const float DEW_POINT_ALPHA = 17.27;    // August-Roche-Magnus dew point constant
  const float DEW_POINT_BETA = 237.7;     // August-Roche-Magnus dew point constant
  const float PWM_MAP_MULTIPLIER = 100.0; // PWM mapping scale factor
  const float PWM_MAP_RANGE = 500.0;      // PWM mapping range value

  #ifdef HEATER_ONE_INSTALLED
    OneWire oneWireA(chOneHeatTempSensor); //setup oneWire instance to communicate with sensor one
    DallasTemperature chOneSensor(&oneWireA);
    float heaterOneTemp; //hold heater temp
  #endif

  #ifdef HEATER_TWO_INSTALLED
    OneWire oneWireB(chTwoHeatTempSensor); //setup oneWire instance to communicate with sensor two
    DallasTemperature chTwoSensor(&oneWireB);
    float heaterTwoTemp; //hold heater temp
  #endif
  
  #ifdef ENABLE_BME280
    #include <Adafruit_BME280.h>
    Adafruit_BME280 bme; //create BME280 object
    const unsigned long dewInterval = 1000; //interval for dew control updates (1 second)
  #endif
  
  #ifdef ENABLE_DHT22
    #include <DHT.h>
    #define DHTTYPE DHT22 //dht sensor type
    DHT dht(dhtSensor, DHTTYPE); //assign to pin 8 and define the object
    const uint32_t dewInterval = 2000; //interval for dew control updates (2 second)
  #endif

  //validation check: ensure at least one heater is defined
  #if !(defined(HEATER_ONE_INSTALLED) + defined(HEATER_TWO_INSTALLED))
    #error "ERROR: No heaters has been defined"
  #endif
  
  //validation check: ensure only one temp & humidity sensor is defined
  #if (defined(ENABLE_BME280) + defined(ENABLE_DHT22)) > 1
    #error "Multiple temp sensor options are defined. Please uncomment only one option."
  #elif !(defined(ENABLE_BME280) + defined(ENABLE_DHT22))
    #error "ERROR: No temp sensor option defined."
  #endif
  
#endif //HEATER_INSTALLED

//---------------------------------------
//----- END OF VARIABLE DECLARATION -----
//---------------------------------------

void setup(){
  pinMode(lightPanel, OUTPUT);
  pinMode(primeServo, OUTPUT);
  pinMode(secondServo, OUTPUT);
  pinMode(chOneHeater, OUTPUT);
  pinMode(chTwoHeater, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(servoButton, INPUT_PULLUP); //enable internal pull-up resistor
  pinMode(lightButton, INPUT_PULLUP); //enable internal pull-up resistor

  #ifdef ENABLE_SAVING_TO_MEMORY
    EEPROMwl.begin(EEPROM_LAYOUT_VERSION, AMOUNT_OF_INDEXES, EEPROM_LENGTH_TOUSE);
  #endif
  
  initializeVariables();

  #if defined(ENABLE_SERIAL_CONTROL)
    initializeComms();
  #endif
}//end of setup

void loop(){
  
  #ifdef ENABLE_SERIAL_CONTROL
    checkSerial();

    if (commandComplete) {
      processCommand();
    }
  #endif

  #ifdef ENABLE_MANUAL_CONTROL
    checkButtons(); //check if buttons pressed
  #endif

  //monitor and move cover
  #ifdef COVER_INSTALLED
    monitorAndMoveCover();
    
    if (detachServo){
      completeDetach();
    }
  #endif

  //monitor light if value changed
  #ifdef LIGHT_INSTALLED
    monitorLightChange();
  #endif

  //monitor and control heater
  #ifdef HEATER_INSTALLED
    #ifdef COVER_INSTALLED
      //if cover is not moving and heater isn't in error state
      if (currentCoverState !=2 && heaterState != 5){
        manageHeat();
      }
    #else
    //if heater isn't in error state
     if (heaterState != 5){
        manageHeat();
      }
    #endif
  #endif

  //debug, if true blinks builtin led
  #ifdef SHOW_HEARTBEAT
    beat();
  #endif
}//end of loop

void initializeVariables(){  
  //get saved values from EEPROM
  #ifdef ENABLE_SAVING_TO_MEMORY
    #ifdef COVER_INSTALLED
      currentCoverState = EEPROMwl.get(SAVED_COVER_STATE, currentCoverState);
      if (currentCoverState <= 0) currentCoverState = 4; //set cover to 4:Unknown if no recorded state exists
    #endif

    #ifdef LIGHT_INSTALLED
      previousLightPanelValue = EEPROMwl.get(SAVED_PANEL_VALUE, previousLightPanelValue);
      broadbandValue = EEPROMwl.get(SAVED_BROADBAND_VALUE, broadbandValue);
      narrowbandValue = EEPROMwl.get(SAVED_NARROWBAND_VALUE, narrowbandValue);
  
      //set default for last brigtness value if none exists
      if (previousLightPanelValue < 0){
        previousLightPanelValue = 255;
      }
    
      //set default broadband value if none exists
      if (broadbandValue <= 0){
        broadbandValue = 25;
      }
    
      //set default narrowband value if none exists
      if (narrowbandValue <= 0){
        narrowbandValue = 255;
      }
    #endif
  #else
    //set default values if not using EEPROM
    #ifdef COVER_INSTALLED
      currentCoverState = 4; //set cover to 4:Unknown
    #endif
    
    #ifdef LIGHT_INSTALLED
      previousLightPanelValue = 255; //sets default value
      broadbandValue = 0; //set for operation
      narrowbandValue = 0; //set for operation
    #endif
  #endif
  
  #if !(defined(LIGHT_INSTALLED))
    calibratorState = 0; //light panel not installed, set reporting to 0:NotPresent
    maxBrightness = 0; //set maxBrightness to zero
  #endif

  #ifdef COVER_INSTALLED
    //if panel in open position at start leave in position
    if (currentCoverState == 3){
      primaryServo.write(primaryServoOpenCoverAngle);
      primaryServoLastPosition = primaryServoOpenCoverAngle;

      #ifdef SECONDARY_SERVO_INSTALLED
        secondaryServo.write(secondaryServoOpenCoverAngle);
        secondaryServoLastPosition = secondaryServoOpenCoverAngle;
      #endif
      currentCoverState = 3;
    }
    //if panel in any other position, move to close for known starting point
    else {
      primaryServo.write(primaryServoCloseCoverAngle);
      primaryServoLastPosition = primaryServoCloseCoverAngle;

      #ifdef SECONDARY_SERVO_INSTALLED
        secondaryServo.write(secondaryServoCloseCoverAngle);
        secondaryServoLastPosition = secondaryServoCloseCoverAngle;
      #endif
      currentCoverState = 1;
    }

    previousMoveCoverTo = currentCoverState;
    attachServo();
    setDetachTimer(); //start timer to detach servo
  #else
    currentCoverState = 0; //cover not installed, set reporting to 0:NotPresent
  #endif

  #ifdef HEATER_INSTALLED
    #ifdef ENABLE_BME280
      //check environment monitoring sensors exist before allowing function
      bool bmeStatus = bme.begin(0x76); //initalize sensor at address 0x76
      if (!bmeStatus){
        bmeStatus = bme.begin(0x77); //try 2nd address if failure
        if (!bmeStatus){
          heaterError = true; //report ERROR
        }
      }
    #else
      dht.begin();
    #endif

    #ifdef HEATER_ONE_INSTALLED
      chOneSensor.begin();
    #endif

    #ifdef HEATER_TWO_INSTALLED
      chTwoSensor.begin();
    #endif

    setHeaterState();
  #else
    heaterState = 0; //heater not installed, set reporting to 0:NotPresent
  #endif
}//end of initializeVariables

#ifdef ENABLE_SERIAL_CONTROL
  void initializeComms(){
    Serial.begin(serialSpeed); //start serial
    Serial.flush();
  }

  void checkSerial() {
    static uint8_t index = 0;
    static bool receiveInProgress = false;

    //while serial.available and command isn't complete, read the serial data
    while (Serial.available() > 0 && !commandComplete) {
      char incomingChar = Serial.read();

      if (receiveInProgress) {
        if (incomingChar != endMarker) {
          if(incomingChar == startMarker){
            index = 0;
            memset(receivedChars, 0, sizeof(receivedChars));
          }
          else {
            receivedChars[index] = incomingChar;
            index++;
            if (index >= maxNumChars) {
              index = maxNumChars - 1;
            }
          }
        } else {
          receivedChars[index] = '\0';  //terminate string
          receiveInProgress = false;
          commandComplete = true;
          index = 0;
        }
      } else if (incomingChar == startMarker) {
        receiveInProgress = true;
        index = 0;
        memset(receivedChars, 0, sizeof(receivedChars));
      }
    }
  }

  void processCommand() {
    char cmd = receivedChars[0];
    char* cmdParameter = &receivedChars[1];

    switch (cmd) {
      
      //currentCoverState reports # 0:NotPresent, 1:Closed, 2:Moving, 3:Open, 4:Unknown, 5:Error
      case 'P':
        getCoverState();
        respondToCommand(response);
        break;

      //OPEN cover
      #ifdef COVER_INSTALLED
        case 'O':
          openCover();
          respondToCommand(receivedChars);
          break;
    
        //CLOSE cover
        case 'C':
          closeCover();
          respondToCommand(receivedChars);
          break;
    
        //HALT cover moving
        case 'H':
          haltCover();
          respondToCommand(receivedChars);
          break;
      #endif //COVER_INSTALLED

      //CalibratorState (reports # 0:NotPresent, 1:Off, 2:NotReady, 3:Ready, 4:Unknown, 5:Error)
      case 'L':
        getCalibratorState();
        respondToCommand(response);
        break;

      #ifdef LIGHT_INSTALLED
      //Brightness (report current brightness level)
      case 'B':
        getCurrentBrightness();
        respondToCommand(response);
        break;

      //MaxBrightness (report maximum brightness value)
      case 'M':
        getMaxBrightness();
        respondToCommand(response);
        break;

      //CalibratorOn (turns light on)
      case 'T':
        lightValue = atoi(cmdParameter); //convert char to int
        lightValue = constrain(lightValue, 0, maxBrightness); //check in place if using direct serial connection
        turnPanelTo();
        respondToCommand(receivedChars);
        break;

      //CalibratorOff (turns light off)
      case 'F':
        turnPanelOff();
        respondToCommand(receivedChars);
        break;

      //autoON set to (true)
      case 'A':
        autoON = true;
        respondToCommand(receivedChars);
        break;

      //autoON set to (false)
      case 'a':
        autoON = false;
        respondToCommand(receivedChars);
        break;

      //setStabilizeTime
      case 'S':
        setStabilizeTime(cmdParameter);
        respondToCommand(receivedChars);
        break;

      //setBroadband & Narrowband values
      case 'D':
        if (cmdParameter[0] == 'B') {
          broadbandValue = lightValue;
          #ifdef ENABLE_SAVING_TO_MEMORY
            EEPROMwl.put(SAVED_BROADBAND_VALUE, broadbandValue);
          #endif
        } else {
          narrowbandValue = lightValue;
          #ifdef ENABLE_SAVING_TO_MEMORY
            EEPROMwl.put(SAVED_NARROWBAND_VALUE, narrowbandValue);
          #endif
        }
        respondToCommand(receivedChars);
        break;

      //goto Broadband & Narrowband values
      case 'G':
        if (cmdParameter[0] == 'B') {
          lightValue = broadbandValue / brightnessSteps;
        } else {
          lightValue = narrowbandValue / brightnessSteps;
        }
        itoa(lightValue, response, 10);  //convert integer to string
        respondToCommand(response);
        break;
      #endif //LIGHT_INSTALLED

      //heaterState //reports # 0:NotPresent, 1:Off, 2:Auto, 3:On, 4:Unknown, 5:Error, 6:Set (HeatOnClose)
      case 'R':
        itoa(heaterState, response, 10); //convert integer to string
        respondToCommand(response);
        break;

      #ifdef HEATER_INSTALLED
        //autoHeat set to (true)
        case 'Q':
          autoHeat = true; //set flag
          heatOnClose = false; //reset flag
          manualHeat = false; //reset flag
          setHeaterState();
          respondToCommand(receivedChars);
          break;
    
        //autoHeat set to (false)
        case 'q':
          if (autoHeat){
            autoHeat = false; //set flag
            resetErrorReadings();
            setHeaterState();
          }
          respondToCommand(receivedChars);
          break;
    
        //heatOnClose set to (true)
        case 'E':
          heatOnClose = true; //set flag
          autoHeat = false; //reset flag
          manualHeat = false; //reset flag
          readSensors(); //verify sensors work and report so user can address before event
          setHeaterState();
          respondToCommand(receivedChars);
          break;
    
        //heatOnClose set to (false)
        case 'e':
          if (heatOnClose){
            heatOnClose = false; //set flag
            resetErrorReadings();
            setHeaterState();
          }
          respondToCommand(receivedChars);
          break;
    
        //manualHeat (ON)
        case 'W':
          manualHeat = true; //set flag for time limit
          setHeaterState();
          startHeaterTimer = millis(); //start limit
          respondToCommand(receivedChars);
          break;
    
        //manualHeat (OFF)
        case 'w':
          if (heaterUnknown || heaterError){
            manualHeat = false;
            heatOnClose = false;
            autoHeat = false;
            resetErrorReadings();
          }
          else {
            if (manualHeat){
              //if manual heat is on, turn off
              manualHeat = false; //reset flag
            }
            else {
              //if another option is on, turn off
              heatOnClose = false; //reset flag
              autoHeat = false; //reset flag
            }
          }
          setHeaterState();
          respondToCommand(receivedChars);
          break;
      #endif //HEATER_INSTALLED

      //DLC firmware version
      case 'V':
        respondToCommand(dlcVersion);
        break;

      //acknowledge and respond to unknown command received
      default:
        respondToCommand("?");
        break;
    }//end of switch (cmd)
  }//end of processCommand

  void respondToCommand(const char* response) {
    //acknowledge response to command
    char buffer[maxNumChars];
    snprintf(buffer, sizeof(buffer), "%c%s%c", startMarker, response, endMarker);
    Serial.print(buffer);

    commandComplete = false; //reset flag to receive next command
  }//end of respondToCommand
#endif //(ENABLE_SERIAL_CONTROL)

#ifdef ENABLE_MANUAL_CONTROL
  void checkButtons() {
    #ifdef COVER_INSTALLED
      static uint8_t servoPressCount = 0;  //track the number of presses for servoButton
      static uint32_t firstPressTime = 0;

      uint8_t readingServoButton = digitalRead(servoButton);

      //servo button logic
      if (readingServoButton == LOW && lastServoButtonState == HIGH && (millis() - lastServoButtonPressTime) > debounceDelay) {
        lastServoButtonPressTime = millis();
        servoPressCount++;

        if (servoPressCount == 1) {
          firstPressTime = millis(); //mark first press time
        }
      }

      if (servoPressCount > 0 && (millis() - firstPressTime) >= doublePressTime) {  
        if (servoPressCount == 1) { //single press
          if (currentCoverState == 2) {
            haltCover();
          }
          else if (currentCoverState == 1 && previousMoveCoverTo == 1 || ((currentCoverState == 4 || currentCoverState == 5) && previousMoveCoverTo == 3)) {
            openCover();
          }
          else if (currentCoverState == 3 && previousMoveCoverTo == 3 || ((currentCoverState == 4 || currentCoverState == 5) && previousMoveCoverTo == 1)) {
            closeCover();
          }
        }
        else if (servoPressCount == 2) { //double press
          if (currentCoverState == 1 && previousMoveCoverTo == 1 || ((currentCoverState == 4 || currentCoverState == 5) && previousMoveCoverTo == 1)) {
            openCover();
          }
          else if (currentCoverState == 3 && previousMoveCoverTo == 3 || ((currentCoverState == 4 || currentCoverState == 5) && previousMoveCoverTo == 3)) {
            closeCover();
          }
        }
        servoPressCount = 0; //reset press count after action
      }

      lastServoButtonState = readingServoButton; //save servo button state for next loop
    #endif  //COVER_INSTALLED

    #ifdef LIGHT_INSTALLED
      static bool adjustingBrightness = false;
      static int8_t brightnessDirection = 1; // 1 = increasing, -1 = decreasing
      static uint32_t lastBrightnessAdjustTime = 0; //track last brightness adjustment time
      static uint32_t lightPressTime = 0; 

      int readingLightButton = digitalRead(lightButton);

      //light button logic
      if (readingLightButton == LOW && lastLightButtonState == HIGH && (millis() - lastLightButtonPressTime) > debounceDelay) {
        lastLightButtonPressTime = millis();
        lightPressTime = millis();
        adjustingBrightness = false; //reset brightness adjustment
      }

      //single press options (turn on/off)
      if (calibratorState == 1 && readingLightButton == HIGH && lastLightButtonState == LOW && (millis() - lastLightButtonPressTime) < longPressTime) {
        lightValue = maxBrightness;
        turnPanelTo();
      }
        
      if (calibratorState == 3 && readingLightButton == HIGH && lastLightButtonState == LOW && (millis() - lastLightButtonPressTime) < longPressTime) {
        turnPanelOff();
      }

      //long press (brightness control)
      if (readingLightButton == LOW && (millis() - lightPressTime) >= longPressTime) {
        if (!adjustingBrightness) {
          adjustingBrightness = true;
          lastBrightnessAdjustTime = millis(); //initialize adjustment time
        }

        if (adjustingBrightness && (millis() - lastBrightnessAdjustTime) >= 1000) { //check if it's time to adjust
          //convert PWM value back to step level
          lightValue = lightValue / brightnessSteps;

          if (lightValue >= maxBrightness) {
              brightnessDirection = -1; //start decreasing
          } else if (lightValue <= 1) {
              brightnessDirection = 1; //start increasing
          }

          //adjust the step level
          lightValue += brightnessDirection;
          lightValue = constrain(lightValue, 1, maxBrightness); //keep within bounds
          turnPanelTo();
          lastBrightnessAdjustTime = millis(); //update last adjustment time
        }
      } else {
          adjustingBrightness = false; //stop adjusting if button is released
      }

      lastLightButtonState = readingLightButton; //save light button state for next loop
    #endif //LIGHT_INSTALLED
  }
#endif //ENABLE_MANUAL_CONTROL

#ifdef ENABLE_SERIAL_CONTROL
  void getCoverState(){
    itoa(currentCoverState, response, 10); //convert integer to string
  }
#endif

#ifdef COVER_INSTALLED
  void openCover(){
    //if not already moving, OPEN, or set to 0:Not Present
    if (currentCoverState != 2 && currentCoverState != 3 && currentCoverState != 0) {
      //if cover is closed
      if (currentCoverState == 1) {
        #ifdef LIGHT_INSTALLED
          //if light panel is present and on, turn off
          if (calibratorState != 0 && calibratorState != 1) {
            turnPanelOff();
          }
        #endif

        #ifdef HEATER_INSTALLED
        //if heater is present, heatOnClose is true, and heater is on, turn off
          if (heaterState == 3) {
            manualHeat = false;
          }
        #endif
        }
      moveCoverTo = 3;
      setMovement();
    }
  }//end of openCover

  void closeCover(){
    //if not already moving, CLOSED, or set to 0:Not Present
    if (currentCoverState != 2 && currentCoverState != 1 && currentCoverState != 0) {
      moveCoverTo = 1;
      setMovement();
    }
  }//end of closeCover

  void haltCover(){
    //if moving
    if (currentCoverState == 2) {
      halt = true;
      previousMoveCoverTo = moveCoverTo;
      currentCoverState = 4; //set to Unknown
      elapsedMoveTime += millis() - startServoTimer;
      setDetachTimer();
    }
  }//end of haltCover

  void attachServo(){
    primaryServo.attach(primeServo, primaryServoMinPulseWidth, primaryServoMaxPulseWidth);

    #ifdef SECONDARY_SERVO_INSTALLED
      secondaryServo.attach(secondServo, secondaryServoMinPulseWidth, secondaryServoMaxPulseWidth);
    #endif
  }//end of attachServo
  
  void setDetachTimer(){
    detachServo = true;
    startDetachTimer = millis();
  }//end of setDetachTimer
  
  void completeDetach(){
    uint32_t detachTime = 3000;
    
    //detach to stop sending PWM since servo stopped
    if (millis() - startDetachTimer >= detachTime){
      primaryServo.detach();

      #ifdef SECONDARY_SERVO_INSTALLED
        secondaryServo.detach();
      #endif

      detachServo = false;
    }
  }//end of completeDetach
  
  void setMovement(){
    //sets time left and servo position based on previous and expected direction for calculation in monitorAndMoveCover
    detachServo = false;  //reset in case restart issued right after halt issued
    
    #ifdef USE_LINEAR
      if (!halt){
        primaryServoLastPosition = primaryServo.read();
      }
      else {
        if (moveCoverTo != previousMoveCoverTo) {
          elapsedMoveTime = timeToMoveCover - elapsedMoveTime;
          primaryServoLastPosition = (moveCoverTo == 3) ? primaryServoCloseCoverAngle : primaryServoOpenCoverAngle;

          #ifdef SECONDARY_SERVO_INSTALLED
            secondaryServoLastPosition = (moveCoverTo == 3) ? secondaryServoCloseCoverAngle : secondaryServoOpenCoverAngle;
          #endif
        }
      }
    #else
      if (halt && moveCoverTo != previousMoveCoverTo){
        elapsedMoveTime = timeToMoveCover - elapsedMoveTime;
      }
    
      primaryServoLastPosition = primaryServo.read();
      primaryServoRemainingDistance = (moveCoverTo == 3) ? (primaryServoOpenCoverAngle - primaryServoLastPosition) : (primaryServoCloseCoverAngle - primaryServoLastPosition);
  
      if (abs(primaryServoRemainingDistance) > abs(primaryServoOpenCoverAngle - primaryServoCloseCoverAngle) / 2){
        elapsedMoveTime = 0;
      }
    #endif
    
    attachServo();
    currentCoverState = 2;
    startServoTimer = millis();
    halt = false; //reset
  }//end of setMovement
  
  void monitorAndMoveCover(){
    uint32_t currentMillis; //holds current milliseconds
    
    //monitor moving and unknown cover
    if (currentCoverState == 2 || currentCoverState == 4){
      //get current time
      currentMillis = millis();
  
      //report ERROR if timeToMoveCover * 2 reached
      if (currentMillis - startServoTimer >= timeToMoveCover * 2){
        currentCoverState = 5;
        #ifdef ENABLE_SAVING_TO_MEMORY
          saveCurrentCoverState();
        #endif
        return; //exit function since error reached
      }
  
      //if moving, then move cover
      if (currentCoverState == 2) {
        uint32_t currentServoTimer = millis();
        float progress = (float)(currentServoTimer - startServoTimer + elapsedMoveTime) / timeToMoveCover;
        progress = constrain(progress, 0.0, 1.0); //stay within bounds

        uint8_t primaryServoPreviousCoverAngle;  
        uint8_t primaryServoTargetPosition = (moveCoverTo == 3) ? primaryServoOpenCoverAngle : primaryServoCloseCoverAngle;

        #ifdef SECONDARY_SERVO_INSTALLED
          uint8_t secondaryServoPreviousCoverAngle;  
          uint8_t secondaryServoTargetPosition = (moveCoverTo == 3) ? secondaryServoOpenCoverAngle : secondaryServoCloseCoverAngle;
        #endif

        uint8_t primaryServoCurrentCoverAngle = calculateServoPosition(currentServoTimer, startServoTimer, primaryServoLastPosition, primaryServoTargetPosition, progress, primaryServoRemainingDistance, primaryServoOpenCoverAngle, primaryServoCloseCoverAngle);
        #ifdef SECONDARY_SERVO_INSTALLED
          uint8_t secondaryServoCurrentCoverAngle = calculateServoPosition(currentServoTimer, startServoTimer, secondaryServoLastPosition, secondaryServoTargetPosition, progress, secondaryServoRemainingDistance, secondaryServoOpenCoverAngle, secondaryServoCloseCoverAngle);
        #endif
  
        if (primaryServoCurrentCoverAngle != primaryServoPreviousCoverAngle) {
          primaryServo.write(primaryServoCurrentCoverAngle);
          primaryServoPreviousCoverAngle = primaryServoCurrentCoverAngle;
        }

        #ifdef SECONDARY_SERVO_INSTALLED
          if (secondaryServoCurrentCoverAngle != secondaryServoPreviousCoverAngle) {
            secondaryServo.write(secondaryServoCurrentCoverAngle);
            secondaryServoPreviousCoverAngle = secondaryServoCurrentCoverAngle;
          }
        #endif
  
        if (progress == 1.0){
          //if cover moved to close
          if (moveCoverTo == 1) {
            #ifdef LIGHT_INSTALLED
              if (autoON){
                lightValue = previousLightPanelValue;
                turnPanelTo();
              }
            #endif
            
            #ifdef HEATER_INSTALLED
              if (heatOnClose){
                manualHeat = true; //set flag for time limit
                setHeaterState();
                startHeaterTimer = millis(); //start time limit
              }
            #endif
          }
          elapsedMoveTime = 0;
          primaryServoLastPosition = primaryServoCurrentCoverAngle;
          #ifdef SECONDARY_SERVO_INSTALLED
            secondaryServoLastPosition = secondaryServoCurrentCoverAngle;
          #endif
          previousMoveCoverTo = currentCoverState = (moveCoverTo == 3) ? 3 : 1;
          #ifdef ENABLE_SAVING_TO_MEMORY
            saveCurrentCoverState();
          #endif
          setDetachTimer();
        }
      }//end of if moving, then move cover
    }//end of monitor moving and unknown cover
  }//end of monitorAndMoveCover

  int calculateServoPosition(unsigned long actualServoTime, unsigned long servoStartTime, int lastPosition, int targetPosition, float progress, int remainingDistance, int openAngle, int closeAngle) {
    #ifdef USE_LINEAR
      return lastPosition + (targetPosition - lastPosition) * progress;
    #else
      if (abs(remainingDistance) > abs(openAngle - closeAngle) / 2) {
        float easedProgress = calculateEasedProgress(progress);
        return lastPosition + (targetPosition - lastPosition) * easedProgress;
      } else {
        float adjustedProgress = (float)(actualServoTime - servoStartTime) / (timeToMoveCover - elapsedMoveTime);;
        adjustedProgress = constrain(adjustedProgress, 0.0, 1.0);
        return lastPosition + (targetPosition - lastPosition) * adjustedProgress;
      }
    #endif
  }

  float calculateEasedProgress(float progress){
    #ifdef USE_CIRCULAR
        return (progress < 0.5) ? 0.5 * (1 - sqrt(1 - 4 * pow(progress, 2))) : 0.5 * (sqrt(-((2 * progress) - 3) * ((2 * progress) - 1)) + 1);
    #elif defined(USE_CUBIC)
        return (progress < 0.5) ? 4 * progress * progress * progress : 1 - pow(-2 * progress + 2, 3) / 2;
    #elif defined(USE_EXPO)
        return (progress == 0) ? 0 : (progress == 1) ? 1 : (progress < 0.5) ? pow(2, 20 * progress - 10) / 2 : (2 - pow(2, -20 * progress + 10)) / 2;
    #elif defined(USE_QUAD)
        return (progress < 0.5) ? 2 * progress * progress : 1 - pow(-2 * progress + 2, 2) / 2;
    #elif defined(USE_QUART)
        return (progress < 0.5) ? 8 * progress * progress * progress * progress : 1 - pow(-2 * progress + 2, 4) / 2;
    #elif defined(USE_QUINT)
        return (progress < 0.5) ? 16 * progress * progress * progress * progress * progress : 1 - pow(-2 * progress + 2, 5) / 2;
    #elif defined(USE_SINE)
        return -(cos(M_PI * progress) - 1) / 2;
    #endif
  }//end of calculateEasedProgress

  #ifdef ENABLE_SAVING_TO_MEMORY
    void saveCurrentCoverState(){
      EEPROMwl.put(SAVED_COVER_STATE, currentCoverState);
    }//end of saveCurrentCoverState
  #endif
#endif //COVER_INSTALLED

#ifdef ENABLE_SERIAL_CONTROL
  void getCalibratorState(){
    itoa(calibratorState, response, 10); //convert integer to string
  }
#endif

#ifdef LIGHT_INSTALLED
  void setStabilizeTime(const char* cmdParameter){
    stabilizeTime = atoi(cmdParameter); //convert char to int
  }

  #ifdef ENABLE_SERIAL_CONTROL
    void getCurrentBrightness(){
      itoa(lightValue / brightnessSteps, response, 10); //convert integer to string
    }

    void getMaxBrightness(){
      itoa(maxBrightness, response, 10); //convert integer to string
    }
  #endif

  void turnPanelTo(){
    lightValue = lightValue * brightnessSteps; //determine the lightValue based on number of brightess steps
    calibratorState = 2; //set to 2:Not Ready
    analogWrite(lightPanel, lightValue); //turn light to

    startLightTimer = millis(); //start timer for stabilizeLight
  }//end of turnPanelON
  
  void turnPanelOff(){
    analogWrite(lightPanel, 0);
    lightValue = 0;
    calibratorState = 1;  //1:Off
  }//end of turnPanelOff
  
  void monitorLightChange(){
    //if light changed, report Ready after defined time
    if (calibratorState == 2){
      if (millis() - startLightTimer >= stabilizeTime){
        calibratorState = 3;
        previousLightPanelValue = lightValue;
          #ifdef ENABLE_SAVING_TO_MEMORY
            EEPROMwl.put(SAVED_PANEL_VALUE, previousLightPanelValue);
          #endif
      }
    }
  }//end of monitorLightChange
#endif //LIGHT_INSTALLED

#ifdef HEATER_INSTALLED
  void setHeaterState(){
    //report # 0:NotPresent, 1:Off, 2:Auto, 3:On, 4:Unknown, 5:Error, 6:Set (HeatOnClose)
    if (heaterError) {
        heaterState = 5; // Error
    } else if (heaterUnknown) {
        heaterState = 4; // Unknown
    } else if (manualHeat) {
        heaterState = 3; // On (Manual)
    } else if (autoHeat) {
        heaterState = 2; // Auto
    } else if (heatOnClose) {
        heaterState = 6; // Heat on Close
    } else {
        heaterState = 1; // Off
        resetErrorReadings();
    }

    //!!!!! SAFTEY FIRST !!!!! ensure PWM is shut off unless heater is set to ON or AUTO 
    if (heaterState != 2 && heaterState != 3){
      #ifdef HEATER_ONE_INSTALLED
        analogWrite(chOneHeater, 0);
      #endif

      #ifdef HEATER_TWO_INSTALLED
        analogWrite(chTwoHeater, 0);
      #endif
    }
  }//end of setHeaterState
  
  void manageHeat(){
    if (!heaterError && (autoHeat || manualHeat)) {
      uint32_t currentDewMillis = millis();
      
      if (currentDewMillis - previousDewMillis >= dewInterval){
        previousDewMillis = currentDewMillis; //update time check
        static uint8_t heaterOnePWM = 0; //initialize PWM value for heater one
        static uint8_t heaterTwoPWM = 0; //initialize PWM value for heater two
  
        //read sensors and check for errors
        if (readSensors()) {
          return; //skip further processing if there's an error
        }
        
        //calculate dew point
        float temp = ((DEW_POINT_ALPHA * outsideTemp) / (DEW_POINT_BETA + outsideTemp)) + log(humidityLevel/100.0);
        float dewPoint = (DEW_POINT_BETA * temp) / (DEW_POINT_ALPHA - temp);

        #ifdef HEATER_ONE_INSTALLED
          activateHeater(heaterOneTemp, chOneHeater, dewPoint, deltaPoint, maxPWM, PWM_MAP_MULTIPLIER, PWM_MAP_RANGE, heaterOnePWM);
        #endif

        #ifdef HEATER_TWO_INSTALLED
          activateHeater(heaterTwoTemp, chTwoHeater, dewPoint, deltaPoint, maxPWM, PWM_MAP_MULTIPLIER, PWM_MAP_RANGE, heaterTwoPWM);
        #endif
      }
    }
  
    //handle manual heat timeout
    if (!heaterError && manualHeat && currentCoverState == 1 && millis() - startHeaterTimer >= heaterShutoff) {
      manualHeat = false;
      setHeaterState();
    }
  
    //if there's a reading issue, attempt to reset the error state
    if (heaterError || (heaterUnknown && heatOnClose)) {
      readSensors();
    }
  }//end of manageHeat

  void activateHeater(float heaterTemp, uint8_t heaterPin, float dewPoint, float deltaPoint, uint8_t maxPWM, float pwmMapMultiplier, uint8_t pwmMapRange, uint8_t& heaterPWM) {
    if (heaterTemp < dewPoint + deltaPoint) {
      //calculate difference from target (dew point + safety margin)
      float tempDiff = (dewPoint + deltaPoint) - heaterTemp;
      heaterPWM = map(tempDiff * pwmMapMultiplier, 0, pwmMapRange, 0, maxPWM);
      heaterPWM = constrain(heaterPWM, 0, maxPWM);

      //write PWM to heater
      analogWrite(heaterPin, heaterPWM);
    } else {
      // If the temperature is above the target, ensure the heater is off
      analogWrite(heaterPin, 0);
      heaterPWM = 0; // Optionally reset the PWM value
    }
  }//end of activateHeater
  
  bool readSensors() {
    bool errorReading = false;
    static bool lastErrorReading = true; //track the previous state
    
    #ifdef HEATER_ONE_INSTALLED
      //read DS18B20 heater temperature
      chOneSensor.requestTemperatures();
      heaterOneTemp = chOneSensor.getTempCByIndex(0);
      if (heaterOneTemp == DEVICE_DISCONNECTED_C) {
        errorReading = true;
      }
    #endif

    #ifdef HEATER_TWO_INSTALLED
      //read DS18B20 heater temperature
      chTwoSensor.requestTemperatures();
      heaterTwoTemp = chTwoSensor.getTempCByIndex(0);
      if (heaterTwoTemp == DEVICE_DISCONNECTED_C) {
        errorReading = true;
      }
    #endif

    #ifdef ENABLE_BME280
      //read BME280 outside temperature and humidity
      outsideTemp = bme.readTemperature();
      humidityLevel = bme.readHumidity();

      //check if any reads failed or outside of expected values
      if (isnan(outsideTemp) || isnan(humidityLevel) ||
          humidityLevel < 0 || humidityLevel > 100 ||
          outsideTemp < -40 || outsideTemp > 85) {
        errorReading = true;
      }
    #endif

    #ifdef ENABLE_DHT22
      //read DHT22 outside temperature and humidity
      outsideTemp = dht.readTemperature();
      humidityLevel = dht.readHumidity();

      //check if any reads failed
      if (isnan(outsideTemp) || isnan(humidityLevel)) {
        errorReading = true;
      }
    #endif
  
    if (errorReading) {
      errorCounter++;
      if (errorCounter >= maxErrorCount) {
        heaterUnknown = false; //clear unknown state
        heaterError = true;    //mark the heater as having an error
        //possibly add greaterErrorCount then:
        //possibly add code to read through bme addresses and update
        //possibly add code to read through DS18B20 addresses and update
      } else {
        heaterUnknown = true;  //set the heater to unknown state
      }
      setHeaterState();
    } else {
      if (lastErrorReading) { 
        resetErrorReadings(); //reset only on transition from true to false
        setHeaterState();
      }
    }
  
    lastErrorReading = errorReading; //update the last error state
    return errorReading;
  } //end of readSensors

  void resetErrorReadings(){
    errorCounter = 0;        //reset error counter on successful reading
    heaterUnknown = false;   //clear unknown state
    heaterError = false;     //clear error state
  }//end of resetErrorReadings
#endif //HEATER_INSTALLED

#ifdef SHOW_HEARTBEAT
  void beat(){
    static uint32_t ledTime;
    uint32_t currentTime = millis();
  
    //check if a second has passed since the last LED toggle
    if (currentTime - ledTime >= 1000) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //toggle the LED
      ledTime = currentTime; //update the time for the next toggle
    }
  }//end of beat
#endif //SHOW_HEARTBEAT