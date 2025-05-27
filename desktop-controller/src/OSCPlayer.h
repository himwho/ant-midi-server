//
//  OSCPlayer.h
//  desktop-controller
//
//  Created by Dylan Marcus on 8/2/20.
//

#ifndef OSCPlayer_h
#define OSCPlayer_h

#include "ofMain.h"
#include "ofxMidiConstants.h"
#include "ofxOsc.h"
#include <atomic>

#define LOGMIDI
//#define FULLDEBUG

// send host (aka ip address)
//#define IPHOST "127.0.0.1"
#define IPHOST "100.25.29.52"

/// send port
#define PORT 9998

class OSCPlayerObject: public ofThread{
public:    
    bool playing;
    bool played = false;
    int deviceId, sensorId, value, lastvalue, valuemin, valuemax, msBPM, chan;
    ofxOscSender sender;
    
    OSCPlayerObject(){
        playing = false;
        // open an outgoing connection to IPHOST:PORT
        sender.setup(IPHOST, PORT);
    }
    
    ~OSCPlayerObject(){
        stop();
        waitForThread(false);
    }
    
    void outputDeviceValueOSC(int deviceID, int sensorID, int deviceValue, int lastDeviceValue, int deviceValueMin, int deviceValueMax, int bpm, int channel){
#if defined(FULLDEBUG) || defined(LOGMIDI)
        std::cout << "[MIDI] ON | Device: " << deviceID << ", Sensor: " << sensorID << ", Value: " << deviceValue << ", LastValue: " << lastDeviceValue << ", MinValue: " << deviceValueMin << ", MaxValue: " << deviceValueMax << ", BPM: " << bpm << ", Channel: " << channel << std::endl;
#endif
        this->deviceId = deviceID;
        this->sensorId = sensorID;
        this->value = deviceValue;
        this->lastvalue = lastDeviceValue;
        this->valuemin = deviceValueMin;
        this->valuemax = deviceValueMax;
        this->msBPM = 60000/bpm/16;
        this->chan = channel;
        playing = true;
        startThread();
    }

    void oscNoteOff(int deviceID, int sensorID, float timeHeld, int channel, int pitch){
        // Delay for `seconds`
        sleep(timeHeld);
        
        // Send stop message
        ofxOscMessage m;
        m.setAddress("/device" + to_string(deviceID));
        std::vector<unsigned char> rawMessage;
        rawMessage.push_back(MIDI_NOTE_OFF+(channel-1));
        rawMessage.push_back(pitch);
        rawMessage.push_back(0);
        m.addInt32Arg(rawMessage[0]);
        m.addInt32Arg(rawMessage[1]);
        m.addInt32Arg(rawMessage[2]);
        sender.sendMessage(m, false);
        
#if defined(FULLDEBUG) || defined(LOGMIDI)
        std::cout << "[MIDI] OFF | Device: " << deviceID << ", Sensor: " << sensorID << ", Held: " << timeHeld << ", Pitch: " << pitch << ", Channel: " << channel << std::endl;
#endif
#if defined(FULLDEBUG) || defined(LOGMIDI)
        std::cout << "[MIDI] RAW MESSAGE: " << m.getAddress() << ", " << m.getArgAsInt(0) << ", " << m.getArgAsInt(1) << ", " << m.getArgAsInt(2) << std::endl;
#endif
        
        played = true;
    }

    void stop(){
        stopThread();
    }

    /// Everything in this function will happen in a different
    /// thread which leaves the main thread completelty free for
    /// other task;s.
    void threadedFunction(){
        // Locally set the pitch and velocity of the bang from the input device/sensor
        float fpitch, fvelocity;
        fpitch = ofMap(value, valuemin, valuemax, 42, 100, true);
        fvelocity = ofMap(value, valuemin, valuemax, 20, 127, true);
        int pitch = (int) fpitch;
        int velocity = (int) fvelocity;

        // Send received byte via OSC to server
        ofxOscMessage m;
        m.setAddress("/device" + to_string(deviceId));
        std::vector<unsigned char> rawMessage;
        rawMessage.push_back(MIDI_NOTE_ON+(chan-1));
        rawMessage.push_back(pitch);
        rawMessage.push_back(velocity);
        m.addInt32Arg(rawMessage[0]);
        m.addInt32Arg(rawMessage[1]);
        m.addInt32Arg(rawMessage[2]);
        sender.sendMessage(m, false);
        
#if defined(FULLDEBUG) || defined(LOGMIDI)
        std::cout << "[MIDI] RAW MESSAGE: " << m.getAddress() << ", " << m.getArgAsInt(0) << ", " << m.getArgAsInt(1) << ", " << m.getArgAsInt(2) << std::endl;
#endif

        int msHoldNote = std::abs(value - lastvalue) * msBPM/4; //div 4 is a temp reducer
        oscNoteOff(deviceId, sensorId, msHoldNote, chan, pitch);
        stop();
    }
};

#endif /* OSCPlayer_h */
