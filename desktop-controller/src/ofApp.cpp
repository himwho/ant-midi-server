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
        deviceData.resize(numberOfConnectedDevices);
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
        // TODO: Change this to check all available devices
        if (devices.size() > 0) {
            for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
                while (devices[j].available() > 0){
                    // Read all bytes from the devices;
                    std::vector<uint8_t> buffer;
                    buffer = devices[j].readBytesUntil(); //TODO: find out device range and size for buffer properly
                    std::string str(buffer.begin(), buffer.end());
                    std::cout << "Device [" << j << "]: " << str << std::endl;
                    receivedData[j] = str;
                    
                    // Convert string and set array of values
                    std::vector<int> tempVector = convertStrtoVec(receivedData[j]);
                    deviceData[j].deviceValues = tempVector;
                    deviceData[j].numberOfSensors = deviceData[j].deviceValues.size();
                    
                    deviceData[j].deltaValues = updateDeltaValues(deviceData[j].deviceValues, deviceData[j].lastDeviceValues);
                    updateMinMaxValues(j, deviceData[j].deviceValues);
                    
                    outputDeviceValueOSC(j);
                }
                
                // Set next lastDeviceValue
                deviceData[j].lastDeviceValues = deviceData[j].deviceValues;
                
                // Send next message of current frame
                devices[j].writeByte((unsigned char)ofGetFrameNum());
                devices[j].writeByte('\n');
            }
        }
        
    } catch (const std::exception& exc) {
        ofLogError("ofApp::update") << exc.what();
    }
}

std::vector<int> ofApp::updateDeltaValues(std::vector<int> value, std::vector<int> lastValue){
    std::vector<int> delta;
    // Check that the size of vectors match otherwise skip this for safety
    if (value.size() == lastValue.size()){
        delta.resize(value.size());
        for (std::size_t j = 0; j < value.size(); j++) {
            delta[j] = value[j] - lastValue[j];
        }
    } else {
        ofLogError("Update Delta: ") << "Mismatched sizes.";
    }
    return delta;
}

std::vector<int> ofApp::updateMinMaxValues(int deviceID, std::vector<int> value){
    for (std::size_t k = 0; k < value.size(); k++) {
        if (value[k] > deviceData[deviceID].deviceValuesMax[k]){
            deviceData[deviceID].deviceValuesMax[k] = value[k];
        }
        if (value[k] < deviceData[deviceID].deviceValuesMin[k]){
            deviceData[deviceID].deviceValuesMin[k] = value[k];
        }
    }
}

void ofApp::outputDeviceValueOSC(int deviceID){
    // pitch = Delta
    // velocity = abs(Delta) mapped 0->127
    
    // Send received byte via OSC to server
    ofxOscMessage m;
    m.setAddress("/device" + to_string(deviceID));
    for (std::size_t k = 0; k < deviceData[deviceID].deltaValues.size(); k++){
        m.addIntArg(deviceData[deviceID].deltaValues[k]);
    }
    sender.sendMessage(m, false);
}

std::vector<int> ofApp::convertStrtoVec(string str){
    std::stringstream ss(str);
    std::vector<int> vector;

    int tmp;
    while(ss >> tmp)
    {
        vector.push_back(tmp);
    }
    return vector;
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
