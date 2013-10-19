#include "ofMain.h"
#include "ofxBlackmagicGrabber.h"

class ofApp : public ofBaseApp {
public:
	
	ofxBlackmagicGrabber cam;
	ofImage img;
	
	void setup() {
		cam.setVideoMode(bmdModeHD1080p30);
		cam.setDeinterlace(false);
		cam.initGrabber(1920, 1080);
	}
	void update() {
		cam.update();
		if(cam.isFrameNew()) {
			img.setFromPixels(cam.getPixelsRef());
			img.update();
		}
	}
	void draw() {
		img.draw(0, 0, ofGetWidth(), ofGetHeight());
	}
};

int main() {
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp(new ofApp());
}