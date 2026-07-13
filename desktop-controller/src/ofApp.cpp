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

static const std::string kSettingsFile = "settings.json";

std::string ofApp::readBytesUntilNewline(ofSerial& serial, char until){
    std::string result;
    int safety = 0;
    while (safety++ < 4096) {
        if (serial.available() <= 0) {
            ofSleepMillis(1);
            if (serial.available() <= 0) {
                break;
            }
        }
        int b = serial.readByte();
        if (b < 0) {
            break;
        }
        if ((char)b == until || (char)b == '\r') {
            if ((char)b == '\r' && serial.available() > 0) {
                // consume trailing \n if present
                int peek = serial.readByte();
                if (peek >= 0 && (char)peek != '\n') {
                    // put back isn't supported; keep going with that byte next loop by appending if not terminator
                    if ((char)peek != until) {
                        result.push_back((char)peek);
                    }
                }
            }
            break;
        }
        result.push_back((char)b);
    }
    return result;
}

//--------------------------------------------------------------
void ofApp::setup(){
#ifdef LOGSENSORVALUES
#define ofLogNotice() ofLogNotice() << ofGetTimestampString("[%Y-%m-%d %H:%M:%S.%i] ")
#endif
    ofLogNotice() << "ofApp::setup" << "Connected Devices: ";

    ofSerial enumerator;
    enumerator.listDevices();
    std::vector<ofSerialDeviceInfo> devicesInfo = enumerator.getDeviceList();
        
    if (!devicesInfo.empty()) {
        for (std::size_t i = 0; i < devicesInfo.size(); ++i) {
            std::string path = devicesInfo[i].getDevicePath();
            ofLogNotice() << "ofApp::setup" << "\t" << path;
            if (path.find("usbmodem") != std::string::npos) {
                std::cout << "Port Position: " << i << " contains ants!" << '\n';
                numberOfConnectedDevices++; // Add to count of discovered habs
                devices.resize(numberOfConnectedDevices); // Resize devices for number of discovered habs
                devicePorts.resize(numberOfConnectedDevices);
                foundDevicesArray.push_back(i);
            } else {
                std::cout << "Port Position: " << i << " probably has no ants." << '\n';
            }
        }

        // create 'n' number of device structs
        deviceData.resize(numberOfConnectedDevices);
        receivedData.resize(numberOfConnectedDevices);

        for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
            std::string path = devicesInfo[foundDevicesArray[j]].getDevicePath();
            bool success = devices[j].setup(path, 115200);
            if (success) {
                ofLogNotice() << "ofApp::setup" << "Successfully setup " << path;
                deviceData[j].deviceNameStr = path;
                devicePorts[j] = path;
            } else {
                ofLogNotice() << "ofApp::setup" << "Unable to setup " << path;
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
    
    // CAMERA DISCOVERY
    // Create a handler for every detected camera (index == OS device id) so
    // cameras can be enabled/disabled at runtime from the settings window.
    ofVideoGrabber enumeratorGrabber;
    std::vector<ofVideoDevice> cameras = enumeratorGrabber.listDevices();

    for(size_t i = 0; i < cameras.size(); i++){
        ofLogNotice() << cameras[i].id << ": " << cameras[i].deviceName
                      << (cameras[i].bAvailable ? "" : " - unavailable");
        auto handler = std::make_unique<VideoHandler>();
        handler->setup(cameras[i].id, cameras[i].deviceName, 640, 480, cameras[i].bAvailable);
        // defaults (may be overridden by settings.json): enable available
        // cameras, use CV on C920s as before
        handler->bEnabled = cameras[i].bAvailable;
        handler->bUseForCV = cameras[i].deviceName.find("C920") != std::string::npos;
        videos.push_back(std::move(handler));
        camCV.push_back(nullptr);
    }

    // default display order = discovery order (overridden by settings.json)
    displayOrder.resize(videos.size());
    for (size_t i = 0; i < videos.size(); i++) {
        displayOrder[i] = (int)i;
    }

    showLabels = false;
    loadSettings();
    reconcileCameras();
    saveSettings(); // write current state so newly found cameras appear in the file
    
    // Initialize OSC sender for hash messages
    hashSender.setup(IPHOST, PORT);
}

//--------------------------------------------------------------
// SETTINGS PERSISTENCE
//--------------------------------------------------------------
void ofApp::loadSettings(){
    if (!ofFile::doesFileExist(kSettingsFile)) {
        ofLogNotice("ofApp::loadSettings") << "No settings file, using defaults.";
        return;
    }
    ofJson json = ofLoadJson(kSettingsFile);
    layoutColumns = json.value("layoutColumns", 0);
    if (json.contains("cameras") && json["cameras"].is_array()) {
        // The file's array order is the display order. Duplicate device names
        // are common (several identical webcams), so match name+index first,
        // then name only, then index only — never reusing a camera.
        std::vector<bool> used(videos.size(), false);
        auto findMatch = [&](const std::string& name, int index) -> int {
            for (size_t i = 0; i < videos.size(); i++) {
                if (!used[i] && videos[i]->deviceName == name && videos[i]->camIndex == index) return (int)i;
            }
            for (size_t i = 0; i < videos.size(); i++) {
                if (!used[i] && !name.empty() && videos[i]->deviceName == name) return (int)i;
            }
            for (size_t i = 0; i < videos.size(); i++) {
                if (!used[i] && videos[i]->camIndex == index) return (int)i;
            }
            return -1;
        };

        std::vector<int> order;
        for (const auto& cam : json["cameras"]) {
            int i = findMatch(cam.value("name", ""), cam.value("index", -1));
            if (i < 0) continue; // camera from the file is not connected now
            used[i] = true;
            order.push_back(i);
            auto& v = *videos[i];
            v.bEnabled = cam.value("enabled", v.bEnabled) && v.bAvailable;
            v.bUseForCV = cam.value("cv", v.bUseForCV);
            v.bShowName = cam.value("showName", v.bShowName);
        }
        // newly connected cameras that aren't in the file go to the end
        for (size_t i = 0; i < videos.size(); i++) {
            if (!used[i]) order.push_back((int)i);
        }
        displayOrder = order;
    }
}

void ofApp::saveSettings(){
    ofJson json;
    json["layoutColumns"] = layoutColumns;
    json["cameras"] = ofJson::array(); // array order == display order
    for (int i : displayOrder) {
        const auto& v = *videos[i];
        ofJson cam;
        cam["name"] = v.deviceName;
        cam["index"] = v.camIndex;
        cam["enabled"] = v.bEnabled;
        cam["cv"] = v.bUseForCV;
        cam["showName"] = v.bShowName;
        json["cameras"].push_back(cam);
    }
    ofSavePrettyJson(kSettingsFile, json);
}

std::vector<int> ofApp::enabledCameraIndices() const {
    std::vector<int> indices;
    for (int i : displayOrder) {
        // enabled cameras keep their grid slot even when the grabber failed
        // to open, so contention is visible instead of tiles disappearing
        if (videos[i]->bEnabled && videos[i]->bAvailable) {
            indices.push_back(i);
        }
    }
    return indices;
}

//--------------------------------------------------------------
// Open/close grabbers so they match the current enable flags, and
// (re)build per-camera CV state.
void ofApp::reconcileCameras(){
    for (size_t i = 0; i < videos.size(); i++) {
        auto& v = videos[i];
        if (v->bEnabled && v->bAvailable) {
            // AVFoundation reports success even when the device is held by
            // another app, leaving a session that never delivers frames.
            // Recycle grabbers that have been silent for a long time so the
            // camera is picked up once the other app releases it. The close
            // is quick and runs here (GL thread owns the texture); the open
            // happens on a worker thread so the UI never stalls. Attempts are
            // capped: on a bandwidth-starved USB bus, endless recycling
            // disturbs the cameras that ARE streaming.
            if (v->isOpen() && !v->hasRecentFrame() &&
                ofGetElapsedTimeMillis() - v->lastFrameMillis.load() > 20000 &&
                v->recycleAttempts < VideoHandler::kMaxRecycleAttempts) {
                v->recycleAttempts++;
                ofLogNotice("ofApp") << "Recycling stalled camera " << v->camIndex << " (" << v->deviceName
                                     << "), attempt " << v->recycleAttempts << "/" << VideoHandler::kMaxRecycleAttempts;
                v->close();
            }
            if (!v->isOpen() && !v->isBusy()) {
                v->openAsync();
            }
            if (v->bUseForCV) {
                if (!camCV[i]) {
                    // buffers are allocated lazily in runCV once the first
                    // frame arrives, since a contended/starved camera may
                    // never deliver pixels
                    camCV[i] = std::make_unique<CamCV>();
                    camCV[i]->contourFinder.setMinAreaRadius(3);
                    camCV[i]->contourFinder.setMaxAreaRadius(50);
                    camCV[i]->contourFinder.setThreshold(15);
                    camCV[i]->contourFinder.getTracker().setPersistence(15);
                    camCV[i]->contourFinder.getTracker().setMaximumDistance(50);
                }
            } else {
                camCV[i].reset();
            }
        } else {
            // if a worker is mid-open (Busy), close() is a no-op; a later
            // reconcile pass will close it once the worker finishes
            v->close();
            camCV[i].reset();
        }
    }
}

void ofApp::setupDevice(int deviceID){
    // Initial setup for min/max values per device
    // Send next message of current frame
    unsigned char sendTriggerByte = 1;
    devices[deviceID].writeByte(sendTriggerByte);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Read all bytes from the devices;
    receivedData[deviceID] = readBytesUntilNewline(devices[deviceID]);
    
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
    // apply pending camera enable/disable changes from the settings window,
    // and periodically retry cameras that failed to open (device may have
    // been released by another app since)
    uint64_t nowMillis = ofGetElapsedTimeMillis();
    if (camerasDirty || nowMillis - lastReconcileMillis > 5000) {
        camerasDirty = false;
        lastReconcileMillis = nowMillis;
        reconcileCameras();
    }

    // periodic per-camera delivery report, to make frame starvation visible
    if (nowMillis - lastFpsLogMillis > 5000 && !videos.empty()) {
        lastFpsLogMillis = nowMillis;
        std::stringstream fpsReport;
        for (const auto& v : videos) {
            if (!v->bEnabled) continue;
            fpsReport << "[" << v->camIndex << " " << v->deviceName << "] "
                      << (v->isBusy() ? "opening" : (v->isOpen() ? ofToString(v->measuredFps, 1) + "fps" : "closed"))
                      << "  ";
        }
        ofLogNotice("cameras") << fpsReport.str();
    }
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
                        receivedData[j] = readBytesUntilNewline(devices[j]);
                        devices[j].flush();
#if defined(FULLDEBUG) || defined(LOGTIME)
                        millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                        std::cout << "[TIME] After readBytesUntil : " << millisec_since_epoch << std::endl;
#endif
                        
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
                                            oscPlayers.push_back(std::make_unique<OSCPlayerObject>());
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
                                            oscPlayers.push_back(std::make_unique<OSCPlayerObject>());
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
                    unsigned char sendTriggerByte = 1;
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
    // Update every enabled camera exactly once per frame, then run CV on the
    // ones flagged for it.
    for (size_t i = 0; i < videos.size(); i++){
        videos[i]->update();
        if (videos[i]->bEnabled && videos[i]->bUseForCV && videos[i]->isOpen() && camCV[i]) {
            runCV((int)i);
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
        // aggregate tracked points from all CV cameras
        std::vector<ofPoint> allTrackedPoints;
        for (const auto& camState : camCV) {
            if (camState) {
                allTrackedPoints.insert(allTrackedPoints.end(), camState->lastCenter.begin(), camState->lastCenter.end());
            }
        }
        currentHash = hashGenerator.generateHash(deviceData, allTrackedPoints);
        
        // Send hash to nodejs server via OSC
        ofxOscMessage m;
        m.setAddress("/hash");
        m.addStringArg(currentHash);
        hashSender.sendMessage(m, false);
    }
}

//--------------------------------------------------------------
// Motion-tracking pipeline for a single CV-enabled camera.
void ofApp::runCV(int camIndex){
    auto& v = *videos[camIndex];
    auto& state = *camCV[camIndex];
    if (!v.vidGrabber.isFrameNew()) return;

    // first frame (or capture size changed): allocate buffers and store a
    // baseline, diffing starts on the next frame
    if (!state.previous.isAllocated() ||
        (int)state.previous.getWidth() != (int)v.vidGrabber.getWidth() ||
        (int)state.previous.getHeight() != (int)v.vidGrabber.getHeight()) {
        ofxCv::imitate(state.previous, v.vidGrabber);
        ofxCv::imitate(state.diff, v.vidGrabber);
        ofxCv::copy(v.vidGrabber, state.previous);
        return;
    }

    // take the absolute difference of prev and cam and save it inside diff
    ofxCv::absdiff(v.vidGrabber, state.previous, state.diff);
    state.diff.update();
    ofxCv::copy(v.vidGrabber, state.previous);
    ofxCv::blur(state.diff, 10);
    state.contourFinder.findContours(state.diff);
    state.lastCenter.resize(state.contourFinder.size());
    for(int j = 0; j < state.contourFinder.size(); j++) {
        auto center = ofxCv::toOf(state.contourFinder.getCenter(j));
        auto velocity = ofxCv::toOf(state.contourFinder.getVelocity(j));

        // only add a new osc player if there is found velocity to the movement
        if (velocity.x > 0 || velocity.y > 0) {
            if (std::fabs(velocity.x) < state.lowestVelocityX) {
                // cannot allow the mins to become < 1
                state.lowestVelocityX = std::max(std::abs((int)velocity.x), 1);
            }
            if (std::fabs(velocity.y) < state.lowestVelocityY) {
                state.lowestVelocityY = std::max(std::abs((int)velocity.y), 1);
            }
            if (std::fabs(velocity.x) > state.highestVelocityX) {
                state.highestVelocityX = std::fabs(velocity.x);
            }
            if (std::fabs(velocity.y) > state.highestVelocityY) {
                state.highestVelocityY = std::fabs(velocity.y);
            }
            if (oscPlayers.size() < MAX_CONCURRENT_VOICES){ //block too many triggers
                float lastValue = 0;
                if (j < state.lastCenter.size() && state.lastCenter[j].x >= 0) {
                    lastValue = state.lastCenter[j].x;
                }
                oscPlayers.push_back(std::make_unique<OSCPlayerObject>());
                oscPlayers.back()->outputDeviceValueOSC(camIndex, j, (int)center.x, (int)lastValue, (int)std::fabs((float)velocity.x*(127/state.lowestVelocityX)), (int)std::fabs((float)velocity.x*(127/state.highestVelocityX)), 130, 0+1);
#if defined(FULLDEBUG) || defined(LOGCV)
                std::cout << "[MIDI CV] Cam " << camIndex << " Center : " << center.x << ", " << center.y << std::endl;
                std::cout << "[MIDI CV] Cam " << camIndex << " Velocity : " << velocity.x << ", " << velocity.y << std::endl;
#endif
            }
            state.lastCenter[j] = center;
        }
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
    ofSetHexColor(0xffffff);
    ofSetBackgroundAuto(true);

    // VIDEO GRID
    // Lay enabled cameras out on a grid. layoutColumns == 0 picks a roughly
    // square grid automatically; 1-4 forces that many columns.
    std::vector<int> enabled = enabledCameraIndices();
    int n = (int)enabled.size();
    if (n > 0) {
        int cols = layoutColumns > 0 ? layoutColumns : (int)std::ceil(std::sqrt((double)n));
        cols = std::min(cols, n);
        int rows = (int)std::ceil((double)n / cols);
        float cellW = (float)ofGetWidth() / cols;
        float cellH = (float)ofGetHeight() / rows;

        for (int slot = 0; slot < n; slot++) {
            int i = enabled[slot];
            auto& v = *videos[i];
            float cellX = (slot % cols) * cellW;
            float cellY = (slot / cols) * cellH;

            // fit the camera frame into the cell, preserving aspect ratio
            float scale = std::min(cellW / v.camWidth, cellH / v.camHeight);
            float drawW = v.camWidth * scale;
            float drawH = v.camHeight * scale;
            float drawX = cellX + (cellW - drawW) / 2;
            float drawY = cellY + (cellH - drawH) / 2;

            if (v.isOpen() && v.hasTexture()) {
                ofSetColor(255);
                v.vidGrabber.draw(drawX, drawY, drawW, drawH);
            } else {
                ofSetColor(20);
                ofDrawRectangle(drawX, drawY, drawW, drawH);
            }

            if (v.bUseForCV && camCV[i]) {
                auto& state = *camCV[i];
                ofxCv::RectTracker& tracker = state.contourFinder.getTracker();

                // motion overlay: draw the diff over the full camera cell with
                // additive blending so its black background is transparent and
                // only motion shows, tinted to stand out from the video
                if (state.diff.isAllocated()) {
                    ofEnableBlendMode(OF_BLENDMODE_ADD);
                    ofSetColor(255, 160, 0);
                    state.diff.draw(drawX, drawY, drawW, drawH);
                    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
                }

                // contour overlay, scaled from camera space to cell space
                ofPushMatrix();
                ofTranslate(drawX, drawY);
                ofScale(scale, scale);
                ofSetColor(255);
                if(showLabels) {
                    state.contourFinder.draw();
                    for(int c = 0; c < state.contourFinder.size(); c++) {
                        ofPoint center = ofxCv::toOf(state.contourFinder.getCenter(c));
                        ofPushMatrix();
                        ofTranslate(center.x, center.y);
                        int label = state.contourFinder.getLabel(c);
                        string msg = ofToString(label) + ":" + ofToString(tracker.getAge(label));
                        ofDrawBitmapString(msg, 0, 0);
                        ofVec2f velocity = ofxCv::toOf(state.contourFinder.getVelocity(c));
                        ofScale(5, 5);
                        ofDrawLine(0, 0, velocity.x, velocity.y);
                        ofPopMatrix();
                    }
                } else {
                    for(int c = 0; c < state.contourFinder.size(); c++) {
                        unsigned int label = state.contourFinder.getLabel(c);
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
                        ofPoint center = ofxCv::toOf(state.contourFinder.getCenter(c));
                        label = state.contourFinder.getLabel(c);
                        string msg = ofToString(label) + ":" + ofToString(tracker.getAge(label));
                        ofDrawBitmapString(msg, center.x + 20, center.y + 20);
                    }
                }
                ofPopMatrix();
            }

            // camera not delivering frames: either another app holds it, or
            // too many cameras share one USB bus and this one lost bandwidth
            if (!v.hasRecentFrame()) {
                std::string msg;
                ofColor bg(180, 30, 30);
                if (v.isBusy()) {
                    msg = "OPENING...";
                    bg = ofColor(60, 60, 60);
                } else if (v.isOpen()) {
                    msg = "NO SIGNAL";
                } else {
                    msg = "IN USE / FAILED TO OPEN";
                }
                ofSetColor(255);
                ofDrawBitmapStringHighlight(msg, drawX + drawW / 2 - msg.size() * 4, drawY + drawH / 2,
                                            bg, ofColor(255));
            }

            // camera label
            if (v.bShowName) {
                ofSetColor(255);
                std::string tag = "[" + std::to_string(v.camIndex) + "] " + v.deviceName + (v.bUseForCV ? "  (CV)" : "");
                ofDrawBitmapStringHighlight(tag, drawX + 6, drawY + drawH - 8);
            }
        }
    } else {
        ofSetColor(255);
        ofDrawBitmapStringHighlight("No cameras enabled. Use the Settings window to enable cameras.",
                                    ofGetWidth()/2 - 240, ofGetHeight()/2);
    }
    
    // SENSOR OVERLAY
    for (std::size_t j = 0; j < numberOfConnectedDevices; j++) {
        ofSetColor(255, 255, 255); //white
        ofDrawBitmapStringHighlight("Ants found on port:  " + devicePorts[j], 20, (j * 20) + 20);
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
// SETTINGS WINDOW
//--------------------------------------------------------------
void ofApp::drawSettings(ofEventArgs& args){
    ofBackground(35);
    uiHits.clear();

    float x = 16, y = 28;
    ofSetColor(255);
    ofDrawBitmapStringHighlight("SETTINGS", x, y, ofColor(35), ofColor(255));

    // ---- layout picker ----
    y += 28;
    ofSetColor(200);
    ofDrawBitmapString("Layout (grid columns)", x, y);
    y += 10;
    const char* labels[5] = {"Auto", "1", "2", "3", "4"};
    for (int b = 0; b < 5; b++) {
        ofRectangle r(x + b * 64, y, 56, 26);
        bool selected = (layoutColumns == b);
        ofSetColor(selected ? ofColor(0, 140, 255) : ofColor(70));
        ofDrawRectRounded(r, 4);
        ofSetColor(255);
        ofDrawBitmapString(labels[b], r.x + (b == 0 ? 12 : 24), r.y + 17);
        uiHits.push_back({r, UIHit::LayoutButton, b});
    }
    y += 26 + 30;

    // ---- camera list ----
    ofSetColor(200);
    ofDrawBitmapString("Cameras (click to toggle, arrows reorder)", x, y);
    y += 14;

    if (videos.empty()) {
        ofSetColor(150);
        ofDrawBitmapString("No cameras detected.", x, y + 16);
    }

    float rowW = ofGetWidth() - 2 * x;
    for (size_t p = 0; p < displayOrder.size(); p++) {
        int i = displayOrder[p];
        auto& v = *videos[i];
        float rowY = y + (float)p * 66;
        float rowRight = x + rowW;

        // row background
        ofSetColor(v.bAvailable ? ofColor(50) : ofColor(42));
        ofDrawRectRounded(x, rowY, rowW, 58, 4);

        // name (truncated to fit)
        std::string name = "[" + std::to_string(v.camIndex) + "] " + v.deviceName;
        if (name.size() > 36) name = name.substr(0, 33) + "...";
        ofSetColor(v.bAvailable ? ofColor(255) : ofColor(120));
        ofDrawBitmapString(name, x + 10, rowY + 18);

        // reorder arrows (right edge, stacked)
        float arrowX = rowRight - 26;
        if (p > 0) {
            ofSetColor(180);
            ofDrawTriangle(arrowX + 8, rowY + 8, arrowX, rowY + 20, arrowX + 16, rowY + 20);
            uiHits.push_back({ofRectangle(arrowX - 4, rowY + 4, 24, 22), UIHit::MoveUp, (int)p});
        }
        if (p + 1 < displayOrder.size()) {
            ofSetColor(180);
            ofDrawTriangle(arrowX, rowY + 36, arrowX + 16, rowY + 36, arrowX + 8, rowY + 48);
            uiHits.push_back({ofRectangle(arrowX - 4, rowY + 32, 24, 22), UIHit::MoveDown, (int)p});
        }

        if (!v.bAvailable) {
            ofSetColor(120);
            ofDrawBitmapString("unavailable", x + 10, rowY + 42);
            continue;
        }

        // enabled checkbox
        ofRectangle enabledBox(x + 10, rowY + 28, 16, 16);
        ofSetColor(70);
        ofDrawRectangle(enabledBox);
        if (v.bEnabled) {
            ofSetColor(0, 200, 90);
            ofDrawRectangle(enabledBox.x + 3, enabledBox.y + 3, 10, 10);
        }
        ofSetColor(220);
        ofDrawBitmapString("Enabled", enabledBox.getRight() + 8, rowY + 41);
        uiHits.push_back({ofRectangle(enabledBox.x, enabledBox.y - 4, 90, 24), UIHit::ToggleEnabled, i});

        // CV checkbox
        ofRectangle cvBox(x + 120, rowY + 28, 16, 16);
        ofSetColor(70);
        ofDrawRectangle(cvBox);
        if (v.bUseForCV) {
            ofSetColor(255, 160, 0);
            ofDrawRectangle(cvBox.x + 3, cvBox.y + 3, 10, 10);
        }
        ofSetColor(220);
        ofDrawBitmapString("Motion CV", cvBox.getRight() + 8, rowY + 41);
        uiHits.push_back({ofRectangle(cvBox.x, cvBox.y - 4, 105, 24), UIHit::ToggleCV, i});

        // show-name checkbox
        ofRectangle nameBox(x + 235, rowY + 28, 16, 16);
        ofSetColor(70);
        ofDrawRectangle(nameBox);
        if (v.bShowName) {
            ofSetColor(90, 170, 255);
            ofDrawRectangle(nameBox.x + 3, nameBox.y + 3, 10, 10);
        }
        ofSetColor(220);
        ofDrawBitmapString("Name", nameBox.getRight() + 8, rowY + 41);
        uiHits.push_back({ofRectangle(nameBox.x, nameBox.y - 4, 70, 24), UIHit::ToggleName, i});

        // live status: green = frames flowing, orange = open but starved,
        // yellow = opening, gray = closed; plus measured delivery rate
        std::string status;
        if (v.hasRecentFrame()) {
            ofSetColor(0, 220, 100);
            status = ofToString(v.measuredFps, 1) + "fps";
        } else if (v.isBusy()) {
            ofSetColor(230, 230, 0);
            status = "opening";
        } else if (v.isOpen()) {
            ofSetColor(255, 140, 0);
            status = "no signal";
        } else {
            ofSetColor(120);
            status = "off";
        }
        ofDrawCircle(rowRight - 44, rowY + 12, 5);
        ofSetColor(160);
        ofDrawBitmapString(status, rowRight - 44 - 12 - status.size() * 8, rowY + 17);
    }

    float footerY = y + (float)std::max<size_t>(displayOrder.size(), 1) * 66 + 20;
    ofSetColor(140);
    ofDrawBitmapString("All settings persist to bin/data/settings.json", x, footerY);
    ofDrawBitmapString("Orange dot = camera open but not sending frames", x, footerY + 16);
}

void ofApp::mousePressedSettings(ofMouseEventArgs& args){
    for (const auto& hit : uiHits) {
        if (!hit.rect.inside(args.x, args.y)) continue;
        switch (hit.action) {
            case UIHit::LayoutButton:
                layoutColumns = hit.value;
                break;
            case UIHit::ToggleEnabled:
                videos[hit.value]->bEnabled = !videos[hit.value]->bEnabled;
                videos[hit.value]->recycleAttempts = 0; // manual toggle re-arms retries
                camerasDirty = true;
                break;
            case UIHit::ToggleCV:
                videos[hit.value]->bUseForCV = !videos[hit.value]->bUseForCV;
                camerasDirty = true;
                break;
            case UIHit::ToggleName:
                videos[hit.value]->bShowName = !videos[hit.value]->bShowName;
                break;
            case UIHit::MoveUp:
                std::swap(displayOrder[hit.value], displayOrder[hit.value - 1]);
                break;
            case UIHit::MoveDown:
                std::swap(displayOrder[hit.value], displayOrder[hit.value + 1]);
                break;
        }
        saveSettings();
        break;
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key >= '0' && key <= '4') {
        layoutColumns = key - '0';
        saveSettings();
    } else if (key == 'l') {
        showLabels = !showLabels;
    }
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
        videos[i]->close();
    }
    videos.clear();
    camCV.clear();
}
