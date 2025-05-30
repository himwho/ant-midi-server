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

#include <sys/time.h>
#include <ctime>
using std::cout; using std::endl;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

//--------------------------------------------------------------
void ofApp::setup(){
#ifdef LOGSENSORVALUES
#define ofLogNotice() ofLogNotice() << ofGetTimestampString("[%Y-%m-%d %H:%M:%S.%i] ")
#endif
    ofLogNotice() << "ofApp::setup" << "Connected Devices: ";

    for (auto& device: ofxIO::SerialDeviceUtils::listDevices()) {
        ofLogNotice() << "ofApp::setup" << "\t" << device;
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

        // create 'n' number of device structs
        deviceData.resize(numberOfConnectedDevices);
        receivedData.resize(numberOfConnectedDevices);

        for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
            bool success = devices[j].setup(devicesInfo[foundDevicesArray[j]], 115200);
            if (success) {
                ofLogNotice() << "ofApp::setup" << "Successfully setup " << devicesInfo[foundDevicesArray[j]];
                deviceData[j].deviceNameStr = devicesInfo[foundDevicesArray[j]].port();
            } else {
                ofLogNotice() << "ofApp::setup" << "Unable to setup " << devicesInfo[foundDevicesArray[j]];
            }
        }
        ofLogNotice() << "Number of Discovered Devices: " << numberOfConnectedDevices << " Number Setup: " << devices.size();
        for (std::vector<int>::const_iterator i = foundDevicesArray.begin(); i != foundDevicesArray.end(); ++i){
            std::cout << *i << ' ';
        }
        std::cout << "< Array of Devices" << std::endl;
        for (int mult = 0; mult < 3; mult++){
            for (int deviceID = 0; deviceID < numberOfConnectedDevices; deviceID++){
                setupDevice(deviceID);
            }
        }
        bInitialSetupComplete = true;
    } else {
        ofLogNotice() << "ofApp::setup" << "No devices connected.";
    }
    
    //get back a list of devices.
    vector<ofVideoDevice> cameras = vidGrabber.listDevices();

    for(size_t i = 0; i < cameras.size(); i++){
        if(cameras[i].bAvailable){
            //log the device
            ofLogNotice() << cameras[i].id << ": " << cameras[i].deviceName;
            videos.push_back(move(unique_ptr<VideoHandler>(new VideoHandler)));
            if (cameras[i].deviceName.find("C920") != std::string::npos) {
                videos.back()->setup(i, IPHOST, 10005 + i, 640, 480, true);
                // OpenCV Setup
                // imitate() will set up previous and diff
                // so they have the same size and type as cam
                ofxCv::imitate(previous, videos[i]->vidGrabber);
                ofxCv::imitate(diff, videos[i]->vidGrabber);
            } else {
                // setup a non cv camera
                videos.back()->setup(i, IPHOST, 10005 + i, 640, 480, false);
            }
        }else{
#if defined(FULLDEBUG) || defined(LOGSENSORS)
            //log the device and note it as unavailable
            ofLogNotice() << cameras[i].id << ": " << cameras[i].deviceName << " - unavailable ";
#endif
        }
    }
    contourFinder.setMinAreaRadius(3);
    contourFinder.setMaxAreaRadius(50);
    contourFinder.setThreshold(15);
    contourFinder.getTracker().setPersistence(15);
    contourFinder.getTracker().setMaximumDistance(50);
    showLabels = false;
    
    // Initialize OSC sender for hash messages
    hashSender.setup(IPHOST, PORT);
}

void ofApp::setupDevice(int deviceID){
    // Initial setup for min/max values per device
    // Send next message of current frame
    char sendTriggerByte = 1;
    devices[deviceID].writeByte(sendTriggerByte);
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
    deviceData[deviceID].deviceValues.clear();

    //TODO: programmatically setup trigger ranges by sampling averages for a time period
    if (deviceData[deviceID].deviceNameStr.find("usbmodem1464") != std::string::npos) {
        // force test for wall mounted micro
        // this should be temporary until a better test can be made
        deviceData[deviceID].trigger1 = 50;
        deviceData[deviceID].trigger2 = 35;
        deviceData[deviceID].trigger3 = 20;
    }
    
    for (int k = 0; k < deviceData[deviceID].numberOfSensors; k++){
        deviceData[deviceID].deviceValues.push_back(0);
        deviceData[deviceID].deviceValuesMin.push_back(1023);
        deviceData[deviceID].deviceValuesMax.push_back(0);
        deviceData[deviceID].deltaValues.push_back(0);
        deviceData[deviceID].lastDeviceValues.push_back(0);
    }
    deviceData[deviceID].bSetupComplete = true;
}

auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

//--------------------------------------------------------------
void ofApp::update(){
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::cout << "[TIME] Start of update: " << millisec_since_epoch << std::endl;
#endif
    // The serial device can throw exeptions.
    try {
        if (bInitialSetupComplete && devices.size() > 0) {
            for (int j = 0; j < numberOfConnectedDevices; j++) {
#if defined(FULLDEBUG) || defined(LOGTIME)
                millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                std::cout << "[TIME] Start of first FOR " << j << " loop : " << millisec_since_epoch << std::endl;
#endif
                if (deviceData[j].bSetupComplete){
                    if (devices[j].available() > deviceData[j].numberOfSensors*2){ // TODO: what is this if statement?
                        // Read all bytes from the devices;
                        std::vector<uint8_t> buffer;
                        buffer = devices[j].readBytesUntil(); //TODO: find out device range and size for buffer properly
                        devices[j].flush();
#if defined(FULLDEBUG) || defined(LOGTIME)
                        millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                        std::cout << "[TIME] After readBytesUntil : " << millisec_since_epoch << std::endl;
#endif
                        std::string str(buffer.begin(), buffer.end());
                        receivedData[j] = str;
                        
                        // Convert string and set array of values
                        std::vector<int> tempVector = convertStrtoVec(receivedData[j]);
                        deviceData[j].deviceValues = tempVector;
                        if (tempVector.size() == deviceData[j].numberOfSensors){
                            
                            // dirty removal of wallmount colony's 4th sensor which contains 10 sensors
                            // TODO: fix or remove this sensor and remove the below
                            if (deviceData[j].numberOfSensors == 10) {
                                deviceData[j].deviceValues[3] = 0;
                            }
                            if (deviceData[j].numberOfSensors == 11) {
                                if (std::abs(deviceData[j].deviceValues[9] - deviceData[j].lastDeviceValues[9]) > 20){
                                    deviceData[j].deviceValues[9] = std::abs(deviceData[j].deviceValues[9]*0.1); // reduce the sensitivity to 10%
                                }
                            }
                            
                            updateDeltaValues(j, deviceData[j].deviceValues, deviceData[j].lastDeviceValues);
                            updateMinMaxValues(j, deviceData[j].deviceValues);
                            if (bInitialRunComplete){
                                for (int k = 0; k < deviceData[j].numberOfSensors; k++){
#if defined(FULLDEBUG) || defined(LOGTIME)
                                    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    std::cout << "[TIME] Start of second FOR " << j << " " << k << " loop : " << millisec_since_epoch << std::endl;
#endif
                                    if (std::abs(deviceData[j].deltaValues[k]) > deviceData[j].trigger1){
#if defined(FULLDEBUG) || defined(LOGSENSORS)
                                        ofLogNotice() << "BANG: " <<  deviceData[j].trigger1 << " | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k];
#endif
#ifdef LOGSENSORVALUES
                                        writeToLog(j);
#endif
                                        if (oscPlayers.size() < MAX_CONCURRENT_VOICES){ //block too many triggers
                                            oscPlayers.push_back(move(unique_ptr<OSCPlayerObject>(new OSCPlayerObject)));
                                            oscPlayers.back()->outputDeviceValueOSC(j, k, deviceData[j].deviceValues[k], deviceData[j].lastDeviceValues[k], deviceData[j].deviceValuesMin[k], deviceData[j].deviceValuesMax[k], 130, j+1);
                                        }
                                    } else if (std::abs(deviceData[j].deltaValues[k]) >  deviceData[j].trigger2){
#if defined(FULLDEBUG) || defined(LOGSENSORS)
                                        ofLogNotice() << "BANG: " <<  deviceData[j].trigger2 << "  | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k];
#endif
#ifdef LOGSENSORVALUES
                                        writeToLog(j);
#endif
                                        if (oscPlayers.size() < MAX_CONCURRENT_VOICES){
                                            oscPlayers.push_back(move(unique_ptr<OSCPlayerObject>(new OSCPlayerObject)));
                                            oscPlayers.back()->outputDeviceValueOSC(j, k, deviceData[j].deviceValues[k], deviceData[j].lastDeviceValues[k], deviceData[j].deviceValuesMin[k], deviceData[j].deviceValuesMax[k], 130, j+1);
                                        }
                                    } else if (std::abs(deviceData[j].deltaValues[k]) > deviceData[j].trigger3){
#if defined(FULLDEBUG) || defined(LOGSENSORS)
                                        ofLogNotice() << "BANG: " <<  deviceData[j].trigger3 << "  | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k];
#endif
#ifdef LOGSENSORVALUES
                                        writeToLog(j);
#endif
                                    }
//                                    else if (std::abs(deviceData[j].deltaValues[k]) > 3){ //DEBOUNCE FOR LOGS
//#if defined(FULLDEBUG) || defined(LOGSENSORS)
//                                        ofLogNotice() << "BANG: " << "3" << "  | Device " << j << " | Sensor: " << k << " | Value: " << deviceData[j].deltaValues[k];
//#endif
//#ifdef LOGSENSORVALUES
//                                        writeToLog(j);
//#endif
//                                    }
                                }
                            }
#if defined(FULLDEBUG) || defined(LOGTIME)
                            millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                            cout << "[TIME] End of second FOR loop : " << millisec_since_epoch << endl;
#endif
                            initialRunCount++; // increment initial run count to bounce OSCPlayers until stable
                            if (initialRunCount > 100){
                                bInitialRunComplete = true;
                            }
                            // Set next lastDeviceValue
                            deviceData[j].lastDeviceValues = deviceData[j].deviceValues;
                        } else {
                            // had a read error and resetting for setup check
                            setupDevice(j);
                        }
                        for (int livePlayer = 0; livePlayer < oscPlayers.size(); livePlayer++){
                            if (oscPlayers[livePlayer]->played){
                                oscPlayers.erase(oscPlayers.begin() + livePlayer);
                            }
                        }
                        break;
                    }
                    // Send next message of current frame
                    char sendTriggerByte = 1;
                    devices[j].writeByte(sendTriggerByte);
                }
            }
        }
    } catch (const std::exception& exc) {
#ifdef FULLDEBUG
        ofLogError("ofApp::update") << exc.what();
#endif
    }
    ofBackground(100, 100, 100);
    for (int i = 0; i < videos.size(); i++){
        if (videos[i]->bUseForCV) {
            videos[i]->update();
            if(videos[i]->vidGrabber.isFrameNew()) {
                // take the absolute difference of prev and cam and save it inside diff
                ofxCv::absdiff(videos[i]->vidGrabber, previous, diff);
                diff.update();
                // like ofSetPixels, but more concise and cross-toolkit
                ofxCv::copy(videos[i]->vidGrabber, previous);
                // mean() returns a Scalar. it's a cv:: function so we have to pass a Mat
                diffMean = mean(ofxCv::toCv(diff));
                // you can only do math between Scalars,
                // but it's easy to make a Scalar from an int (shown here)
                diffMean *= cv::Scalar(50);
                ofxCv::blur(diff, 10);
                contourFinder.findContours(diff);
                lastCenter.resize(contourFinder.size());
                for(int j = 0; j < contourFinder.size(); j++) {
                    auto center = ofxCv::toOf(contourFinder.getCenter(j));
                    auto velocity = ofxCv::toOf(contourFinder.getVelocity(j));
                    
                    // only add a new osc player if there is found velocity to the movement
                    if (velocity.x > 0 || velocity.y > 0) {
                        if (std::fabs(velocity.x) < lowestVelocityX) {
                            // cannot allow the mins to become < 1
                            lowestVelocityX = std::max(std::abs((int)velocity.x), 1);
                        }
                        if (std::fabs(velocity.y) < lowestVelocityY) {
                            // cannot allow the mins to become < 1
                            lowestVelocityY = std::max(std::abs((int)velocity.y), 1);
                        }
                        if (std::fabs(velocity.x) > highestVelocityX) {
                            highestVelocityX = std::fabs(velocity.x);
                        }
                        if (std::fabs(velocity.y) > highestVelocityY) {
                            highestVelocityY = std::fabs(velocity.y);
                        }
                        if (oscPlayers.size() < MAX_CONCURRENT_VOICES){ //block too many triggers
                            // TODO: remove this null check by setting up constructor default values
                            float lastValue = 0;
                            if (lastCenter[j].x >= 0) {
                                lastValue = lastCenter[j].x;
                            }
                            oscPlayers.push_back(move(unique_ptr<OSCPlayerObject>(new OSCPlayerObject)));
                            oscPlayers.back()->outputDeviceValueOSC(i, j, (int)center.x, (int)lastValue, (int)std::fabs((float)velocity.x*(127/lowestVelocityX)), (int)std::fabs((float)velocity.x*(127/highestVelocityX)), 130, 0+1);
#if defined(FULLDEBUG) || defined(LOGCV)
                            std::cout << "[MIDI CV] Center : " << center.x << ", " << center.y << std::endl;
                            std::cout << "[MIDI CV] Velocity : " << velocity.x << ", " << velocity.y << std::endl;
#endif
#if defined(FULLDEBUG)
                            std::cout << "[MIDI] Device: " << i << ", Sensor: " << j << ", Value: " << center.x << ", LastValue: " << lastValue << ", MinValue: " << std::fabs((float)velocity.x*(127/lowestVelocityX)) << ", MaxValue: " << std::fabs((float)velocity.x*(127/highestVelocityX)) << ", BPM: " << 130 << ", Channel: " << 0 << std::endl;
#endif
                        }
                        lastCenter[j] = center;
                    }
                }
            }
        }
    }

    // erase played notes that signaled an off
    for (int livePlayer = 0; livePlayer < oscPlayers.size(); livePlayer++){
        if (oscPlayers[livePlayer]->played){
            oscPlayers.erase(oscPlayers.begin() + livePlayer);
        }
    }
    
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] End of update loop : " << millisec_since_epoch << endl;
#endif

    if (hashGenerator.shouldUpdateHash()) {
        currentHash = hashGenerator.generateHash(deviceData, lastCenter);
        
        // Send hash to nodejs server via OSC
        ofxOscMessage m;
        m.setAddress("/hash");
        m.addStringArg(currentHash);
        hashSender.sendMessage(m, false);
    }
}

void ofApp::writeToLog(int deviceID){
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] Start of writeToLog : " << millisec_since_epoch << endl;
#endif
    std::string tabbedValues;
    for (int k = 0; k < deviceData[deviceID].deviceValues.size(); k++) {
#ifdef LOGSENSORVALUES
        tabbedValues += "\t" + std::to_string(deviceData[deviceID].deviceValues[k]);
#endif
#ifdef LOGFNL
        std::stringstream ss;
        ss << deviceData[deviceID].deviceValues[k];
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
            ofLogNotice() << "Number " << it->first << " occurred " << it->second << " time(s).\n";
        }
#endif
    }
#ifdef LOGSENSORVALUES
    ofFile DeviceLog(ofGetTimestampString("%Y-%m-%d")+"-Device"+std::to_string(deviceID)+".txt", ofFile::Append);
    DeviceLog << ofGetTimestampString("[%Y-%m-%d %H:%M:%S.%i] ") << tabbedValues << std::endl;
#endif
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] End of writeToLog : " << millisec_since_epoch << endl;
#endif
}

void ofApp::updateDeltaValues(int deviceID, std::vector<int> values, std::vector<int> lastValues){
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] Start of updateDeltaValues : " << millisec_since_epoch << endl;
#endif
    // Check that the size of vectors match otherwise skip this for safety
    if (values.size() == deviceData[deviceID].numberOfSensors){
        deviceData[deviceID].deviceValues.resize(deviceData[deviceID].numberOfSensors);
        deviceData[deviceID].lastDeviceValues.resize(deviceData[deviceID].numberOfSensors);
        deviceData[deviceID].deltaValues.resize(deviceData[deviceID].numberOfSensors);
        for (int k = 0; k < values.size(); k++) {
            deviceData[deviceID].deltaValues[k] = values[k] - lastValues[k];
        }
    } else {
        setupDevice(deviceID);
#ifdef FULLDEBUG
        ofLogError("Update Delta: ") << "Mismatched sizes.";
#endif
    }
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] End of updateDeltaValues : " << millisec_since_epoch << endl;
#endif
}

void ofApp::updateMinMaxValues(int deviceID, std::vector<int> values){
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] Start of updateMinMaxValues : " << millisec_since_epoch << endl;
#endif
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
#ifdef FULLDEBUG
        ofLogError("Update MinMax: ") << "Mismatched sizes.";
#endif
    }
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] End of updateMinMaxValues : " << millisec_since_epoch << endl;
#endif
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
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] Start of draw : " << millisec_since_epoch << endl;
#endif
    // Set background video input
    ofSetHexColor(0xffffff);
    ofSetBackgroundAuto(true);
    ofxCv::RectTracker& tracker = contourFinder.getTracker();
    
    // VIDEO DISPLAY
    int columnStep = 0; // Starting point for columns
    int i = 0;
    int rowStep = 0; // Starting point for rows
    
    while (rowStep < ofGetHeight()){
        if (i < videos.size()){
            videos[i]->update();
            if (videos[i]->bUseForCV) {
                if (columnStep == 0) {
                    // draw at the starting column x=0
                    videos[i]->image.draw(0, rowStep, videos[i]->camWidth, videos[i]->camHeight);
                    diff.draw(0+videos[i]->camWidth,rowStep,videos[i]->camWidth,videos[i]->camHeight);
                    // flag to the end of the loop to skip this row
                    columnStep += videos[i]->camWidth;
                } else {
                    videos[i]->image.draw(columnStep, rowStep, videos[i]->camWidth, videos[i]->camHeight);
                    // draw underneath it
                    // TODO: figure out how to blank out any upcoming cameras that would cover this on the grid
                    diff.draw(columnStep+videos[i]->camWidth,rowStep+videos[i]->camHeight,videos[i]->camWidth,videos[i]->camHeight);
                }
                ofPushMatrix();
                ofScale( 1, 1 );
                ofTranslate(columnStep,rowStep);
                ofSetColor(255);
                if(showLabels) {
                    contourFinder.draw();
                    for(int i = 0; i < contourFinder.size(); i++) {
                        ofPoint center = ofxCv::toOf(contourFinder.getCenter(i));
                        ofPushMatrix();
                        ofTranslate(center.x, center.y);
                        int label = contourFinder.getLabel(i);
                        string msg = ofToString(label) + ":" + ofToString(tracker.getAge(label));
                        ofDrawBitmapString(msg, 0, 0);
                        ofVec2f velocity = ofxCv::toOf(contourFinder.getVelocity(i));
                        ofScale(5, 5);
                        ofDrawLine(0, 0, velocity.x, velocity.y);
                        ofPopMatrix();
                    }
                } else {
                    for(int i = 0; i < contourFinder.size(); i++) {
                        unsigned int label = contourFinder.getLabel(i);
                        // only draw a line if this is not a new label
                        if(tracker.existsPrevious(label)) {
                            // use the label to pick a random color
                            ofSeedRandom(label << 24);
                            ofSetColor(ofColor::fromHsb(ofRandom(255), 255, 255));
                            // get the tracked object (cv::Rect) at current and previous position
                            const cv::Rect& previous = tracker.getPrevious(label);
                            const cv::Rect& current = tracker.getCurrent(label);
                            // get the centers of the rectangles
                            ofVec2f previousPosition(previous.x + previous.width / 2, previous.y + previous.height / 2);
                            ofVec2f currentPosition(current.x + current.width / 2, current.y + current.height / 2);
                            ofDrawLine(previousPosition, currentPosition);
                            ofDrawRectangle(current.x, current.y, current.width, current.height);
                        }
                        ofPoint center = ofxCv::toOf(contourFinder.getCenter(i));
                        label = contourFinder.getLabel(i);
                        string msg = ofToString(label) + ":" + ofToString(tracker.getAge(label));
                        ofDrawBitmapString(msg, center.x + 20, center.y + 20);
                    }
                }
                
                // this chunk of code visualizes the creation and destruction of labels
                const vector<unsigned int>& currentLabels = tracker.getCurrentLabels();
                const vector<unsigned int>& previousLabels = tracker.getPreviousLabels();
                const vector<unsigned int>& newLabels = tracker.getNewLabels();
                const vector<unsigned int>& deadLabels = tracker.getDeadLabels();
                ofSetColor(ofxCv::cyanPrint);
                for(int i = 0; i < currentLabels.size(); i++) {
                    int j = currentLabels[i];
                    ofDrawLine(j, 0, j, 4);
                }
                ofSetColor(ofxCv::magentaPrint);
                for(int i = 0; i < previousLabels.size(); i++) {
                    int j = previousLabels[i];
                    ofDrawLine(j, 4, j, 8);
                }
                ofSetColor(ofxCv::yellowPrint);
                for(int i = 0; i < newLabels.size(); i++) {
                    int j = newLabels[i];
                    ofDrawLine(j, 8, j, 12);
                }
                ofSetColor(ofColor::white);
                for(int i = 0; i < deadLabels.size(); i++) {
                    int j = deadLabels[i];
                    ofDrawLine(j, 12, j, 16);
                }
                ofPopMatrix();
            } else {
                // resume drawing grid style of remaining cameras
                videos[i]->image.draw(columnStep, rowStep, videos[i]->camWidth, videos[i]->camHeight);
            }
            if (columnStep < ofGetWidth()/2) { // if the starting point is greater than halfway than it is likely the odd even screen
                columnStep += videos[0]->camWidth;
            } else {
                columnStep = 0;
                rowStep += videos[0]->camHeight;
            }
            i += 1;
        } else {
            break;
        }
    }
    
    for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
        ofSetColor(255, 255, 255); //white
        ofDrawBitmapStringHighlight("Ants found on port:  " + devices[j].port(), 20, (j * 20) + 20);
        ofDrawBitmapStringHighlight("Number of senors: " + std::to_string(deviceData[j].numberOfSensors), 20 + 450, (j * 20) + 20);

        std::stringstream deltas;
        std::copy(deviceData[j].deltaValues.begin(), deviceData[j].deltaValues.end(), std::ostream_iterator<int>(deltas, " "));
        std::string s = deltas.str();
        s = s.substr(0, s.length()-1);
        ofDrawBitmapString(deltas.str().c_str(), 20+ 450 + 200, (j * 20) + 20);
        for (std::size_t k = 0; k < deviceData[j].numberOfSensors; k++){
            ofSetColor(255, 255, 255); //white
            ofDrawCircle((k * 25) + 20 + 450 + 200 + 250, (j * 20) + 17, 5); //exterior
            ofSetColor(0, 0, 0); //black
            ofDrawCircle((k * 25) + 20 + 450 + 200 + 250, (j * 20) + 17, 4); //interior
            if (std::abs(deviceData[j].deltaValues[k]) > 0){
                ofSetColor(std::abs(deviceData[j].deltaValues[k] * 50), 0, 0);
                ofDrawCircle((k * 25) + 20 + 450 + 200 + 250, (j * 20) + 17, 3); //value
            }
        }
    }
    ofDrawBitmapStringHighlight("FPS: " + std::to_string(ofGetFrameRate()), 20, ofGetHeight() - 20);
    ofDrawBitmapStringHighlight("Frame Number: " + std::to_string(ofGetFrameNum()), 20, ofGetHeight() - 40);
    ofDrawBitmapStringHighlight("Number of Threads: " + std::to_string(oscPlayers.size()), 20, ofGetHeight() - 60);
#if defined(FULLDEBUG) || defined(LOGTIME)
    millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << "[TIME] End of draw : " << millisec_since_epoch << endl;
#endif
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
    for (int i = 0; i < videos.size(); i++){
        videos[i]->stop();
    }
    videos.clear();
}
