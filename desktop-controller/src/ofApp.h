#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOsc.h"
#include "OSCPlayer.h"
#include "VideoHandler.h"
#include "Hash.h"
#include <memory>

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

        // SETTINGS WINDOW (second GLFW window, see main.cpp)
        void drawSettings(ofEventArgs& args);
        void mousePressedSettings(ofMouseEventArgs& args);

        // SERIAL SETUP
        std::vector<ofSerial> devices;
        std::vector<std::string> devicePorts;
        int numberOfConnectedDevices = 0;
        bool bInitialSetupComplete = false;
        std::vector<std::string> receivedData;
        std::vector<int> foundDevicesArray;
        bool bInitialRunComplete = false;
        int initialRunCount = 0;
        std::string readBytesUntilNewline(ofSerial& serial, char until = '\n');

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
        std::vector<std::unique_ptr<OSCPlayerObject>> oscPlayers;
        ofxOscSender hashSender;  // OSC sender for hash messages
    
        // VIDEO SETUP
        // One VideoHandler per detected camera (index == OS device id).
        std::vector<std::unique_ptr<VideoHandler>> videos;
        std::vector<int> displayOrder; // indices into videos, in display order
        void reconcileCameras(); // open/close grabbers to match user settings
    
        // CV SETUP (one state per camera so several CV cameras can coexist)
        struct CamCV {
            ofPixels previous;
            ofImage diff;
            ofxCv::ContourFinder contourFinder;
            std::vector<ofPoint> lastCenter;
            float lowestVelocityX = 999, lowestVelocityY = 999;
            float highestVelocityX = 0, highestVelocityY = 0;
        };
        std::vector<std::unique_ptr<CamCV>> camCV; // parallel to videos
        void runCV(int camIndex);
        bool showLabels;

        // LAYOUT / SETTINGS
        // 0 = auto grid, otherwise a fixed number of columns
        int layoutColumns = 0;
        void loadSettings();
        void saveSettings();
        std::vector<int> enabledCameraIndices() const;

        // settings window hit-testing (rebuilt every drawSettings frame)
        struct UIHit {
            ofRectangle rect;
            enum Action { LayoutButton, ToggleEnabled, ToggleCV, ToggleName, MoveUp, MoveDown } action;
            int value = 0; // layout columns, camera index, or display-order position
        };
        std::vector<UIHit> uiHits;
        bool camerasDirty = false; // set by settings clicks, applied in update()
        uint64_t lastReconcileMillis = 0;
        uint64_t lastFpsLogMillis = 0;
    
        // LOG SETUP
        void writeToLog(int deviceID);
        
        // HASH GENERATION SETUP
        HashObject hashGenerator;
        std::string currentHash;
};
