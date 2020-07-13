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
  //begin SLIPSerial just like Serial
  Serial.begin(115200);
  Serial1.begin(115200);

  for (int i = 0; i < NUM_PINS; i++){
    //calibrateSensors(i);
  }
}

void loop(){
    // read from port 0, send to port 1:
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.print(inByte, DEC);
  
    Serial.print(analogRead(analog_pins[0]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[1]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[2]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[3]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[4]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[5]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[6]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[7]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[8]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[9]));
    Serial.print(",");
    Serial.print(analogRead(analog_pins[10]));
    Serial.print("\n");
    delay(1);
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
