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
                foundDevicesArray.push_back(i);
                receivedData.resize(numberOfConnectedDevices);
            } else {
                std::cout << "Port Position: " << i << " probably has no ants." << '\n';
            }
        }
        for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
            bool success = devices[j].setup(devicesInfo[foundDevicesArray[j]], 115200);
            if (success) {
                ofLogNotice("ofApp::setup") << "Successfully setup " << devicesInfo[foundDevicesArray[j]];
            } else {
                ofLogNotice("ofApp::setup") << "Unable to setup " << devicesInfo[foundDevicesArray[j]];
            }
        }
        ofLogNotice("Number of Discovered Devices: ") << numberOfConnectedDevices << " Number Setup: " << devices.size();
        for (std::vector<int>::const_iterator i = foundDevicesArray.begin(); i != foundDevicesArray.end(); ++i){
            std::cout << *i << ' ';
        }
        std::cout << "< Array of Devices" << std::endl;
    } else {
        ofLogNotice("ofApp::setup") << "No devices connected.";
    }
    
    // open an outgoing connection to HOST:PORT
    sender.setup(HOST, PORT);
    ofLogNotice("ofApp::setup") << "OSC Host: " << HOST;
    ofLogNotice("ofApp::setup") << "OSC Port: " << PORT;
}

//--------------------------------------------------------------
void ofApp::update(){
    // The serial device can throw exeptions.
    try {
        // Read all bytes from the devices;
        // TODO: Change this to check all available devices
        if (devices.size() > 0) {
            for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
                uint8_t buffer[1024];
                std::size_t sz = devices[j].readBytes(buffer, 1024);
                
                for (std::size_t i = 0; i < sz; ++i) {
                    std::cout << buffer[i];
                    ss[j] << buffer[i];
                    ss[j] >> receivedData[j];
                }
                std::cout << "Device [" << j << "]: " << receivedData[j];

                // Send some new bytes to the device to have them echo'd back.
                std::string text = ofToString("a");
                ofx::IO::ByteBuffer textBuffer(text);
                devices[j].writeBytes(textBuffer);
                devices[j].writeByte('\n');
                
                // Send received byte via OSC to server
                ofxOscMessage m;
                m.setAddress("/device" + to_string(j));
                m.addStringArg(receivedData[j]);
                sender.sendMessage(m, false);
            }
        }
        
    } catch (const std::exception& exc) {
        ofLogError("ofApp::update") << exc.what();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
        ofDrawBitmapStringHighlight("Ants found on port:  " + devices[j].port(), 20, (j * 20) + 20);
        ofDrawBitmapStringHighlight(receivedData[j], 20, (j * 20) + 100);
    }
    ofDrawBitmapStringHighlight("FPS: " + std::to_string(ofGetFrameRate()), ofGetWidth() - 250, 20);
    ofDrawBitmapStringHighlight("Frame Number: " + std::to_string(ofGetFrameNum()), ofGetWidth() - 250, 40);
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
