// ant formicarium sensor controller
// 6 input (6i)
// handles 6 sensors and transmits them via serial
// by dylan marcus
// ©2020 by design.create.record.

// Declarations
static const unsigned ledPin = 13;
static const int NUM_PINS = 6;
static const uint8_t analog_pins[NUM_PINS] = {A0,A1,A2,A3,A4,A5};
int sensorValue[NUM_PINS];
int sensorMin[NUM_PINS];
int sensorMax[NUM_PINS];
int incomingByte = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  for (int i = 0; i < NUM_PINS; i++){
    //calibrateSensors(i);
  }
}

void loop(){
  if (Serial.available()) {
    Serial.print(analogRead(analog_pins[0]));
    Serial.print(" ");
    Serial.print(analogRead(analog_pins[1]));
    Serial.print(" ");
    Serial.print(analogRead(analog_pins[2]));
    Serial.print(" ");
    Serial.print(analogRead(analog_pins[3]));
    Serial.print(" ");
    Serial.print(analogRead(analog_pins[4]));
    Serial.print(" ");
    Serial.print(analogRead(analog_pins[5]));
    Serial.print("\n");
    delay(5);
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
