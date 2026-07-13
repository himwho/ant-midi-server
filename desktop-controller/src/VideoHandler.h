//
//  VideoHandler.h
//  desktop-controller
//
//  Created by Dylan Marcus on 8/4/20.
//

#ifndef VideoHandler_h
#define VideoHandler_h

#include "ofMain.h"
#include "ofVideoGrabber.h"
#include <atomic>
#include <thread>
#include <mutex>

// Wraps a single physical camera.
//
// THREADING: AVFoundation session setup (ofVideoGrabber::setup) can block for
// many seconds when a device is contended, so it runs on a worker thread.
// Ownership of the grabber is handed off via the atomic `state`: the GL
// thread only touches it while Open, the worker only while Busy. GL objects
// never leave the GL thread: setup() runs with bTexture = false (no texture
// allocation on the worker), ofVideoGrabber::update() lazily allocates the
// texture on the GL thread when the first frame arrives, and close() —
// which destroys the texture — is only ever called from the GL thread.
class VideoHandler {
public:
    enum class State { Closed, Busy, Open };

    ofVideoGrabber vidGrabber;

    int camWidth = 640;  // try to grab at this size.
    int camHeight = 480;
    int camIndex = -1;
    std::string deviceName;

    bool bAvailable = false; // reported available by the OS at startup
    bool bEnabled = false;   // user setting: capture + draw this camera
    bool bUseForCV = false;  // user setting: run the CV pipeline on this camera
    bool bShowName = true;   // user setting: draw the name label in the main UI

    std::atomic<uint64_t> lastFrameMillis { 0 }; // last frame delivery time
    float measuredFps = 0; // actual received frame rate (GL thread only)

    // Recycling a starved camera renegotiates USB bandwidth, which can steal
    // it from cameras that are currently streaming. Cap automatic attempts so
    // a bandwidth-starved bus doesn't churn forever; the counter resets when
    // frames arrive or the user toggles the camera.
    int recycleAttempts = 0;
    static constexpr int kMaxRecycleAttempts = 2;

    ~VideoHandler(){
        joinWorker();
        if (state.load() == State::Open) {
            vidGrabber.close();
        }
    }

    void setup(int camIndex, const std::string& deviceName, int width, int height, bool available){
        this->camIndex = camIndex;
        this->deviceName = deviceName;
        this->camWidth = width;
        this->camHeight = height;
        this->bAvailable = available;
    }

    bool isOpen() const { return state.load() == State::Open; }
    bool isBusy() const { return state.load() == State::Busy; }

    // texture exists only after the first frame reached the GL thread
    bool hasTexture() const {
        const auto& planes = vidGrabber.getTexturePlanes();
        return !planes.empty() && planes[0].isAllocated();
    }

    // Open the camera on a worker thread; never blocks the GL thread.
    // Call from the GL thread only.
    void openAsync(){
        if (!bAvailable || state.load() != State::Closed) return;
        joinWorker();
        state.store(State::Busy);
        int desiredFps = bUseForCV ? 10 : 25;
        worker = std::thread([this, desiredFps]{
            // serialize opens across all cameras: simultaneous AVFoundation
            // session configuration makes USB bandwidth negotiation flakier
            static std::mutex openMutex;
            std::lock_guard<std::mutex> lock(openMutex);
            vidGrabber.setDeviceID(camIndex);
            vidGrabber.setDesiredFrameRate(desiredFps);
            // bTexture=false: no GL allowed off the GL thread
            bool ok = vidGrabber.setup(camWidth, camHeight, false);
            if (ok) {
                if (vidGrabber.getWidth() > 0 && vidGrabber.getHeight() > 0) {
                    camWidth = (int)vidGrabber.getWidth();
                    camHeight = (int)vidGrabber.getHeight();
                }
                ofLogNotice("VideoHandler") << "Opened camera " << camIndex << " (" << deviceName
                                            << ") at " << camWidth << "x" << camHeight;
                lastFrameMillis = ofGetElapsedTimeMillis();
            } else {
                ofLogError("VideoHandler") << "Failed to open camera " << camIndex << " (" << deviceName << ")";
            }
            state.store(ok ? State::Open : State::Closed);
        });
    }

    // Close the camera. Call from the GL thread only (destroys the texture).
    // AVFoundation stopRunning is quick (tens of ms), unlike setup.
    void close(){
        if (state.load() == State::Busy) {
            // let the pending open finish first; reconcile will close it on a
            // later frame once the worker releases ownership
            return;
        }
        joinWorker();
        if (state.load() == State::Open) {
            vidGrabber.close();
        }
        state.store(State::Closed);
        measuredFps = 0;
        frameCounter = 0;
    }

    void update(){
        if (!bEnabled || state.load() != State::Open) return;
        // texture use was deferred while opening on the worker thread
        if (!vidGrabber.isUsingTexture()) {
            vidGrabber.setUseTexture(true);
        }
        vidGrabber.update();
        uint64_t now = ofGetElapsedTimeMillis();
        if (vidGrabber.isFrameNew()) {
            lastFrameMillis = now;
            frameCounter++;
            recycleAttempts = 0; // camera is healthy again
        }
        if (now - fpsWindowStart >= 2000) {
            measuredFps = frameCounter * 1000.0f / float(now - fpsWindowStart);
            frameCounter = 0;
            fpsWindowStart = now;
        }
    }

    // false when the grabber is open but starved of frames (device held by
    // another app, or USB bandwidth exhaustion on a shared bus)
    bool hasRecentFrame() const {
        return isOpen() && (ofGetElapsedTimeMillis() - lastFrameMillis.load()) < 2000;
    }

private:
    std::atomic<State> state { State::Closed };
    std::thread worker;
    int frameCounter = 0;
    uint64_t fpsWindowStart = 0;

    void joinWorker(){
        if (worker.joinable()) {
            worker.join();
        }
    }
};
#endif /* VideoHandler_h */
