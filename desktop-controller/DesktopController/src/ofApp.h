#pragma once

#include "ofMain.h"
#include "ofxSerial.h"
#include "ofxOsc.h"

// send host (aka ip address)
#define HOST "localhost"

/// send port
#define PORT 12345

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

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
        std::vector<int> foundDevicesArray;
    
        // OSC SETUP
        ofxOscSender sender;
};
