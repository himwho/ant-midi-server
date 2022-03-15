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
#include "ofxCv.h"

class VideoHandler: public ofThread {
public:
    bool bUseForCV;
    bool playing;
    bool bCVProcessing = false;
    ofVideoGrabber vidGrabber;
    ofImage image;
    
    int camWidth = 640;  // try to grab at this size.
    int camHeight = 480;
    int camIndex;
    
    ofxOscSender sender;
    std::string iphost;
    int port;
    
    // CV SETUP
    ofxCv::FlowFarneback fb;
    ofxCv::FlowPyrLK lk;
    ofxCv::Flow* curFlow;
    ofxCv::FlowFarneback flow; // ofxCv's dense optical flow analyzer (for whole image)
    // alternative would be FlowPyrLK which detects/analyzes a sparse feature set
    vector<ofVec2f> frameFlows; // store average flow frame-by-frame
    ofVec2f vidFlowTotal = ofVec2f(0,0), vidFlowAvg = ofVec2f(0,0); // store average flow of video
    // moving dot
    ofVec2f dotPos;
    ofPolyline dotPath; // polyline tracks dot movement
    
    int lkMaxLevel = 3;
    int lkMaxFeatures = 200;
    float lkQualityLevel = 0.01;
    int lkMinDistance = 4;
    int lkWinSize = 12;
    bool usefb = true;
    float fbPyrScale = .5;
    int fbLevels = 2;
    int fbIterations = 2;
    int fbPolyN = 5;
    float fbPolySigma = 1.1;
    bool fbUseGaussian = true;
    int fbWinSize = 150;
    
    VideoHandler(){
        playing = false;
    }
    
    ~VideoHandler(){
        stop();
        vidGrabber.close();
        waitForThread(false);
    }

    void setup(int camIndex, std::string host, int port, int width, int height, bool bUseForCV){
        this->bUseForCV = bUseForCV;
        this->iphost = host;
        this->port = port;
        sender.setup(IPHOST, port);
        
        this->camIndex = camIndex;
        vidGrabber.setDeviceID(camIndex);
        if (bUseForCV) {
            vidGrabber.setDesiredFrameRate(20);
        } else {
            vidGrabber.setDesiredFrameRate(20);
        }
        this->camWidth = width;
        this->camHeight = height;
        vidGrabber.setup(camWidth, camHeight);
        ofSetVerticalSync(true);
        playing = true;
        curFlow = &fb;
    }

    void threadedFunction(){
        if (bUseForCV) {
            if(vidGrabber.isFrameNew()) {
                bCVProcessing = true;
                if(usefb) {
                    curFlow = &fb;
                    fb.setPyramidScale(fbPyrScale);
                    fb.setNumLevels(fbLevels);
                    fb.setWindowSize(fbWinSize);
                    fb.setNumIterations(fbIterations);
                    fb.setPolyN(fbPolyN);
                    fb.setPolySigma(fbPolySigma);
                    fb.setUseGaussian(fbUseGaussian);
                } else {
                    curFlow = &lk;
                    lk.setMaxFeatures(lkMaxFeatures);
                    lk.setQualityLevel(lkQualityLevel);
                    lk.setMinDistance(lkMinDistance);
                    lk.setWindowSize(lkWinSize);
                    lk.setMaxLevel(lkMaxLevel);
                }
                
                // you can use Flow polymorphically
//                ofImage resizedInput = vidGrabber.image.resize(this->camWidth/4, this->camHeight/4);
//                curFlow->calcOpticalFlow(ofxCv::toCv(resizedInput));
                curFlow->calcOpticalFlow(vidGrabber);
                // save average flows per frame + per vid
                frameFlows.push_back(flow.getAverageFlow());
                vidFlowTotal += frameFlows.back();
                vidFlowAvg = vidFlowTotal / frameFlows.size();
                // calculate dot position to show average movement
                dotPos += frameFlows.back();
                dotPath.addVertex(dotPos.x, dotPos.y);
            }
        }
        stop();
        bCVProcessing = false;
    }
    
    void update(){
        vidGrabber.update();
        if (!bCVProcessing) {
            startThread();
        }
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
        stopThread();
    }
};
#endif /* VideoHandler_h */
