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
                while (devices[j].available() > 0){ //TODO: bug this blocks flow to first device
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
                    
                    // Initial setup for min/max values per device
                    if (deviceData[j].bSetupComplete){
                        deviceData[j].deltaValues = updateDeltaValues(deviceData[j].deviceValues, deviceData[j].lastDeviceValues);
                        updateMinMaxValues(j, deviceData[j].deviceValues);
                        
                        for (std::size_t k = 0; k < deviceData[j].numberOfSensors; k++){
                            if (std::abs(deviceData[j].deltaValues[k]) > 10){
                                std::cout << "BANG: 10 | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k] << std::endl;
                                outputDeviceValueOSC(j);

                            } else if (std::abs(deviceData[j].deltaValues[k]) > 5){
                                std::cout << "BANG: 5 | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k] << std::endl;
                                outputDeviceValueOSC(j);
                            } else if (std::abs(deviceData[j].deltaValues[k]) >  3){
                                std::cout << "BANG: 3 | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k] << std::endl;
                                outputDeviceValueOSC(j);
                            }
                        }
                    } else if (!deviceData[j].bSetupComplete){
                        deviceData[j].deviceValuesMin.resize(deviceData[j].numberOfSensors);
                        deviceData[j].deviceValuesMax.resize(deviceData[j].numberOfSensors);
                        deviceData[j].deltaValues.resize(deviceData[j].numberOfSensors);
                        deviceData[j].lastDeviceValues.resize(deviceData[j].numberOfSensors);
                        for (std::size_t k = 0; k < deviceData[j].numberOfSensors; k++){
                            deviceData[j].deviceValuesMin[k] = 1023;
                            deviceData[j].deviceValuesMax[k] = 0;
                            deviceData[j].deltaValues[k] = 0;
                            deviceData[j].lastDeviceValues[k] = 0;
                        }
                        updateMinMaxValues(j, deviceData[j].deviceValues);
                        deviceData[j].bSetupComplete = true;
                    } else {
                        ofLogError("ofApp::update") << "Setup state issue.";
                    }
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

void ofApp::updateMinMaxValues(int deviceID, std::vector<int> value){
    if (value.size() == deviceData[deviceID].numberOfSensors){
        for (std::size_t k = 0; k < value.size(); k++) {
            if (value[k] > deviceData[deviceID].deviceValuesMax[k]){
                deviceData[deviceID].deviceValuesMax[k] = value[k];
            }
            if (value[k] < deviceData[deviceID].deviceValuesMin[k]){
                deviceData[deviceID].deviceValuesMin[k] = value[k];
            }
        }
    } else {
        ofLogError("Update MinMax: ") << "Mismatched sizes.";
    }
}

void ofApp::outputDeviceValueOSC(int deviceID){
    // pitch = Delta
    // velocity = abs(Delta) mapped 0->127
    std::vector<int> pitchValues;
    std::vector<int> velocityValues;
    pitchValues.resize(deviceData[deviceID].numberOfSensors);
    velocityValues.resize(deviceData[deviceID].numberOfSensors);
    
    for (std::size_t k = 0; k < deviceData[deviceID].numberOfSensors; k++) {
        pitchValues[k] = deviceData[deviceID].deltaValues[k];
        velocityValues[k] = std::abs(scale(deviceData[deviceID].deltaValues[k], deviceData[deviceID].deviceValuesMin[k], deviceData[deviceID].deviceValuesMax[k], 0, 127));
    }
    
    // set note length and bpm
    
    // Send received byte via OSC to server
    ofxOscMessage m;
    m.setAddress("/device" + to_string(deviceID));
    m.addStringArg("Pitch: ");
    for (std::size_t k = 0; k < deviceData[deviceID].numberOfSensors; k++){
        m.addIntArg(pitchValues[k]);
    }
    m.addStringArg("Velocity: ");
    for (std::size_t k = 0; k < deviceData[deviceID].numberOfSensors; k++){
        m.addIntArg(velocityValues[k]);
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

float ofApp::scale(float in, float inMin, float inMax, float outMin, float outMax){
    long double percentage = (in-inMin)/(inMin-inMax);
    return (percentage) * (outMin-outMax)+outMin;
}

//--------------------------------------------------------------
void ofApp::draw(){
    for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
        ofSetColor(255, 255, 255); //white
        ofDrawBitmapStringHighlight("Ants found on port:  " + devices[j].port(), 20, (j * 20) + 20);
        std::stringstream deltas;
        std::copy(deviceData[j].deltaValues.begin(), deviceData[j].deltaValues.end(), std::ostream_iterator<int>(deltas, " "));
        std::string s = deltas.str();
        s = s.substr(0, s.length()-1);
        ofDrawBitmapString(deltas.str().c_str(), 20, (j * 20) + 100);
        ofDrawBitmapStringHighlight("Number of senors: " + std::to_string(deviceData[j].numberOfSensors), ofGetWidth()/2, (j * 20) + 100);
        
        for (std::size_t k = 0; k < deviceData[j].numberOfSensors; k++){
            ofSetColor(255, 255, 255); //white
            ofDrawCircle((k * 25) + 20, (j * 20) + 200, 5); //exterior
            ofSetColor(0, 0, 0); //black
            ofDrawCircle((k * 25) + 20, (j * 20) + 200, 4); //interior
            if (std::abs(deviceData[j].deltaValues[k]) > 0){
                ofSetColor(std::abs(deviceData[j].deltaValues[k] * 50), 0, 0);
                ofDrawCircle((k * 25) + 20, (j * 20) + 200, 3); //value
            }
        }
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
