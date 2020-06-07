//antfarm microcontroller
//mini module
//by dylan marcus
//©2019 by design.create.record.

#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCBoards.h>

#ifdef BOARD_HAS_USB_SERIAL
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial( thisBoardsSerialUSB );
#else
#include <SLIPEncodedSerial.h>
 SLIPEncodedSerial SLIPSerial(Serial);
#endif

static const unsigned ledPin = 13;
static const int NUM_PINS = 6;
static const uint8_t analog_pins[NUM_PINS] = {A0,A1,A2,A3,A4,A5};
int sensorValue[NUM_PINS];
int sensorMin[NUM_PINS];
int sensorMax[NUM_PINS];
int lastSensorValue[NUM_PINS];
int deltaValue[NUM_PINS];
int deltaValueSigned[NUM_PINS];
int detectDiffMin = 15;
int detectDiffMax = 99;
int readLevel[NUM_PINS];
int adjustedReadLevel[NUM_PINS];

//MUSIC SETUP
int currentBPM = 120;
int calcVelocity = 0x45;
int calcPitch = 0x24;
//SECURITY SETUP
int secArray[NUM_PINS];
static const unsigned long UPDATE_TIME_LIMIT = 1000; // ms
static unsigned long lastUpdateTime = 0;

//ANT BASED ADC (Ant to Digital Converter) APP SELECTION HERE:
bool ANTSECURITY = false;
bool ANTMUSIC = true;

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("ready.");

  SLIPSerial.begin(115200);
  
  for (int i = 0; i < NUM_PINS; i++){
    readLevel[i]=analogRead(analog_pins[i]);
    sensorMin[i]=readLevel[i]-5;
    sensorMax[i]=readLevel[i];
    Serial.println("Pin number: " + String(i) + ", Min: " + sensorMin[i] + ", Max: " + sensorMax[i]);
  }
  
//  //input function for BPM changing
//  Particle.function("BPM",particleSetBPM);
}

void loop() {
    for (int i = 0; i < NUM_PINS; i++){
        if (ANTMUSIC) {
          //auto-adjust the minimum and maximum limits in real time
          readLevel[i]=analogRead(analog_pins[i]);
          if(sensorMin[i]>readLevel[i]){
          sensorMin[i]=readLevel[i];
          }
          if(sensorMax[i]<readLevel[i]){
          sensorMax[i]=readLevel[i];
          }
          
          //Adjust the light level to produce a result between 0 and 100.
          adjustedReadLevel[i] = map(readLevel[i], sensorMin[i], sensorMax[i], 0, 100); 
          Serial.println("AdjustedLevel[" + String(i) + "]: " + adjustedReadLevel[i]);
          updateMusicPin(i, adjustedReadLevel[i]);
        }
        delay(50);  //Slow down readings       
        if (ANTSECURITY) {
            updateSecurityPin(i);
            if(millis() - lastUpdateTime >= UPDATE_TIME_LIMIT){
            lastUpdateTime += UPDATE_TIME_LIMIT;
                bool clearArray = true; 
                sendSecurityArray(clearArray);
          }
        }
    }
}

// SECURITY //
void updateSecurityPin(int pinNumber){
    secArray[pinNumber] = analogRead(analog_pins[pinNumber]);
}

void sendSecurityArray(bool clearArray){
    //TODO: make this adapt to NUM_PINS
//    Particle.publish("ANT ENTROPY", String(secArray[0])+String(secArray[1])+String(secArray[2])+String(secArray[3])+String(secArray[4])+String(secArray[5])+String(secArray[6])+String(secArray[7]));
    //clear array
    if (clearArray){
        for (int i = 0; i < NUM_PINS; i++){
            secArray[i] = 0;
        }
        return;
    } else {
        return;
    }
}

// MUSIC //
void updateMusicPin(int pinNumber, int value){
    sensorValue[pinNumber] = value;
    //deltaValue will be used to calculate velocity
    deltaValue[pinNumber] = abs(sensorValue[pinNumber] - lastSensorValue[pinNumber]);
    //deltaValueSigned will be used to calculate pitch change
    deltaValueSigned[pinNumber] = sensorValue[pinNumber] - lastSensorValue[pinNumber];
    if (deltaValue[pinNumber] > detectDiffMin){
        playNote(pinNumber, deltaValue[pinNumber], deltaValueSigned[pinNumber]);
    }
    //update the value for the next loop
    lastSensorValue[pinNumber] = sensorValue[pinNumber];
}

void playNote(int pinNumber, int deltaValue, int deltaValueSigned){
    Serial.print("ant activity at "); Serial.print(pinNumber); Serial.print(" with delta value: "); Serial.println(deltaValue);
    //Particle.publish("AntTriggeredOn", String(pinNumber)+String(" with delta: ")+String(deltaValue[pinNumber])); 
    //the velocity is calculated and mapped from 0 to 127
    calcVelocity = map(deltaValue, detectDiffMin, detectDiffMax, 0x00, 0x90);
    //the pitch is calculated and mapped (please change the input mappings to calibrate)
    //TODO: use chromatic scaling for mapped values
    calcPitch = map(deltaValueSigned, detectDiffMin, detectDiffMax, 0x1E, 0x5A);
    //send as MIDI
    noteOn(0x90, calcPitch, calcVelocity, currentBPM);
    delay(100);
    //TODO: change for correct note length calculation
    noteOff(0x80, calcPitch, 0x00, currentBPM);
}

int SetBPM(String setBPM){
  if (setBPM != NULL) {
        //set the BPM for quantization
        currentBPM = setBPM.toInt();
        Serial.print("BPM: "); Serial.println(currentBPM);
        return 1;
  } else {
        return -1;
  }
}

void noteOn(int cmd, int pitch, int velocity, int bpm) {
    //convert current tempo to ms
    int msBPM = 60000 / bpm;
    //TODO: make the tempo beat storatge smarter
    //calculate and store 16th note value
    int msBPMSixteen = msBPM / 4;
    //calculate and store 32nd note value
    int msBPMThirtySecond = msBPM / 8;
    //find the remainder ms
    int msRemainder = millis() % msBPMThirtySecond;
    //use the remainder to wait for the next beat
    delay(msRemainder);
    //send it!
    //maxOSCMessage msg("/ant_trigger/");
    //msg.add(cmd);
    
/*
    if(millis() - lastUpdateTime >= UPDATE_TIME_LIMIT){
    lastUpdateTime += UPDATE_TIME_LIMIT;    
        Particle.publish(String("MIDI"), String(cmd)+" "+String(pitch)+" "+String(abs(velocity)));
        Particle.publish(String("MIDI"), String(cmd)+" "+String(pitch)+" "+String(velocity)+" @ "+String(millis()) +"-"+String(msBPMThirtySecond)+"rem: "+String(msRemainder)+ " BPM: "+String(currentBPM));
        delay(msBPMSixteen);
        Particle.publish(String("MIDI"), String(cmd)+" "+String(pitch)+" "+String(abs(velocity)));
    }
*/
}

void noteOff(int cmd, int pitch, int velocity, int bpm) {
        // Serial.write(cmd);
        // Serial.write(pitch);
        // Serial.write(velocity);
        //turn off that note
        //Particle.publish(String("MIDI"), String(cmd)+" "+String(pitch)+" "+String(velocity));
}
