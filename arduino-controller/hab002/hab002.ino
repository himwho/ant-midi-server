// ant formicarium sensor controller
// 6 input (6i)
// handles 6 sensors and transmits them via serial as osc
// by dylan marcus
// ©2020 by design.create.record.

#include <OSCBundle.h>
#include <OSCBoards.h>
#include <OSCTiming.h>

#ifdef BOARD_HAS_USB_SERIAL
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial( thisBoardsSerialUSB );
#else
#include <SLIPEncodedSerial.h>
 SLIPEncodedSerial SLIPSerial(Serial);
#endif

// Declarations
static const unsigned ledPin = 13;
static const int NUM_PINS = 6;
static const uint8_t analog_pins[NUM_PINS] = {A0,A1,A2,A3,A4,A5};
int sensorValue[NUM_PINS];
int sensorMin[NUM_PINS];
int sensorMax[NUM_PINS];

void setup() {
  //begin SLIPSerial just like Serial
    SLIPSerial.begin(115200);
#if ARDUINO >= 100
    while(!Serial)
      ;   // Leonardo bug
#endif

  for (int i = 0; i < NUM_PINS; i++){
    calibrateSensors(i);
  }
}

void loop(){
  for (int i = 0; i < NUM_PINS; i++){
    //the message wants an OSC address as first argument
    OSCMessage msg("/hab002/"+i);
    msg.add((int32_t)analogRead(i));

    SLIPSerial.beginPacket();  
      msg.send(SLIPSerial); // send the bytes to the SLIP stream
    SLIPSerial.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message
  }
  delay(5);
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
