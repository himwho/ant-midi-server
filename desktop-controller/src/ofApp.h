#pragma once

#include "ofMain.h"
#include "ofxSerial.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "OSCPlayer.h"
#include "VideoHandler.h"
#include "Hash.h"

#define LOGMIDI
//#define LOGTIME
#define LOGCV
//#define LOGSENSORS
//#define LOGSENSORVALUES
//#define LOGFNL
//#define FULLDEBUG

#define MAX_CONCURRENT_VOICES 15

// DEFAULT TRIGGERS
#define TRIGGER0 15
#define TRIGGER1 8
#define TRIGGER2 5

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
        void exit();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
        // SERIAL SETUP
        std::vector<ofx::IO::SerialDevice> devices;
        int numberOfConnectedDevices = 0;
        bool bInitialSetupComplete = false;
        std::vector<std::string> receivedData;
        std::vector<int> foundDevicesArray;
        bool bInitialRunComplete = false;
        int initialRunCount = 0;

        // UTILITY FUNCTIONS
        std::vector<int> convertStrtoVec(string str);
        float scale(float in, float inMin, float inMax, float outMin, float outMax);
        
        // DEVICE STRUCT
        struct AntDevice {
            bool bSetupComplete = false;
            int numberOfSensors;
            std::vector<int> deviceValues;
            std::vector<int> lastDeviceValues;
            std::vector<int> deviceValuesMin;
            std::vector<int> deviceValuesMax;
            std::vector<int> deltaValues;
            std::vector<int> summedValues;
            std::map<char, int> digit_frequency;
            string deviceNameStr;
            int trigger1 = TRIGGER0;
            int trigger2 = TRIGGER1;
            int trigger3 = TRIGGER2;
        };
    
        std::vector<AntDevice> deviceData;
        void setupDevice(int deviceID);
        void updateDeltaValues(int deviceID, std::vector<int> values, std::vector<int> lastValues);
        void updateMinMaxValues(int deviceID, std::vector<int> values);
    
        // OSC SETUP
        std::vector<unique_ptr<OSCPlayerObject>> oscPlayers;
        ofxOscSender hashSender;  // OSC sender for hash messages
    
        // VIDEO SETUP
        ofVideoGrabber vidGrabber;
        std::vector<unique_ptr<VideoHandler>> videos;
    
        // CV SETUP
        ofPixels previous;
        ofImage diff;
        cv::Scalar diffMean; // color store
        float threshold;
        ofxCv::ContourFinder contourFinder;
        bool showLabels;
        float lowestVelocityX = 999, lowestVelocityY = 999, highestVelocityX = 0, highestVelocityY = 0;
        std::vector<ofPoint> lastCenter;
    
        // LOG SETUP
        void writeToLog(int deviceID);
        
        // HASH GENERATION SETUP
        HashObject hashGenerator;
        std::string currentHash;
};
