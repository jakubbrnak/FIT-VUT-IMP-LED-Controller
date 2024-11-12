#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

//constants for pwm setup
const int freq = 5000;
const int resolution = 8;

// constatns and aliases for PIN setup
#define BLUE 14
const int pwmChannelBlue = 0;

#define GREEN 27
const int pwmChannelGreen = 1;

#define RED 16
const int pwmChannelRed = 2;

#define SIMPLE1 17
const int pwmChannelSimple1 = 3;

#define SIMPLE2 25
const int pwmChannelSimple2 = 4;

#define SIMPLE3 26
const int pwmChannelSimple3 = 5; 

#define CONNECTED 2

//queue declaration
QueueHandle_t queueSimple;
QueueHandle_t queueRGB;

//BLE characteristic pointer declaration
BLECharacteristic *pCharacteristic;

//uuid for BLE service and characteristic
#define SERVICE_UUID           "aa7113df-59cc-4456-adb1-ccd5d5c5f5ef"
#define CHARACTERISTIC_UUID_RX "2f845da3-ce4a-41d5-822a-02513d2ddcf4"

//class for handling connection
class MyServerCallbacks: public BLEServerCallbacks {
  
    //on connnect event, turn off led to indicate connection success
    void onConnect(BLEServer* pServer) {
      digitalWrite(CONNECTED, LOW);
    };
    
    //on disconnect event, start bluetooth advertising and turn on led to indicate ready state
    void onDisconnect(BLEServer* pServer) {
      digitalWrite(CONNECTED, HIGH);
      pServer->getAdvertising()->start();
    }
};

//class for handling incoming messages
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      
      //load recieved message to variable
      std::string recievedMessage;
      recievedMessage = pCharacteristic->getValue();
      if (recievedMessage.length() > 0) {
    
        //convert recieved message to integer
        int receivedInt = std::atoi(recievedMessage.c_str());

        //send message to its destination process using corresponding queue (integers <1,8> -> queueRGB, integers <9, inf> -> queueSimple)
        if (receivedInt <= 8) {
          xQueueSend(queueRGB, &receivedInt, portMAX_DELAY);
        }
        else {
          xQueueSend(queueSimple, &receivedInt, portMAX_DELAY);
        }
      }
    }
};

//aliases for speed values
#define LowSpeed 0
#define MediumSpeed 1
#define HighSpeed 2


//funciton for siren animation
void sirenAnimation(int speed) {
    static unsigned long lastUpdateTime = 0;
    static int redValue = 255;
    static int greenValue = 0;
    static int blueValue = 0;
    static int colorState = 0; // 0: Red to Green, 1: Green to Blue, 2: Blue to Red

    unsigned long currentMillis = millis();
    int updateInterval;

    switch (speed) {
        case LowSpeed:
            updateInterval = 50; //slower transition for low speed
            break;
        case MediumSpeed:
            updateInterval = 20; //medium transition for medium speed
            break;
        case HighSpeed:
            updateInterval = 5; //faster transition for high speed
            break;
        default:
            updateInterval = 25; //default to medium speed
    }

    if (currentMillis - lastUpdateTime > updateInterval) {
        lastUpdateTime = currentMillis;

        switch (colorState) {
            case 0: //Red to Green
                redValue -= (redValue > 0) ? 5 : 0;
                greenValue += (greenValue < 255) ? 5 : 0;
                if (redValue <= 0 && greenValue >= 255) colorState = 1;
                break;
            case 1: //Green to Blue
                greenValue -= (greenValue > 0) ? 5 : 0;
                blueValue += (blueValue < 255) ? 5 : 0;
                if (greenValue <= 0 && blueValue >= 255) colorState = 2;
                break;
            case 2: //Blue to Red
                blueValue -= (blueValue > 0) ? 5 : 0;
                redValue += (redValue < 255) ? 5 : 0;
                if (blueValue <= 0 && redValue >= 255) colorState = 0;
                break;
        }

        ledcWrite(pwmChannelRed, redValue);
        ledcWrite(pwmChannelGreen, greenValue);
        ledcWrite(pwmChannelBlue, blueValue);
    }
}

void cycleblink(int speed) {
    static unsigned long lastUpdateTime = 0;
    static int dutyCycle = 0;
    static bool increasing = true;
    static int colorState = 0;// 0: Red, 1: Green, 2: Blue

    // Adjust the blink interval based on speed
    int blinkInterval;
    switch (speed) {
        case LowSpeed:
            blinkInterval = 12; //slower blink for low speed
            break;
        case MediumSpeed:
            blinkInterval = 6; //medium blink for medium speed
            break;
        case HighSpeed:
            blinkInterval = 2; //faster blink for high speed
            break;
        default:
            blinkInterval = 8; //default to medium speed
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime > blinkInterval) {
        lastUpdateTime = currentMillis;

        if (increasing) {
            dutyCycle++;
            if (dutyCycle >= 255) {
                increasing = false;
            }
        } else {
            dutyCycle--;
            if (dutyCycle <= 0) {
                increasing = true;
                //change color when the cycle is completed (duty cycle reaches 0)
                colorState = (colorState + 1) % 3;
            }
        }

        //update the LED color based on current color state
        switch (colorState) {
            case 0: //Red
                ledcWrite(pwmChannelRed, dutyCycle);
                ledcWrite(pwmChannelGreen, 0);
                ledcWrite(pwmChannelBlue, 0);
                break;
            case 1: //Green
                ledcWrite(pwmChannelRed, 0);
                ledcWrite(pwmChannelGreen, dutyCycle);
                ledcWrite(pwmChannelBlue, 0);
                break;
            case 2: //Blue
                ledcWrite(pwmChannelRed, 0);
                ledcWrite(pwmChannelGreen, 0);
                ledcWrite(pwmChannelBlue, dutyCycle);
                break;
        }
    }
}


void strobeAnimation(int speed) {
    static unsigned long lastUpdateTime = 0;
    static int colorState = 0; //0: Red, 1: Green, 2: Blue

    //adjust the strobe interval based on speed
    int strobeInterval;
    switch (speed) {
        case LowSpeed:
            strobeInterval = 500; //slower strobe
            break;
        case MediumSpeed:
            strobeInterval = 250; //medium strobe
            break;
        case HighSpeed:
            strobeInterval = 100; //faster strobe
            break;
        default:
            strobeInterval = 250; //default to medium
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime > strobeInterval) {
        lastUpdateTime = currentMillis;

        //reset LEDs
        ledcWrite(pwmChannelRed, 0);
        ledcWrite(pwmChannelGreen, 0);
        ledcWrite(pwmChannelBlue, 0);

        //cycle through colors
        switch (colorState) {
            case 0: //Red
                ledcWrite(pwmChannelRed, 255);
                break;
            case 1: //Green
                ledcWrite(pwmChannelGreen, 255);
                break;
            case 2: //Blue
                ledcWrite(pwmChannelBlue, 255);
                break;
        }

        colorState = (colorState + 1) % 3; //move to the next color
    }
}

void patternedBlink(int speed) {
    static unsigned long lastUpdateTime = 0;
    static int blinkCount = 0;
    static int blinkPhase = 0; // 0: One blink, 1: Two blinks, 2: Three blinks
    static int colorIndex = 0;

    // Define a series of colors
    int colors[][3] = {
        {255, 0, 0}, //Red
        {0, 255, 0}, //Green
        {0, 0, 255}, //Blue
        {255, 255, 0}, //Yellow
        {0, 255, 255}, //Cyan
        {255, 0, 255}, //Magenta
    };
    int numColors = sizeof(colors) / sizeof(colors[0]);

    //adjust intervals based on speed
    int blinkInterval, pauseInterval;
    switch (speed) {
        case LowSpeed:
            blinkInterval = 400; //longer blink
            pauseInterval = 800; //longer pause
            break;
        case MediumSpeed:
            blinkInterval = 200; //medium blink
            pauseInterval = 400; //medium pause
            break;
        case HighSpeed:
            blinkInterval = 100; //short blink
            pauseInterval = 200; //short pause
            break;
        default:
            blinkInterval = 200;
            pauseInterval = 400;
    }

    unsigned long currentMillis = millis();
    if (blinkPhase <= blinkCount) {
        if (currentMillis - lastUpdateTime > blinkInterval) {
            //blink on
            ledcWrite(pwmChannelRed, colors[colorIndex][0]);
            ledcWrite(pwmChannelGreen, colors[colorIndex][1]);
            ledcWrite(pwmChannelBlue, colors[colorIndex][2]);
        }
        if (currentMillis - lastUpdateTime > blinkInterval * 2) {
            //blink off and increase phase
            ledcWrite(pwmChannelRed, 0);
            ledcWrite(pwmChannelGreen, 0);
            ledcWrite(pwmChannelBlue, 0);
            lastUpdateTime = currentMillis;
            blinkPhase++;
        }
    } else if (currentMillis - lastUpdateTime > pauseInterval) {
        //after pausing, reset the phase and change color if needed
        blinkPhase = 0;
        blinkCount++;
        if (blinkCount > 2) {
            blinkCount = 0;
            colorIndex = (colorIndex + 1) % numColors; //change color
        }
        lastUpdateTime = currentMillis;
    }
}

void rgbOFF() {
    ledcWrite(pwmChannelRed, 0);
    ledcWrite(pwmChannelGreen, 0);
    ledcWrite(pwmChannelBlue, 0);
}

typedef void (*FunctionPointer)(int);

//task function to control rgb led
void LedControlRGB(void *pvParameters) {
    //pointer to the function that is currently called based on the users input
    FunctionPointer currentFunction = NULL;

    //variables to store current mode and speed
    int currentSpeed = MediumSpeed;
    int receivedInt;

    //task cycle that calls current used function and if needed, processes messages from queue
    while (true) {
        if (xQueueReceive(queueRGB, &receivedInt, 0)) {
            //switch for assigning corresponding function pointer to currentFunction accirdigng to processed message
            switch (receivedInt) {
                case 1:
                    rgbOFF();
                    currentFunction = cycleblink;
                    break;
                case 2:
                     rgbOFF();
                    currentFunction = strobeAnimation;
                    break;
                  
                case 3:
                    rgbOFF();
                    currentFunction = sirenAnimation;
                    break;
                case 4:
                    rgbOFF();;
                    currentFunction = patternedBlink;
                    break;
                case 5: 
                  currentSpeed = LowSpeed;
                  break;

                case 6:
                  currentSpeed = MediumSpeed;
                  break;

                case 7:
                  currentSpeed = HighSpeed;
                  break;

                case 8:
                  rgbOFF();
                  currentFunction = NULL;
                default:
                  rgbOFF();
                  currentFunction = NULL;
                  break;
            }
        }
        if (currentFunction != NULL) {
            currentFunction(currentSpeed);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
} 

void simpleLedBlink(int speed) {
    static unsigned long lastUpdateTime = 0;
    static bool ledState = false;

    //adjust the blink interval based on speed
    int blinkInterval;
    switch (speed) {
        case LowSpeed:
            blinkInterval = 1000; //slower blink
            break;
        case MediumSpeed:
            blinkInterval = 500;  //medium blink
            break;
        case HighSpeed:
            blinkInterval = 250;  //fast blink
            break;
        default:
            blinkInterval = 500;  //default to medium
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime > blinkInterval) {
        lastUpdateTime = currentMillis;
        ledState = !ledState; //toggle the state

        //control each LED individually
        ledcWrite(pwmChannelSimple1, ledState ? 0 : 255);
        ledcWrite(pwmChannelSimple2, ledState ? 0 : 255);
        ledcWrite(pwmChannelSimple3, ledState ? 0 : 255);
    
    }
}

void simpleLedFade(int speed) {
    static unsigned long lastUpdateTime = 0;
    static int fadeValue = 0;
    static int fadeDirection = 1;

    //adjust the fade speed based on speed input
    int fadeInterval;
    switch (speed) {
        case LowSpeed:
            fadeInterval = 15; //slower fade
            break;
        case MediumSpeed:
            fadeInterval = 10; //medium fade
            break;
        case HighSpeed:
            fadeInterval = 5;  //fast fade
            break;
        default:
            fadeInterval = 10; //default to medium
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime > fadeInterval) {
        lastUpdateTime = currentMillis;
        fadeValue += fadeDirection;

        //invert fade direction at the limits
        if (fadeValue <= 0 || fadeValue >= 255) {
            fadeDirection = -fadeDirection;
        }

        //calculate fade values for each LED
        int fadeValue1, fadeValue2, fadeValue3;
        if (fadeValue <= 85) {
            //fade in LED 1, others off
            fadeValue1 = fadeValue;
            fadeValue2 = 0;
            fadeValue3 = 0;
        } else if (fadeValue <= 170) {
            //fade in LED 2, fade out LED 1
            fadeValue1 = 170 - fadeValue;
            fadeValue2 = fadeValue - 85;
            fadeValue3 = 0;
        } else {
            //fade in LED 3, fade out LED 2
            fadeValue1 = 0;
            fadeValue2 = 255 - fadeValue;
            fadeValue3 = fadeValue - 170;
        }

        //update LED brightness
        ledcWrite(pwmChannelSimple1, fadeValue1);
        ledcWrite(pwmChannelSimple2, fadeValue2);
        ledcWrite(pwmChannelSimple3, fadeValue3);
    }
}

void simpleLedToggle(int speed) {
    static unsigned long lastUpdateTime = 0;
    static bool isMiddleOn = false;

    //adjust the toggle speed based on speed input
    int toggleInterval;
    switch (speed) {
        case LowSpeed:
            toggleInterval = 1000; //slower toggle
            break;
        case MediumSpeed:
            toggleInterval = 500; //medium toggle
            break;
        case HighSpeed:
            toggleInterval = 250; //fast toggle
            break;
        default:
            toggleInterval = 500; //default to medium
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime > toggleInterval) {
        lastUpdateTime = currentMillis;
        isMiddleOn = !isMiddleOn; //toggle the state

        //set LED states based on isMiddleOn
        if (isMiddleOn) {
            //middle LED on, side LEDs off
            ledcWrite(pwmChannelSimple1, 0); //1 off
            ledcWrite(pwmChannelSimple2, 255); //2 on
            ledcWrite(pwmChannelSimple3, 0); //3 off
        } else {
            //side LEDs on, middle LED off
            ledcWrite(pwmChannelSimple1, 255); //1 on
            ledcWrite(pwmChannelSimple2, 0); //2 off
            ledcWrite(pwmChannelSimple3, 255); //3 on
        }
    }
}

void simpleLedWave(int speed) {
    static unsigned long lastUpdateTime = 0;
    static int currentLed = 1;
    static bool isAscending = true;

    //adjust the wave speed based on speed input
    int waveInterval;
    switch (speed) {
        case LowSpeed:
            waveInterval = 1000; //slower wave
            break;
        case MediumSpeed:
            waveInterval = 500; //medium wave
            break;
        case HighSpeed:
            waveInterval = 250; //fast wave
            break;
        default:
            waveInterval = 500; //default to medium
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime > waveInterval) {
        lastUpdateTime = currentMillis;

        //turn off all LEDs
        ledcWrite(pwmChannelSimple1, 0);
        ledcWrite(pwmChannelSimple2, 0);
        ledcWrite(pwmChannelSimple3, 0);

        //turn on the current LED
        switch (currentLed) {
            case 1:
                ledcWrite(pwmChannelSimple1, 255);
                break;
            case 2:
                ledcWrite(pwmChannelSimple2, 255);
                break;
            case 3:
                ledcWrite(pwmChannelSimple3, 255);
                break;
        }
        
        // Update the current LED and direction
        if (isAscending) {
            if (currentLed < 3) {
                currentLed++;
            } else {
                isAscending = false;
            }
        } else {
            if (currentLed > 1) {
                currentLed--;
            } else {
                isAscending = true;
            }
        }
    }
}

void simpleLedSequence(int speed) {
    static unsigned long lastUpdateTime = 0;
    static int step = 0;

    //adjust the sequence speed based on speed input
    int sequenceInterval;
    switch (speed) {
        case LowSpeed:
            sequenceInterval = 1000; //slower sequence
            break;
        case MediumSpeed:
            sequenceInterval = 500; //medium sequence
            break;
        case HighSpeed:
            sequenceInterval = 250; //fast sequence
            break;
        default:
            sequenceInterval = 500; //default to medium
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime > sequenceInterval) {
        lastUpdateTime = currentMillis;

        //update the LEDs based on the current step
        switch (step) {
            case 0: //left LED on
                ledcWrite(pwmChannelSimple1, 255);
                break;
            case 1: //right LED on
                ledcWrite(pwmChannelSimple3, 255);
                break;
            case 2: //middle LED on
                ledcWrite(pwmChannelSimple2, 255);
                break;
            case 3: //all LEDs off
              ledcWrite(pwmChannelSimple1, 0);
              ledcWrite(pwmChannelSimple2, 0);
              ledcWrite(pwmChannelSimple3, 0);
                break;
        }
        //update the step
        step = (step + 1) % 4; //cycle back to 0 after reaching 3
    }
}

void simpleLedOff(int speed){
    //turn off all LEDs
    ledcWrite(pwmChannelSimple1, 0);
    ledcWrite(pwmChannelSimple2, 0);
    ledcWrite(pwmChannelSimple3, 0);
}

//task function to control simple leds
void LedControlSimple(void *pvParameters) {
    //pointer to the function that is currently called based on the users input
    FunctionPointer currentFunction = NULL;
    
    //variables to store current mode and speed
    int currentSpeed = MediumSpeed;
    int receivedInt;

    //task cycle that calls current used function and if needed, processes messages from queue
    while (true) {
        if (xQueueReceive(queueSimple, &receivedInt, 0)) {
            switch (receivedInt) {
                case 9:
                  currentFunction = simpleLedBlink;
                break;
                case 10:
               
                  currentFunction = simpleLedFade;
                  break;
                  
                case 11:
                  currentFunction = simpleLedToggle;
                    break;
                case 12:
                  currentFunction = simpleLedSequence;
                    break;
                case 13: 
                  currentSpeed = LowSpeed;
                  break;

                case 14:
                  currentSpeed = MediumSpeed;
                  break;

                case 15:
                  currentSpeed = HighSpeed;
                  break;

                case 16:
                  simpleLedOff(0);
                  currentFunction = NULL;
                default:
                    currentFunction = NULL;
                    break;
            }
        }
        if (currentFunction != NULL) {
            currentFunction(currentSpeed);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
} 

void setup() {

  //serial communication init
  Serial.begin(115200);
  
  //set led pins as output
  pinMode(CONNECTED, OUTPUT);

  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);

  pinMode(SIMPLE1, OUTPUT);
  pinMode(SIMPLE2, OUTPUT);
  pinMode(SIMPLE3, OUTPUT);

  //initialization of BLE and device name set
  BLEDevice::init("ESP32 BLE");
  
  //creating BLE server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  //creatung BLE service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  //create BLE communication channel for recieving messages
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  //set callback function that will be triggered after recieved message
  pCharacteristic->setCallbacks(new MyCallbacks());
  
  //start BLE service
  pService->start();
  
  //start BLE advertising, and turn on led to indicate ready state
  pServer->getAdvertising()->start();
  digitalWrite(CONNECTED, HIGH);

  //setup pwm channels and attach to corresponding pin
  ledcSetup(pwmChannelRed, freq, resolution);
  ledcAttachPin(RED, pwmChannelRed);
  
  ledcSetup(pwmChannelBlue, freq, resolution);
  ledcAttachPin(BLUE, pwmChannelBlue);

  ledcSetup(pwmChannelGreen, freq, resolution);
  ledcAttachPin(GREEN, pwmChannelGreen);

  ledcSetup(pwmChannelSimple1, freq, resolution);
  ledcAttachPin(SIMPLE1, pwmChannelSimple1);

  ledcSetup(pwmChannelSimple2, freq, resolution);
  ledcAttachPin(SIMPLE2, pwmChannelSimple2);

  ledcSetup(pwmChannelSimple3, freq, resolution);
  ledcAttachPin(SIMPLE3, pwmChannelSimple3);

  //create queues for passing messages from bluetooth callback
  //create tasks pinned to core to ensure that each group of leds are controlled by one core
  queueRGB = xQueueCreate(10, sizeof(int));
  xTaskCreatePinnedToCore(LedControlRGB, "LedRGB", 2048, NULL, 1, NULL, 0);

  queueSimple = xQueueCreate(10, sizeof(int));
  xTaskCreatePinnedToCore(LedControlSimple, "LedSimple", 2048, NULL, 1, NULL, 1);
}

void loop() {
  //empty loop because all the functionality is implemented in task functions
}