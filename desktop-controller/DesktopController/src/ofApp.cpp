#include "ofApp.h"
#include <vector>

//--------------------------------------------------------------
void ofApp::setup(){
    ofLogNotice("ofApp::setup") << "Connected Devices: ";

    for (auto& device: ofxIO::SerialDeviceUtils::listDevices()) {
        ofLogNotice("ofApp::setup") << "\t" << device;
    }
    
    std::vector<ofx::IO::SerialDeviceInfo> devicesInfo = ofx::IO::SerialDeviceUtils::listDevices();
        
    if (!devicesInfo.empty()) {
        for (std::size_t i = 0; i < devicesInfo.size(); ++i) {
            if (devicesInfo[i].port().find("usbmodem") != std::string::npos) {
                std::cout << "Port Position: " << i << " contains ants!" << '\n';
                numberOfConnectedDevices++; // Add to count of discovered habs
                devices.resize(numberOfConnectedDevices); // Resize devices for number of discovered habs
                for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
                    bool success = devices[j].setup(devicesInfo[i], 115200);
                    if (success) {
                        ofLogNotice("ofApp::setup") << "Successfully setup " << devicesInfo[i];
                    } else {
                        ofLogNotice("ofApp::setup") << "Unable to setup " << devicesInfo[i];
                    }
                }
            } else {
                std::cout << "Port Position: " << i << " probably has no ants." << '\n';
            }
        }
        ofLogNotice("Number of Discovered Devices: ") << numberOfConnectedDevices << " Number Setup: " << devices.size();
    } else {
        ofLogNotice("ofApp::setup") << "No devices connected.";
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    // The serial device can throw exeptions.
    try {
        // Read all bytes from the devices;
        uint8_t buffer[1024];

        // TODO: Change this to check all available devices
        if (devices.size() > 0) {
            while (devices[0].available() > 0) { // While at least the first device is available
                for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
                    std::size_t sz = devices[j].readBytes(buffer, 1024);

                    for (std::size_t i = 0; i < sz; ++i) {
                        std::cout << "Device[" << j << "]: " << buffer[i];
                    }
                    
                    // Send some new bytes to the device to have them echo'd back.
                    // TODO: Use this to ensure handshake comms
                    std::string text = "Frame Number: " + ofToString(ofGetFrameNum());
                    ofx::IO::ByteBuffer textBuffer(text);
                    devices[j].writeBytes(textBuffer);
                    devices[j].writeByte('\n');
                }
            }
        }
        
    } catch (const std::exception& exc) {
        ofLogError("ofApp::update") << exc.what();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
        ofDrawBitmapStringHighlight("Connected to " + devices[j].port(), 20, 20);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
