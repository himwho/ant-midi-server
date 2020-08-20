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
        sender.setup(HOST, port);
        
        this->camIndex = camIndex;
        vidGrabber.setDeviceID(camIndex);
        vidGrabber.setDesiredFrameRate(10);
        vidGrabber.initGrabber(camWidth, camHeight);
        ofSetVerticalSync(true);
        playing = true;
    }

    void update(){
        vidGrabber.update();
        if(vidGrabber.isFrameNew()){
            //load image
            static unsigned long size;
            image.setFromPixels(vidGrabber.getPixels());
            //image.resize(camWidth/4, camHeight/4);

//            ofBuffer cameraSendBuffer;
//            ofSaveImage(image.getPixelsRef(),cameraSendBuffer,OF_IMAGE_FORMAT_JPEG);
//            
//            // Send received byte via OSC to server
//            ofxOscMessage m;
//            m.setAddress("/camera" + to_string(camIndex));
//            m.addBlobArg(cameraSendBuffer);
//            sender.sendMessage(m, false);
        }
    }
    
    void stop(){

    }
};
#endif /* VideoHandler_h */
