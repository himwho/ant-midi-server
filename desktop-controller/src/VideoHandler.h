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

class VideoHandler: public ofThread{
public:
    bool playing;
    ofVideoGrabber cam;
    ofImage image;
    
    VideoHandler(){
        playing = false;
    }
    
    ~VideoHandler(){
        stop();
        waitForThread(true);
    }

    void stop(){
        stopThread();
    }

    /// Everything in this function will happen in a different
    /// thread which leaves the main thread completelty free for
    /// other task;s.
    void threadedFunction() {
        // start
        while(isThreadRunning()) {
            cam.update();
            if(cam.isFrameNew()){
                //lock access
                lock();
                //load image
                image.setFromPixels(cam.getPixels());
                //unlock
                unlock();
            }
        }
        //done
    }
};
#endif /* VideoHandler_h */
