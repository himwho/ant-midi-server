#include "ofApp.h"
#include <vector>
#include <string>
#include <iostream>
#include <ostream>
#include <sstream>

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
        // create 'n' number of device structs
        deviceData[numberOfConnectedDevices];
        receivedData.resize(numberOfConnectedDevices);
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
                // Send next message of current frame
                devices[j].writeByte((unsigned char)ofGetFrameNum());
                devices[j].writeByte('\n');
                
                std::vector<uint8_t> buffer;
                buffer = devices[j].readBytesUntil();
                
                std::string str(buffer.begin(), buffer.end());
                std::cout << "Device [" << j << "]: " << str << std::endl;
                receivedData[j] = str;
                
                // Convert string and set array of values
                deviceData[j].deviceValues = convertStrtoArr(receivedData[j]);
                deviceData[j].numberOfSensors = deviceData[j].deviceValues.size();
                
                updateDeviceValue(deviceData[j].deviceValues);
                updateVelocityValue(deviceData[j].deviceValues, deviceData[j].lastDeviceValues);
                updatePitchValue(deviceData[j].deviceValues, deviceData[j].lastDeviceValues);

                // Send received byte via OSC to server
                ofxOscMessage m;
                m.setAddress("/device" + to_string(j));
                m.addStringArg(receivedData[j]);
                sender.sendMessage(m, false);
                
                // Set next lastDeviceValue
                deviceData[j].lastDeviceValues = deviceData[j].deviceValues;
            }
        }
        
    } catch (const std::exception& exc) {
        ofLogError("ofApp::update") << exc.what();
    }
}

float ofApp::updateDeviceValue(std::vector<int> value){
    
}

float ofApp::updateVelocityValue(std::vector<int> value, std::vector<int> lastValue){
    //return abs(value - lastValue);
}

float ofApp::updatePitchValue(std::vector<int> value, std::vector<int> lastValue){
    //return value - lastValue;
}

void ofApp::outputDeviceValueOSC(std::vector<int> deltaValue, std::vector<int> deltaValueSigned){
    
}

std::vector<int> ofApp::convertStrtoArr(string str){
    int str_length = str.length();
  
    // create an array with size as string
    // length and initialize with 0
    int arr[str_length] = { 0 };
  
    int j = 0, i, sum = 0;
  
    // Traverse the string
    for (i = 0; str[i] != '\0'; i++) {
  
        // if str[i] is ', ' then split
        if (str[i] == ',')
            continue;
         if (str[i] == ' '){
            // Increment j to point to next
            // array location
            j++;
        } else {
            // subtract str[i] by 48 to convert it to int
            // Generate number by multiplying 10 and adding
            // (int)(str[i])
            arr[j] = arr[j] * 10 + (str[i] - 48);
        }
    }
  
    for (i = 0; i <= j; i++) {
        cout << arr[i] << " ";
    }
    
    std::vector<int> dest(std::begin(arr), std::end(arr));
    for (int i: dest) {
        std::cout << i << " ";
    }
    return dest;
}

//--------------------------------------------------------------
void ofApp::draw(){
    for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
        ofDrawBitmapStringHighlight("Ants found on port:  " + devices[j].port(), 20, (j * 20) + 20);
        ofDrawBitmapString(receivedData[j], 20, (j * 20) + 100);
        //ofDrawBitmapStringHighlight("Number of senors: " + std::to_string(deviceData[j].numberOfSensors), 20, (j * 20) + 120);
    }
    ofDrawBitmapStringHighlight("FPS: " + std::to_string(ofGetFrameRate()), 20, ofGetHeight() - 20);
    ofDrawBitmapStringHighlight("Frame Number: " + std::to_string(ofGetFrameNum()), 20, ofGetHeight() - 40);
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
