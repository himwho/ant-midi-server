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

// SERVER HOST (aka ip address)
#define HOST "3.94.213.186"
// OSC PORT
#define OSCPORT 9998
// SECURITY PORT
#define SECPORT 9995

class OSCPlayerObject: public ofThread{
public:    
    bool playing;
    bool played;
    int deviceId, sensorId, value, lastvalue, valuemin, valuemax, msBPM, chan;
    ofxOscSender sender;
    
    OSCPlayerObject(){
        playing = false;
        // open an outgoing connection to HOST:PORT
        sender.setup(HOST, OSCPORT);
    }
    
    ~OSCPlayerObject(){
        stop();
        waitForThread(true);
    }
    
    void outputDeviceValueOSC(int deviceID, int sensorID, int deviceValue, int lastDeviceValue, int deviceValueMin, int deviceValueMax, int bpm, int channel){
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
        fvelocity = ofMap(value, valuemin, valuemax, 0, 127, true);
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
        
        int msHoldNote = std::abs(value - lastvalue) * msBPM;
        oscNoteOff(deviceId, sensorId, msHoldNote, chan, pitch);
        played = true;
        stop();
    }
};

#endif /* OSCPlayer_h */
