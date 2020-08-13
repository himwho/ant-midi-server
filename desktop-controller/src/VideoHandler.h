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

class VideoHandler {
public:
    bool playing;
    ofVideoGrabber vidGrabber;
    ofImage image;
    int camWidth = 640;  // try to grab at this size.
    int camHeight = 480;
    int camIndex;
    
    VideoHandler(){
        playing = false;
    }
    
    ~VideoHandler(){
        stop();
        vidGrabber.close();
    }

    void setCamIndex(int camIndex){
        this->camIndex = camIndex;
        vidGrabber.setDeviceID(camIndex);
        vidGrabber.setDesiredFrameRate(20);
        vidGrabber.initGrabber(camWidth, camHeight);
        ofSetVerticalSync(true);
        playing = true;
    }

    void update(){
        vidGrabber.update();
        if(vidGrabber.isFrameNew()){
            //load image
            image.setFromPixels(vidGrabber.getPixels());
        }
    }
    
    void draw(float x, float y){
        
    }
    
    void stop(){
    
    }
};
#endif /* VideoHandler_h */
