#include "ofApp.h"
#include <vector>
#include <string>
#include <iostream>
#include <ostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <future>
#include <math.h>
#include <cctype>
#include "ofxMidiConstants.h"

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
        for (int mult = 0; mult < 3; mult++){
            for (int deviceID = 0; deviceID < numberOfConnectedDevices; deviceID++){
                setupDevice(deviceID);
            }
        }
        bInitialSetupComplete = true;
    } else {
        ofLogNotice("ofApp::setup") << "No devices connected.";
    }
    camWidth = 640;  // try to grab at this size.
    camHeight = 480;

    //get back a list of devices.
    vector<ofVideoDevice> cameras = vidGrabber.listDevices();

    for(size_t i = 0; i < cameras.size(); i++){
        if(cameras[i].bAvailable){
            //log the device
            ofLogNotice() << cameras[i].id << ": " << cameras[i].deviceName;
        }else{
            //log the device and note it as unavailable
            ofLogNotice() << cameras[i].id << ": " << cameras[i].deviceName << " - unavailable ";
        }
    }

    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(60);
    vidGrabber.initGrabber(camWidth, camHeight);
    ofSetVerticalSync(true);
}

void ofApp::setupDevice(int deviceID){
    // Initial setup for min/max values per device
    // Send next message of current frame
    devices[deviceID].writeByte((unsigned char)ofGetFrameNum());
    devices[deviceID].writeByte('\n');
    std::this_thread::sleep_for( std::chrono::seconds{(long)0.01});

    // Read all bytes from the devices;
    std::vector<uint8_t> buffer;
    buffer = devices[deviceID].readBytesUntil(); //TODO: find out device range and size for buffer properly
    std::string str(buffer.begin(), buffer.end());
    receivedData[deviceID] = str;
    
    // Convert string and set array of values
    std::vector<int> tempVector = convertStrtoVec(receivedData[deviceID]);
    deviceData[deviceID].deviceValues = tempVector;
    deviceData[deviceID].numberOfSensors = deviceData[deviceID].deviceValues.size();

    for (int k = 0; k < deviceData[deviceID].numberOfSensors; k++){
        deviceData[deviceID].deviceValues.push_back(0);
        deviceData[deviceID].deviceValuesMin.push_back(1023);
        deviceData[deviceID].deviceValuesMax.push_back(0);
        deviceData[deviceID].deltaValues.push_back(0);
        deviceData[deviceID].lastDeviceValues.push_back(0);
    }
    deviceData[deviceID].bSetupComplete = true;
}

//--------------------------------------------------------------
void ofApp::update(){
    // The serial device can throw exeptions.
    try {
        if (bInitialSetupComplete && devices.size() > 0) {
            for (int j = 0; j < numberOfConnectedDevices; j++) {
                if (deviceData[j].bSetupComplete){
                    while (devices[j].available() > 0){ //TODO: bug this blocks flow to first device
                        // Send next message of current frame
                        devices[j].writeByte((unsigned char)ofGetFrameNum());
                        devices[j].writeByte('\n');
                        std::this_thread::sleep_for( std::chrono::seconds{(long)0.01});
                        
                        // Read all bytes from the devices;
                        std::vector<uint8_t> buffer;
                        buffer = devices[j].readBytesUntil(); //TODO: find out device range and size for buffer properly
                        std::string str(buffer.begin(), buffer.end());
                        receivedData[j] = str;
                        
                        // Convert string and set array of values
                        std::vector<int> tempVector = convertStrtoVec(receivedData[j]);
                        deviceData[j].deviceValues = tempVector;
                        if (tempVector.size() == deviceData[j].numberOfSensors){
                            updateDeltaValues(j, deviceData[j].deviceValues, deviceData[j].lastDeviceValues);
                            updateMinMaxValues(j, deviceData[j].deviceValues);
                            for (int k = 0; k < deviceData[j].numberOfSensors; k++){
                                if (std::abs(deviceData[j].deltaValues[k]) > 10){
                                    //std::cout << "BANG: 10 | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k] << std::endl;
                                    oscPlayers.push_back(move(unique_ptr<OSCPlayerObject>(new OSCPlayerObject)));
                                    oscPlayers.back()->outputDeviceValueOSC(j, k, deviceData[j].deviceValues[k], deviceData[j].lastDeviceValues[k], deviceData[j].deviceValuesMin[k], deviceData[j].deviceValuesMax[k], 120, j+1);
                                } else if (std::abs(deviceData[j].deltaValues[k]) > 5){
                                    //std::cout << "BANG: 5  | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k] << std::endl;
                                    oscPlayers.push_back(move(unique_ptr<OSCPlayerObject>(new OSCPlayerObject)));
                                    oscPlayers.back()->outputDeviceValueOSC(j, k, deviceData[j].deviceValues[k], deviceData[j].lastDeviceValues[k], deviceData[j].deviceValuesMin[k], deviceData[j].deviceValuesMax[k], 120, j+1);
                                } else if (std::abs(deviceData[j].deltaValues[k]) >  3){
                                    //std::cout << "BANG: 3  | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k] << std::endl;
                                }
                            }
                            // Set next lastDeviceValue
                            deviceData[j].lastDeviceValues = deviceData[j].deviceValues;
                        } else {
                            // had a read error and resetting for setup check
                            setupDevice(j);
                        }
                        for (int livePlayers = 0; livePlayers < oscPlayers.size(); livePlayers++){
                            if (oscPlayers[livePlayers]->played){
                                oscPlayers.erase(oscPlayers.begin() + livePlayers);
                            }
                        }
                    }
                }
            }
        }
    } catch (const std::exception& exc) {
        ofLogError("ofApp::update") << exc.what();
    }
    ofBackground(100, 100, 100);
    vidGrabber.update();

    if(vidGrabber.isFrameNew()){
        ofPixels & pixels = vidGrabber.getPixels();
        videoTexture.loadData(pixels);
    }
}

void ofApp::updateDeltaValues(int deviceID, std::vector<int> values, std::vector<int> lastValues){
    // Check that the size of vectors match otherwise skip this for safety
    if (values.size() == deviceData[deviceID].numberOfSensors){
        deviceData[deviceID].deviceValues.resize(deviceData[deviceID].numberOfSensors);
        deviceData[deviceID].lastDeviceValues.resize(deviceData[deviceID].numberOfSensors);
        deviceData[deviceID].deltaValues.resize(deviceData[deviceID].numberOfSensors);
        for (int k = 0; k < values.size(); k++) {
            deviceData[deviceID].deltaValues[k] = values[k] - lastValues[k];
            std::stringstream ss;
            ss << values[k];
            std::string str = ss.str();
            if(isdigit(str[0]))
            {
                ++deviceData[deviceID].digit_frequency[str[0]];
            }
            else if(isdigit(str[1]))
            {
                ++deviceData[deviceID].digit_frequency[str[1]];
            }
            std::map<char, int>::iterator it;
            for(it = deviceData[deviceID].digit_frequency.begin(); it != deviceData[deviceID].digit_frequency.end(); ++it) {
                std::cout << "Number " << it->first << " occurred " << it->second << " time(s).\n";
            }
        }
    } else {
        setupDevice(deviceID);
        ofLogError("Update Delta: ") << "Mismatched sizes.";
    }
}

void ofApp::updateMinMaxValues(int deviceID, std::vector<int> values){
    // Check that the size of vectors match otherwise skip this for safety
    if (values.size() == deviceData[deviceID].numberOfSensors){
        deviceData[deviceID].deviceValuesMin.resize(deviceData[deviceID].numberOfSensors);
        deviceData[deviceID].deviceValuesMax.resize(deviceData[deviceID].numberOfSensors);
        for (int k = 0; k < values.size(); k++) {
            if (values[k] > deviceData[deviceID].deviceValuesMax[k]){
                deviceData[deviceID].deviceValuesMax[k] = values[k];
            }
            if (values[k] < deviceData[deviceID].deviceValuesMin[k]){
                deviceData[deviceID].deviceValuesMin[k] = values[k];
            }
        }
    } else {
        setupDevice(deviceID);
        ofLogError("Update MinMax: ") << "Mismatched sizes.";
    }
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
    // Set background video input
    ofSetHexColor(0xffffff);
    vidGrabber.draw(0, ofGetHeight()-480);
    videoTexture.draw(20 + camWidth, 20, camWidth, camHeight);
    
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
    ofDrawBitmapStringHighlight("Number of Threads: " + std::to_string(oscPlayers.size()), 20, ofGetHeight() - 60);
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

//--------------------------------------------------------------
void ofApp::exit(){
    for (int i = 0; i < oscPlayers.size(); i++){
        oscPlayers[i]->stopThread();
        oscPlayers[i]->stop();
    }
    oscPlayers.clear();
}
