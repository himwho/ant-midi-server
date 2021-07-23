// ant formicarium sensor controller
// 11 input (11i)
// handles 11 sensors and transmits them via serial
// by dylan marcus
// ©2020 by design.create.record.

// Declarations
static const unsigned ledPin = 13;
static const int NUM_PINS = 11;
static const uint8_t analog_pins[NUM_PINS] = {A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10};
int sensorValue[NUM_PINS];
int sensorMin[NUM_PINS];
int sensorMax[NUM_PINS];

void setup() {
  Serial.begin(115200);

//  for (int i = 0; i < NUM_PINS; i++){
//    calibrateSensors(i);
//  }
}

void loop(){
  if (Serial.available()) { // If there is any data available
    char inByte = Serial.read(); // store the incoming data
    if (inByte == 1) {    // Whether the received data is '1'
      for (int i = 0; i < NUM_PINS; i++){
        Serial.print(analogRead(analog_pins[i]));
        Serial.print(F(" "));
      }
      Serial.println();
    }
  }
}

void calibrateSensors(int pinNumber){
    int currentMillis = millis();
    while (millis() - currentMillis < 5000){
        for (int i = 0; i < NUM_PINS; i++){
            sensorValue[pinNumber] = analogRead(pinNumber);
            if (sensorValue[pinNumber] > sensorMax[pinNumber]){
                sensorMax[pinNumber] = sensorValue[pinNumber];
            }
            if (sensorValue[pinNumber] < sensorMin[pinNumber]){
                sensorMin[pinNumber] = sensorValue[pinNumber];
            }
        }
    }
}
