#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

void ofApp::setup()
{
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
    finder.setup("hand2.xml");
    finder.setPreset(ObjectFinder::Fast);
	cam.setup(640, 480);
    
    fboFrame.allocate(640,480,GL_RGB);
    fboFrame.begin();
    ofClear(0);
    fboFrame.end();
    
    gui.setup();
    //gui.add(resetBackground.set("Reset Background", false));
    gui.add(learningTime.set("Learning Time", 30, 0, 30));
    gui.add(thresholdValue.set("Threshold Value", 10, 0, 255));
    gui.add(erodeAmt.set("erodeAmt", 1, 0, 5));
    gui.add(dilateAmt.set("dilateAmt", 1,0, 5));
    gui.add(blurAmt.set("blurAmt", 1, 0, 5));
    gui.add(smooth.set("smooth", 1, 0, 1));
    
    pointer.set(-100, -100);
    oldPos.set(-100, -100);
    
    bOld = false;
    frameLost = 0;
    
    // init OSC
    ip = "127.0.0.1";
    port = 12345;
    oscOut.setup(ip, port);
    
}

void ofApp::update()
{
	cam.update();

    if(cam.isFrameNew()) {
        background.setLearningTime(learningTime);
        background.setThresholdValue(thresholdValue);
        background.update(cam, thresholded);
        ofxCv::erode(thresholded, thresholded, erodeAmt);
        ofxCv::dilate(thresholded, thresholded, dilateAmt);
        ofxCv::blur(thresholded, thresholded, blurAmt);
        
        thresholded.update();
        
        fboFrame.begin();
        cam.draw(0,0);
        ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
        thresholded.draw(0, 0);
        ofDisableBlendMode();
        fboFrame.end();
        
        ofPixels pix;
        fboFrame.readToPixels(pix);
        
        finder.update(pix);
        
        ofVec2f tempPos;
        tempPos = pointer;
        
        if(finder.size() > 0)
        {
            tempPos = finder.getObject(0).getCenter();
            bOld = true;
            frameLost = 0;
        }
        else
        {
            // prima volta palla persa
            if(bOld)
            {
                bOld = false;
                frameLost = 1;
            }
            // era giˆ persa
            else
            {
                if(frameLost < 30)
                    frameLost++;
                else
                {
                    pointer.set(-100,-100);
                    tempPos.set(-100, -100);
                }
            }
        }
        
        pointer = (tempPos * smooth) + (oldPos * (1-smooth));
        if(pointer.x > 0)
            sendMessage("pointer/", pointer);
        
        oldPos = pointer;
    }
}

void ofApp::draw()
{
	fboFrame.draw(0, 0);
    
    finder.draw();
	//ofDrawBitmapStringHighlight(ofToString(finder.size()), 10, 20);
    
    if(thresholded.isAllocated())
    {
        thresholded.draw(640, 0);
    }
    
    ofPushStyle();
    ofSetColor(255,0,0);
    ofFill();
    ofDrawCircle(pointer.x, pointer.y, 30);
    ofPopStyle();
    
    gui.draw();
}

void ofApp::sendMessage(const string & addr, ofVec2f & arg)
{
    ofxOscMessage msg;
    msg.setAddress(addr);
    msg.addFloatArg(arg.x);
    msg.addFloatArg(arg.y);
    oscOut.sendMessage(msg, false);
}

void ofApp::keyPressed(int key)
{
    if(key == ' ')
      background.reset();
}
