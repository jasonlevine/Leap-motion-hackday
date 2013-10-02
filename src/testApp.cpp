#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    setupComplete = false;
    
    //graphics
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofEnableAlphaBlending();
    
    bUseFbo = false;
    bFlip = true;
    
    //leap setup
	leap.open();
    
    
	cam.setOrientation(ofPoint(-20, 0, 0));
	l1.setPosition(200, 300, 50);
	l2.setPosition(-200, -200, 50);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    
    ofSetFullscreen(true);
    fadesFbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA32F_ARB);
    fadesFbo.begin();
	ofClear(255,255,255, 20);
    fadesFbo.end();
    
    //audio
    synth = ofxAudioUnitSampler('aumu', 'CaC2', 'CamA');
    synth.getParameters(false, false);
    synth.setBank(0, 0);
    prog = 0;
    synth.setProgram(prog);
    
    limiter = ofxAudioUnit('aufx', 'mcmp', 'appl');
    limiter.setParameter(kMultibandCompressorParam_Threshold1, kAudioUnitScope_Global, -10.0);
    limiter.setParameter(kMultibandCompressorParam_Headroom1, kAudioUnitScope_Global, 0.1);
    limiter.setParameter(kMultibandCompressorParam_Threshold2, kAudioUnitScope_Global, -10.0);
    limiter.setParameter(kMultibandCompressorParam_Headroom2, kAudioUnitScope_Global, 0.1);
    limiter.setParameter(kMultibandCompressorParam_Threshold3, kAudioUnitScope_Global, -10.0);
    limiter.setParameter(kMultibandCompressorParam_Headroom3, kAudioUnitScope_Global, 0.1);
    limiter.setParameter(kMultibandCompressorParam_Threshold4, kAudioUnitScope_Global, -10.0);
    limiter.setParameter(kMultibandCompressorParam_Headroom4, kAudioUnitScope_Global, 0.1);
    
    synth >> tap >> limiter >> output;
    output.start();
    
    tap.getSamples(samples);
    bufferSize = samples.size();
    buffer = new float[bufferSize];
    spectrum = new float[bufferSize/2];
    
    ofSoundStreamSetup(2, 0, 44100, bufferSize, 4);
    fftCalc.setup(bufferSize, bufferSize/8, 44100);
    
    int DorianScaleDegrees[] = {0, 2, 3, 5, 7, 9, 10};
    dorian.assign(DorianScaleDegrees, DorianScaleDegrees+7);
    
    
    //postproc
    post.init(1920, 1080);
    post.createPass<FxaaPass>()->setEnabled(true);
    post.createPass<BloomPass>()->setEnabled(true);
    post.createPass<KaleidoscopePass>()->setEnabled(true);
    
    //gui
    
    gui = new ofxUICanvas(0,0,320,640);
    gui->addWidgetDown(new ofxUILabel("LEAP HACK DAY", OFX_UI_FONT_LARGE));
    gui->addSpacer(304, 2);
    gui->addWidgetDown(new ofxUISlider(304,16,0.0,255.0,100.0,"BACKGROUND VALUE"));
    gui->addWidgetDown(new ofxUIToggle(32, 32, false, "FULLSCREEN"));
    gui->addSpacer(304, 2);
    gui->addWidgetDown(new ofxUIButton("LAST PATCH", false, 16, 16));
    gui->addWidgetDown(new ofxUIButton("NEXT PATCH", false, 16, 16));
    gui->addSlider("PROGRAM", 0, 127, &prog, 95, 16);
    gui->addSlider("XyPad1x", 0.0,1.0, &XyPad1x, 304, 16);
    gui->addSlider("XyPad1y", 0.0,1.0, &XyPad1y, 304, 16);
    gui->addSlider("XyPad2x", 0.0,1.0, &XyPad2x, 304, 16);
    gui->addSlider("XyPad2y", 0.0,1.0, &XyPad2y, 304, 16);
    gui->addSpacer(304, 2);
    gui->addWidgetDown(new ofxUILabel("WAVEFORM DISPLAY", OFX_UI_FONT_MEDIUM));
	gui->addWidgetDown(new ofxUIWaveform(304, 64, buffer, bufferSize, -1.0, 1.0, "WAVEFORM"));
    gui->addWidgetDown(new ofxUILabel("SPECTRUM DISPLAY", OFX_UI_FONT_MEDIUM));
    gui->addWidgetDown(new ofxUISpectrum(304, 64, spectrum, bufferSize/2, 0.0, 1.0, "SPECTRUM"));
    gui->addSpacer(304, 2);
    gui->addSlider("fade amnt", 1.0, 200.0, &fadeAmnt, 304, 16);
    gui->addToggle("kaleidascope", bKaleidoscope);
    
    ofAddListener(gui->newGUIEvent, this, &testApp::guiEvent);
    gui->loadSettings("GUI/guiSettings.xml");

    setupComplete = true;
}

//--------------------------------------------------------------
void testApp::update(){
    if (setupComplete) {
        tap.getSamples(samples);
        
        float curVol = 0.0;
        for (int i = 0; i < bufferSize; i++) {
            float sample = (samples.left[i] + samples.right[i])/2;
            curVol+=sample;
            fftCalc.inputBuffer[i] = buffer[i] = sample;
        }
        curVol/=samples.size();
        rmsVol = sqrt(curVol);
        fftCalc.process(0);
        
        for (int i = 0; i < fftCalc.spectrum.size(); i++) {
            spectrum[i] = fftCalc.spectrum[i];
        }
    }
    
    simpleHands = leap.getSimpleHands();
    if( leap.isFrameNew() && simpleHands.size() ){
        vector <int> prevFingersFound = fingersFound;
        fingersFound.clear();
        
        leap.setMappingX(-170, 170, -ofGetWidth()/2, ofGetWidth()/2);
		leap.setMappingY(150, 400, -ofGetHeight()/2, ofGetHeight()/2);
        leap.setMappingZ(-150, 150, -200, 200);
        
        //store all the current fingers
        for(int i = 0; i < simpleHands.size(); i++){
            
            for(int j = 0; j < simpleHands[i].fingers.size(); j++){
                fingersFound.push_back(simpleHands[i].fingers[j].id);
            }
        }
        

        bool allFingersGone = (prevFingersFound.size() > 0) && (fingersFound.size() == 0);
        bool fingersBack = (prevFingersFound.size() == 0) && (fingersFound.size() > 0);
        
        //when we get a finger, clear any curves we left behind
        if(fingersBack)
        {
//            fingerTrails.clear();
        }
        
        //printf(")
        //now check against prev fingers to see whats no longer around
        for(int i = 0 ; i < prevFingersFound.size() ; i++)
        {
            int id = prevFingersFound[i];
            bool found = false;
            for(int j = 0 ; j < fingersFound.size() ; j++)
            {
                if(fingersFound[j] == id)
                {
                    found = true;
                    break;
                }
            }
            //delete the path now that this finger is no longer around
            if(!found)
            {
                //if(!allFingersGone) { //leave the last curve there if your final finger is going away
                    
                //}
                synth.midiNoteOff(60+dorian[id%7], 0);
                
                cout << "finger ID: " << id << endl;
            }
        }
        
        //turn all notes off once we have no fingers
        if(fingersFound.size() == 0)
        {
            for(int i = 0 ; i < 7 ; i++)
                synth.midiNoteOff(dorian[i], 0);
        }
        
        if (rmsVol < 0.001) {
            for (int i = 0; i < fingerTrails.size(); i++)
            fingerTrails[i].clear();
        }
        
        for(int i = 0; i < simpleHands.size(); i++){
            
            for(int j = 0; j < simpleHands[i].fingers.size(); j++){
                int id = simpleHands[i].fingers[j].id;
                
                ofPolyline & polyline = fingerTrails[id];
                ofPoint pt = simpleHands[i].fingers[j].pos;
                
                if (i == 0) {
                    XyPad1x = ofMap(simpleHands[0].fingers[j].pos.x, -ofGetWidth()/2, ofGetWidth()/2, 0.0, 1.0, true);
                    synth.setParameter(8, kAudioUnitScope_Global, XyPad1x);
                    
                    XyPad1y = ofMap(simpleHands[0].fingers[j].pos.y, -ofGetHeight()/2, ofGetHeight()/2, 0.0, 1.0, true);
                    synth.setParameter(9, kAudioUnitScope_Global, XyPad1y);
                }
                
                if (i == 1) {
                    XyPad2x = ofMap(simpleHands[1].fingers[j].pos.x, -ofGetWidth()/2, ofGetWidth()/2, 0.0, 1.0, true);
                    synth.setParameter(8, kAudioUnitScope_Global, XyPad2x);
                    
                    XyPad2y = ofMap(simpleHands[1].fingers[j].pos.y, -ofGetHeight()/2, ofGetHeight()/2, 0.0, 1.0, true);
                    synth.setParameter(9, kAudioUnitScope_Global, XyPad2y);
                }
                
                //add our point to our trail
                polyline.addVertex(pt);
                
                if (polyline.size() == 1) {
                    synth.midiNoteOn(60+dorian[id%7], 127);
                }
            }
        }
    }
    
    //post[2]->setEnabled(bKaleidoscope);
}

//--------------------------------------------------------------
void testApp::draw(){
    post.begin();
    if (bUseFbo) {
        fadesFbo.begin();
        ofFill();
        ofSetColor(0,0,0, fadeAmnt);
        ofRect(0,0,fadesFbo.getWidth(),fadesFbo.getHeight());
    }
    
    //post.begin();
    cam.begin();
	ofEnableLighting();
	l1.enable();
	l2.enable();
	
	m1.begin();
	m1.setShininess(0.6);
    
    ofSetColor(200, 200, 250);
    for(int i = 0; i < simpleHands.size(); i++){
        for(int j = 0; j < simpleHands[i].fingers.size(); j++){
            ofSphere(simpleHands[i].fingers[j].pos, 15);
        }
    }
    
    
    //temporarily draw polylines to test that code
	for(int i = 0; i < fingersFound.size(); i++){
		ofxStrip strip;
		int id = fingersFound[i];
		
		ofPolyline & polyline = fingerTrails[id];
        vector<float> widthsFromFFT;
        const float kWidthMultiplier = 20.0f;
        vector<ofFloatColor> stripColor;
        
        if (bFlip) {
            for(int j = 0 ; j < polyline.size() ; j++)
            {
                int fftIdx = (int)(((float)j / polyline.size())*255.0f);
                widthsFromFFT.push_back(spectrum[fftIdx]*kWidthMultiplier);
            }
            
            
            for(int j = 0 ; j < polyline.size() ; j++)
            {
                int waveformIdx = (int)(((float)j / polyline.size())*511.0f);
                float sampleNorm = (buffer[waveformIdx] + 1) / 2;
                ofFloatColor currColor((float)id / 10.0, sampleNorm * sampleNorm * 2, float((id * 23) % 10) / 20.0 + sampleNorm);
                stripColor.push_back(currColor);
            }

        }
        else {
            for(int j = 0 ; j < polyline.size() ; j++)
            {
                int fftIdx = (int)(((float)j / polyline.size())*511.0f);
                widthsFromFFT.push_back(buffer[fftIdx]*kWidthMultiplier);
            }
            
            
            for(int j = 0 ; j < polyline.size() ; j++)
            {
                int waveformIdx = (int)(((float)j / polyline.size())*255.0f);
                float sampleNorm = (spectrum[waveformIdx] + 1) / 2;
                ofFloatColor currColor((float)id / 10.0, sampleNorm * sampleNorm * 2, float((id * 23) % 10) / 20.0 + sampleNorm);
                stripColor.push_back(currColor);
            }
        }
        
        strip.generate(polyline.getVertices(), widthsFromFFT, stripColor, ofPoint(0, 0.5, 0.5) );
		
		ofSetColor(255 - id * 15, 0, id * 25);
		
        strip.getMesh().draw();
        
	}
    cam.end();
    m1.end();
    
    
    ofDisableLighting();
    if (bUseFbo) {
        fadesFbo.end();
        
        ofPushMatrix();
        /*glMatrixMode(GL_PROJECTION);
        ofScale(1.0, -1.0, 1.0);*/
        ofSetColor(255, 255, 255, 255);
        fadesFbo.draw(0, 0, ofGetWidth(), ofGetHeight());
        ofPopMatrix();
    }
    post.end();
    ofSetColor(200);
    //ofDrawBitmapString("hands " + ofToString(leap.getSimpleHands().size()) , 600, 20);
   /* ofDrawBitmapString("fingers 0" + ofToString(leap.getSimpleHands()[0].fingers.size()), 600, 40);
    ofDrawBitmapString("fingers 1" + ofToString(leap.getSimpleHands()[1].fingers.size()), 600, 40);*/
}

//--------------------------------------------------------------
void testApp::audioOut(float * input, int bufferSize, int nChannels) {

}

//--------------------------------------------------------------
void testApp::exit()
{
    gui->saveSettings("GUI/guiSettings.xml");
    delete gui;
}

//--------------------------------------------------------------
void testApp::guiEvent(ofxUIEventArgs &e)
{
    if(e.widget->getName() == "BACKGROUND VALUE")
    {
        ofxUISlider *slider = (ofxUISlider *) e.widget;
        ofBackground(slider->getScaledValue());
    }
    else if(e.widget->getName() == "FULLSCREEN")
    {
        ofxUIToggle *toggle = (ofxUIToggle *) e.widget;
        ofSetFullscreen(toggle->getValue());
    }
    else if(e.widget->getName() == "XyPad1x")
    {
        ofxUISlider *slider = (ofxUISlider *) e.widget;
        synth.setParameter(8, kAudioUnitScope_Global, slider->getScaledValue());
    }
    else if(e.widget->getName() == "XyPad1y")
    {
        ofxUISlider *slider = (ofxUISlider *) e.widget;
        synth.setParameter(9, kAudioUnitScope_Global, slider->getScaledValue());
    }
    else if(e.widget->getName() == "XyPad2x")
    {
        ofxUISlider *slider = (ofxUISlider *) e.widget;
        synth.setParameter(10, kAudioUnitScope_Global, slider->getScaledValue());
    }
    else if(e.widget->getName() == "XyPad2y")
    {
        ofxUISlider *slider = (ofxUISlider *) e.widget;
        synth.setParameter(11, kAudioUnitScope_Global, slider->getScaledValue());
    }
    else if(e.widget->getName() == "LAST PATCH")
    {
        ofxUIButton *button = (ofxUIButton *) e.widget;
        if(button->getValue())
        {
            prog--;
            if (prog < 0) prog = 0;
            synth.setProgram(prog);
        }
    }
    else if(e.widget->getName() == "NEXT PATCH")
    {
        ofxUIButton *button = (ofxUIButton *) e.widget;
        if(button->getValue())
        {
            prog++;
            if (prog > 127) prog = 127;
            synth.setProgram(prog);
        }
    }
    else if(e.widget->getName() == "fade amnt")
    {
        ofxUISlider *slider = (ofxUISlider *) e.widget;
    }
    
    
    //params
    /*
     #8: _XyPad1x [0 ~ 1]
     #9: _XyPad1y [0 ~ 1]
     #10: _XyPad2x [0 ~ 1]
     #11: _XyPad2y [0 ~ 1]
     */
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    if (key == 'c') {
        synth.showUI();
    }
    if (key == 'g') {
        gui->setVisible(!gui->isVisible());
    }
    if (key == 'f') {
        bUseFbo = !bUseFbo;
    }
    if (key == 's') {
        bFlip = !bFlip;
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}