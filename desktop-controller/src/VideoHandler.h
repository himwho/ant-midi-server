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
#include "ofxOsc.h"

class VideoHandler {
public:
    bool playing;
    ofVideoGrabber vidGrabber;
    ofImage image;
    int camWidth = 640;  // try to grab at this size.
    int camHeight = 480;
    int camIndex;
    
    ofxOscSender sender;
    std::string iphost;
    int port;
    
    VideoHandler(){
        playing = false;
    }
    
    ~VideoHandler(){
        stop();
        vidGrabber.close();
    }

    void setup(int camIndex, std::string host, int port){
        this->iphost = host;
        this->port = port;
        sender.setup(HOST, PORT);
        
        this->camIndex = camIndex;
        vidGrabber.setDeviceID(camIndex);
        vidGrabber.setDesiredFrameRate(15);
        vidGrabber.initGrabber(camWidth, camHeight);
        ofSetVerticalSync(true);
        playing = true;
    }

    void update(){
        vidGrabber.update();
        if(vidGrabber.isFrameNew()){
            //load image
            image.setFromPixels(vidGrabber.getPixels());
            
            // Send received byte via OSC to server
            ofxOscMessage m;
            m.setAddress("/camera" + to_string(camIndex));
            //m.addBlobArg(vidGrabber.getPixels());
            //sender.sendMessage(m, false);
        }
    }
    
    void draw(float x, float y){
        
    }
    
    void stop(){
    
    }
};
#endif /* VideoHandler_h */
