/*
 * ofxBlackmagicGrabber.h
 *
 *  Created on: 06/10/2011
 *      Author: arturo
 */

#ifndef OFXBLACKMAGICGRABBER_H_
#define OFXBLACKMAGICGRABBER_H_

#include "DeckLinkAPI.h"
#include "ofBaseTypes.h"

class ofxBlackmagicGrabber: public ofBaseVideoGrabber, public IDeckLinkInputCallback {
public:
	ofxBlackmagicGrabber();
	virtual ~ofxBlackmagicGrabber();

	//specific blackmagic

	void setVideoMode(_BMDDisplayMode videoMode);
	void setDeinterlace(bool deinterlace);

	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame * videoFrame, IDeckLinkAudioInputPacket * audioFrame);
	virtual HRESULT QueryInterface(REFIID, void**){}
	virtual ULONG AddRef(){}
	virtual ULONG Release(){}
	// common ofBaseVideoGrabber

	void	listDevices();
	bool	initGrabber(int w, int h);
	void	update();
	bool	isFrameNew();

	unsigned char 	* getPixels();
	ofPixels & getPixelsRef();

	void	close();

	float	getHeight();
	float	getWidth();

	void setVerbose(bool bTalkToMe);
	void setDeviceID(int _deviceID);
	void setDesiredFrameRate(int framerate);
	void videoSettings();
	void setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat getPixelFormat();

	static string LOG_NAME;

private:
	IDeckLink 					*deckLink;
	IDeckLinkInput				*deckLinkInput;
	IDeckLinkDisplayMode		*displayMode;
	BMDVideoInputFlags			inputFlags;
	BMDDisplayMode				selectedDisplayMode;
	BMDPixelFormat				pixelFormat;

	BMDTimecodeFormat		g_timecodeFormat;
	int						g_videoModeIndex;
	int						g_audioChannels;
	int						g_audioSampleDepth;

	ofPixels				pixels[2];
	ofPixels *				currentPixels;
	ofPixels *				backPixels;
	unsigned long			frameCount;

	void yuvToRGB(IDeckLinkVideoInputFrame * videoFrame);
	void CreateLookupTables();
	void deinterlace();

	unsigned char           red[256][256];
	unsigned char           blue[256][256];
	unsigned char           green[256][256][256];

	bool 					bNewFrameArrived;
	bool					bIsNewFrame;

	ofMutex					pixelsMutex;

	int						deviceID;

	bool					bDeinterlace;
};

#endif /* OFXBLACKMAGICGRABBER_H_ */
