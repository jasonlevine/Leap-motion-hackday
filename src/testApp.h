#pragma once

#include "ofMain.h"
#include "ofxAudioUnit.h"
#include "ofxUI.h"
#include "ofxAudioFeaturesChannel.h"
#include "ofxLeapMotion.h"
#include "ofxStrip.h"
#include "ofxPostProcessing.h"

class testApp : public ofBaseApp{

public:
    void setup();
    void update();
    void draw();
    void audioOut(float * input, int bufferSize, int nChannels);
    
    void exit();
    void guiEvent(ofxUIEventArgs &e);

    void keyPressed  (int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    ofxUICanvas *           gui;
    
    ofxAudioUnitSampler     synth;
    ofxAudioUnitTap         tap;
    ofxAudioUnitTapSamples  samples;
    ofxAudioUnit            limiter;
    ofxAudioUnitOutput      output;
    
    ofxAudioFeaturesChannel fftCalc;
    
    float                   prog;
    int                     bufferSize;
    float*                  buffer;
    float*                  spectrum;
    bool                    setupComplete;
    float                   rmsVol;
    
    vector<int>             dorian;
    float                   XyPad1x, XyPad1y, XyPad2x, XyPad2y;
    //texture
    
    
    //Leap Stuff
	ofEasyCam cam;
	ofxLeapMotion leap;
	vector <ofxLeapMotionSimpleHand> simpleHands;
	vector <int> fingersFound;
	map <int, ofPolyline> fingerTrails;
    //map<int,
	ofLight l1;
	ofLight l2;
	ofMaterial m1;
    
    ofFbo fadesFbo;
    float   fadeAmnt;
    bool    bUseFbo;
    bool    bFlip;
    bool    bKaleidoscope;
    
    ofxPostProcessing post;
		
};
