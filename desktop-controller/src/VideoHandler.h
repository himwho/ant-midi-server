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
#include "ofxHTTP.h"

class VideoHandler {
public:
    bool playing = false;
    ofxHTTP::SimpleIPVideoServer server;
    ofVideoGrabber vidGrabber;
    ofImage image;
    
    int camWidth = 640;  // try to grab at this size.
    int camHeight = 480;
    int camIndex;

    VideoHandler(int camIndex, std::string host, int port){
        ofxHTTP::SimpleIPVideoServerSettings settings;
        settings.ipVideoRouteSettings.setMaxClientConnections(1);
        settings.setPort(port);
        settings.setHost(host);
        server.setup(settings);
        server.start();

        this->camIndex = camIndex;
        vidGrabber.setDeviceID(camIndex);
        vidGrabber.setDesiredFrameRate(25);
        vidGrabber.initGrabber(camWidth, camHeight);
        ofSetVerticalSync(true);
        playing = true;
    }
    
    ~VideoHandler(){
        stop();
    }

    void update(){
        vidGrabber.update();
        if(vidGrabber.isFrameNew()){
            //load image
            //image.setFromPixels(vidGrabber.getPixels());
            //image.resize(camWidth/4, camHeight/4);
            server.send(vidGrabber.getPixels());
        }
    }
    
    void stop(){
        server.stop();
        vidGrabber.close();
    }
};
#endif /* VideoHandler_h */
