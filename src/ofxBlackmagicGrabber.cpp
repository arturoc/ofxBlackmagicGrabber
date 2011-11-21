/*
 * ofxBlackmagicGrabber.cpp
 *
 *  Created on: 06/10/2011
 *      Author: arturo
 */
 
// YUV 2 RGB conversion by James Hughes under the following terms
// Copyright (c) 2011, James Hughes
// All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ofxBlackmagicGrabber.h"
#include "Poco/ScopedLock.h"

string ofxBlackmagicGrabber::LOG_NAME="ofxBlackmagicGrabber";

// tables are done for all possible values 0 - 255 of yuv
// rather than just "legal" values of yuv.
// two dimensional arrays for red &  blue, three dimensions for green
void ofxBlackmagicGrabber::CreateLookupTables(){

    int yy, uu, vv, ug_plus_vg, ub, vr, val;

    // Red
    for (int y = 0; y < 256; y++) {
        for (int v = 0; v < 256; v++) {
            yy         = y << 8;
            vv         = v - 128;
            vr         = vv * 359;
            val        = (yy + vr) >>  8;
            red[y][v]  = Clamp(val);
        }
    }

    // Blue
    for (int y = 0; y < 256; y++) {
        for (int u = 0; u < 256; u++) {
            yy          = y << 8;
            uu          = u - 128;
            ub          = uu * 454;
            val         = (yy + ub) >> 8;
            blue[y][u]  = Clamp(val);
        }
    }

    // Green
    for (int y = 0; y < 256; y++) {
        for (int u = 0; u < 256; u++) {
            for (int v = 0; v < 256; v++) {
                yy              = y << 8;
                uu              = u - 128;
                vv              = v - 128;
                ug_plus_vg      = uu * 88 + vv * 183;
                val             = (yy - ug_plus_vg) >> 8;
                green[y][u][v]  = Clamp(val);
            }
        }
    }
}

void ofxBlackmagicGrabber::yuvToRGB(IDeckLinkVideoInputFrame * videoFrame)
{
    // convert 4 YUV macropixels to 6 RGB pixels
    unsigned int boundry = videoFrame->GetWidth() * videoFrame->GetHeight() * 2;
    unsigned char y, u, v;
    unsigned char * yuv;
    videoFrame->GetBytes((void**)&yuv);
    ofPixels & pixels = *backPixels;

    unsigned int j=0;

	//#pragma omp for
    for(unsigned int i=0; i<boundry; i+=4, j+=6){
        y = yuv[i+1];
        u = yuv[i];
        v = yuv[i+2];

        pixels[j]   = red[y][v];
        pixels[j+1] = green[y][u][v];
        pixels[j+2] = blue[y][u];

        y = yuv[i+3];

        pixels[j+3] = red[y][v];
        pixels[j+4] = green[y][u][v];
        pixels[j+5] = blue[y][u];
    }

}

// End of YUV 2 RGB conversion


// List of known pixel formats and their matching display names
static const BMDPixelFormat	gKnownPixelFormats[]		= {bmdFormat8BitYUV, bmdFormat10BitYUV, bmdFormat8BitARGB, bmdFormat8BitBGRA, bmdFormat10BitRGB, 0};
static const char *			gKnownPixelFormatNames[]	= {" 8-bit YUV", "10-bit YUV", "8-bit ARGB", "8-bit BGRA", "10-bit RGB", NULL};

static void	print_attributes (IDeckLink* deckLink)
{
	IDeckLinkAttributes*				deckLinkAttributes = NULL;
	bool								supported;
	int64_t								count;
	char *								serialPortName = NULL;
	HRESULT								result;

	// Query the DeckLink for its attributes interface
	result = deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkAttributes interface - result = %08x\n", result);
		goto bail;
	}

	// List attributes and their value
	printf("Attribute list:\n");

	result = deckLinkAttributes->GetFlag(BMDDeckLinkHasSerialPort, &supported);
	if (result == S_OK)
	{
		printf(" %-40s %s\n", "Serial port present ?", (supported == true) ? "Yes" : "No");

		if (supported)
		{
			result = deckLinkAttributes->GetString(BMDDeckLinkSerialPortDeviceName, (const char **) &serialPortName);
			if (result == S_OK)
			{
				printf(" %-40s %s\n", "Serial port name: ", serialPortName);
				free(serialPortName);

			}
			else
			{
				fprintf(stderr, "Could not query the serial port presence attribute- result = %08x\n", result);
			}
		}

	}
	else
	{
		fprintf(stderr, "Could not query the serial port presence attribute- result = %08x\n", result);
	}

    result = deckLinkAttributes->GetInt(BMDDeckLinkNumberOfSubDevices, &count);
    if (result == S_OK)
    {
        printf(" %-40s %lld\n", "Number of sub-devices:",  count);
        if (count != 0)
        {
            result = deckLinkAttributes->GetInt(BMDDeckLinkSubDeviceIndex, &count);
            if (result == S_OK)
            {
                printf(" %-40s %lld\n", "Sub-device index:",  count);
            }
            else
            {
                fprintf(stderr, "Could not query the sub-device index attribute- result = %08x\n", result);
            }
        }
    }
    else
    {
        fprintf(stderr, "Could not query the number of sub-device attribute- result = %08x\n", result);
    }

	result = deckLinkAttributes->GetInt(BMDDeckLinkMaximumAudioChannels, &count);
	if (result == S_OK)
	{
		printf(" %-40s %lld\n", "Number of audio channels:",  count);
	}
	else
	{
		fprintf(stderr, "Could not query the number of supported audio channels attribute- result = %08x\n", result);
	}

	result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supported);
	if (result == S_OK)
	{
		printf(" %-40s %s\n", "Input mode detection supported ?", (supported == true) ? "Yes" : "No");
	}
	else
	{
		fprintf(stderr, "Could not query the input mode detection attribute- result = %08x\n", result);
	}

	result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInternalKeying, &supported);
	if (result == S_OK)
	{
		printf(" %-40s %s\n", "Internal keying supported ?", (supported == true) ? "Yes" : "No");
	}
	else
	{
		fprintf(stderr, "Could not query the internal keying attribute- result = %08x\n", result);
	}

	result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsExternalKeying, &supported);
	if (result == S_OK)
	{
		printf(" %-40s %s\n", "External keying supported ?", (supported == true) ? "Yes" : "No");
	}
	else
	{
		fprintf(stderr, "Could not query the external keying attribute- result = %08x\n", result);
	}

	result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsHDKeying, &supported);
	if (result == S_OK)
	{
		printf(" %-40s %s\n", "HD-mode keying supported ?", (supported == true) ? "Yes" : "No");
	}
	else
	{
		fprintf(stderr, "Could not query the HD-mode keying attribute- result = %08x\n", result);
	}

bail:
	printf("\n");
	if(deckLinkAttributes != NULL)
		deckLinkAttributes->Release();

}

static void	print_output_modes (IDeckLink* deckLink)
{
	IDeckLinkOutput*					deckLinkOutput = NULL;
	IDeckLinkDisplayModeIterator*		displayModeIterator = NULL;
	IDeckLinkDisplayMode*				displayMode = NULL;
	HRESULT								result;

	// Query the DeckLink for its configuration interface
	result = deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&deckLinkOutput);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkOutput interface - result = %08x\n", result);
		goto bail;
	}

	// Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
	result = deckLinkOutput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
		goto bail;
	}

	// List all supported output display modes
	printf("Supported video output display modes and pixel formats:\n");
	while (displayModeIterator->Next(&displayMode) == S_OK)
	{
		char *			displayModeString = NULL;

		result = displayMode->GetName((const char **) &displayModeString);
		if (result == S_OK)
		{
			char					modeName[64];
			int						modeWidth;
			int						modeHeight;
			BMDTimeValue			frameRateDuration;
			BMDTimeScale			frameRateScale;
			int						pixelFormatIndex = 0; // index into the gKnownPixelFormats / gKnownFormatNames arrays
			BMDDisplayModeSupport	displayModeSupport;


			// Obtain the display mode's properties
			modeWidth = displayMode->GetWidth();
			modeHeight = displayMode->GetHeight();
			displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);
			printf(" %-20s \t %d x %d \t %7g FPS\t", displayModeString, modeWidth, modeHeight, (double)frameRateScale / (double)frameRateDuration);

			// Print the supported pixel formats for this display mode
			while ((gKnownPixelFormats[pixelFormatIndex] != 0) && (gKnownPixelFormatNames[pixelFormatIndex] != NULL))
			{
				if ((deckLinkOutput->DoesSupportVideoMode(displayMode->GetDisplayMode(), gKnownPixelFormats[pixelFormatIndex], bmdVideoOutputFlagDefault, &displayModeSupport, NULL) == S_OK)
						&& (displayModeSupport != bmdDisplayModeNotSupported))
				{
					printf("%s\t", gKnownPixelFormatNames[pixelFormatIndex]);
				}
				pixelFormatIndex++;
			}

			printf("\n");

			free(displayModeString);
		}

		// Release the IDeckLinkDisplayMode object to prevent a leak
		displayMode->Release();
	}

	printf("\n");

bail:
	// Ensure that the interfaces we obtained are released to prevent a memory leak
	if (displayModeIterator != NULL)
		displayModeIterator->Release();

	if (deckLinkOutput != NULL)
		deckLinkOutput->Release();
}


static void	print_capabilities (IDeckLink* deckLink)
{
	IDeckLinkAttributes*		deckLinkAttributes = NULL;
	int64_t						ports;
	int							itemCount;
	HRESULT						result;

	// Query the DeckLink for its configuration interface
	result = deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkAttributes interface - result = %08x\n", result);
		goto bail;
	}

	printf("Supported video output connections:\n  ");
	itemCount = 0;
	result = deckLinkAttributes->GetInt(BMDDeckLinkVideoOutputConnections, &ports);
	if (result == S_OK)
	{
		if (ports & bmdVideoConnectionSDI)
		{
			itemCount++;
			printf("SDI");
		}

		if (ports & bmdVideoConnectionHDMI)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("HDMI");
		}

		if (ports & bmdVideoConnectionOpticalSDI)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("Optical SDI");
		}

		if (ports & bmdVideoConnectionComponent)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("Component");
		}

		if (ports & bmdVideoConnectionComposite)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("Composite");
		}

		if (ports & bmdVideoConnectionSVideo)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("S-Video");
		}
	}
	else
	{
		fprintf(stderr, "Could not obtain the list of output ports - result = %08x\n", result);
		goto bail;
	}

	printf("\n\n");

	printf("Supported video input connections:\n  ");
	itemCount = 0;
	result = deckLinkAttributes->GetInt(BMDDeckLinkVideoInputConnections, &ports);
	if (result == S_OK)
	{
		if (ports & bmdVideoConnectionSDI)
		{
			itemCount++;
			printf("SDI");
		}

		if (ports & bmdVideoConnectionHDMI)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("HDMI");
		}

		if (ports & bmdVideoConnectionOpticalSDI)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("Optical SDI");
		}

		if (ports & bmdVideoConnectionComponent)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("Component");
		}

		if (ports & bmdVideoConnectionComposite)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("Composite");
		}

		if (ports & bmdVideoConnectionSVideo)
		{
			if (itemCount++ > 0)
				printf(", ");
			printf("S-Video");
		}
	}
	else
	{
		fprintf(stderr, "Could not obtain the list of input ports - result = %08x\n", result);
		goto bail;
	}
	printf("\n");

bail:
	if (deckLinkAttributes != NULL)
		deckLinkAttributes->Release();
}


ofxBlackmagicGrabber::ofxBlackmagicGrabber() {
	inputFlags = 0;
	selectedDisplayMode = bmdModePAL;
	pixelFormat = bmdFormat8BitYUV;
	g_timecodeFormat = 0;
	g_videoModeIndex = -1;
	g_audioChannels = 2;
	g_audioSampleDepth = 16;
	bNewFrameArrived = false;
	bIsNewFrame = false;
	frameCount = 0;
	deckLinkInput = 0;
	deckLink = 0;
	currentPixels = &pixels[0];
	backPixels = &pixels[1];
	deviceID = 0;
	bDeinterlace = false;
	CreateLookupTables();
}

ofxBlackmagicGrabber::~ofxBlackmagicGrabber() {
	// TODO Auto-generated destructor stub
}

void ofxBlackmagicGrabber::setVideoMode(_BMDDisplayMode videoMode){
	g_videoModeIndex = videoMode;
}

void ofxBlackmagicGrabber::setDeinterlace(bool deinterlace){
	bDeinterlace = deinterlace;
}

inline unsigned char Clamp(int value)
{
    if(value > 255) return 255;
    if(value < 0)   return 0;
    return value;
}

void ofxBlackmagicGrabber::deinterlace(){
	 for (int i = 1; i < backPixels->getHeight()-1; i++){

		unsigned char * momspixa = backPixels->getPixels() + ((i-1) * backPixels->getWidth()*3);
		unsigned char * momspixb = backPixels->getPixels() + ((i) * backPixels->getWidth()*3);
		unsigned char * momspixc =backPixels->getPixels() + ((i+1) * backPixels->getWidth()*3);

		unsigned char * mepix = backPixels->getPixels() + (i * backPixels->getWidth()*3);

		for (int j = 0; j < backPixels->getWidth()*3; j++){
			if (i % 2 == 0) mepix[j] = momspixb[j];  // should be memcopy!!
			else{
				int pix = (int)(0.5f * (unsigned char)momspixa[j]) + (0.5f * (unsigned char)momspixc[j]);
			  mepix[j] = pix;
			}
		}
	}
}

HRESULT ofxBlackmagicGrabber::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode* displayMode, BMDDetectedVideoInputFormatFlags flags){
	long num,den;
	displayMode->GetFrameRate(&num,&den);
	ofLogVerbose(LOG_NAME) << "video format changed" << displayMode->GetWidth() << "x" << displayMode->GetHeight() << "fps: " << num << "/" << den;

	pixels[0].allocate(displayMode->GetWidth(),displayMode->GetHeight(),OF_IMAGE_COLOR);
	pixels[1].allocate(displayMode->GetWidth(),displayMode->GetHeight(),OF_IMAGE_COLOR);

	return S_OK;
}

HRESULT ofxBlackmagicGrabber::VideoInputFrameArrived(IDeckLinkVideoInputFrame * videoFrame, IDeckLinkAudioInputPacket * audioFrame){
	IDeckLinkVideoFrame*	                rightEyeFrame = NULL;
	IDeckLinkVideoFrame3DExtensions*        threeDExtensions = NULL;

	// Handle Video Frame
	if(videoFrame)
	{
		// If 3D mode is enabled we retreive the 3D extensions interface which gives.
		// us access to the right eye frame by calling GetFrameForRightEye() .
		if ( (videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions, (void **) &threeDExtensions) != S_OK) ||
			(threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK))
		{
			rightEyeFrame = NULL;
		}

		if (threeDExtensions)
			threeDExtensions->Release();

		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource){
			ofLogError(LOG_NAME) <<  "Frame received (#" << frameCount << "- No input signal detected";
		}
		/*else
		{*/
			const char *timecodeString = NULL;
			if (g_timecodeFormat != 0)
			{
				IDeckLinkTimecode *timecode;
				if (videoFrame->GetTimecode(g_timecodeFormat, &timecode) == S_OK)
				{
					timecode->GetString(&timecodeString);
				}
			}

			ofLogVerbose(LOG_NAME) << "Frame received (#" <<  frameCount
					<< ") [" << (timecodeString != NULL ? timecodeString : "No timecode")
					<< "] -" << (rightEyeFrame != NULL ? "Valid Frame (3D left/right)" : "Valid Frame")
					<< "- Size: " << (videoFrame->GetRowBytes() * videoFrame->GetHeight()) << "bytes";

			if (timecodeString)
				free((void*)timecodeString);

			yuvToRGB(videoFrame);
			if(bDeinterlace) deinterlace();
			pixelsMutex.lock();
			bNewFrameArrived = true;
			ofPixels * aux = currentPixels;
			currentPixels = backPixels;
			backPixels = aux;
			pixelsMutex.unlock();
		//}

		if (rightEyeFrame)
			rightEyeFrame->Release();

		frameCount++;
	}

#if 0	//No audio
	// Handle Audio Frame
	void*	audioFrameBytes;
	if (audioFrame)
	{
		if (audioOutputFile != -1)
		{
			audioFrame->GetBytes(&audioFrameBytes);
			write(audioOutputFile, audioFrameBytes, audioFrame->GetSampleFrameCount() * g_audioChannels * (g_audioSampleDepth / 8));
		}
	}
#endif
	return S_OK;
}

void ofxBlackmagicGrabber::listDevices() {
	IDeckLinkIterator*		deckLinkIterator;
	IDeckLink*				deckLink;
	int						numDevices = 0;
	HRESULT					result;

	// Create an IDeckLinkIterator object to enumerate all DeckLink cards in the system
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL){
		ofLogError(LOG_NAME) <<  "A DeckLink iterator could not be created.  The DeckLink drivers may not be installed.";
	}

	// Enumerate all cards in this system
	while (deckLinkIterator->Next(&deckLink) == S_OK){
		char *		deviceNameString = NULL;

		// Increment the total number of DeckLink cards found
		numDevices++;
		if (numDevices > 1)
			printf("\n\n");

		// *** Print the model name of the DeckLink card
		result = deckLink->GetModelName((const char **) &deviceNameString);
		if (result == S_OK)
		{
			printf("=============== %s ===============\n\n", deviceNameString);
			free(deviceNameString);
		}

		print_attributes(deckLink);

		// ** List the video output display modes supported by the card
		print_output_modes(deckLink);

		// ** List the input and output capabilities of the card
		print_capabilities(deckLink);

		// Release the IDeckLink instance when we've finished with it to prevent leaks
		deckLink->Release();
	}

	deckLinkIterator->Release();

	// If no DeckLink cards were found in the system, inform the user
	if (numDevices == 0)
		ofLogError(LOG_NAME) <<  "No Blackmagic Design devices were found.";

}

bool ofxBlackmagicGrabber::initGrabber(int w, int h) {
	IDeckLinkIterator			*deckLinkIterator = CreateDeckLinkIteratorInstance();
	int							displayModeCount = 0;
	int							exitStatus = 1;
	bool 						foundDisplayMode = false;
	HRESULT						result;
	IDeckLinkDisplayModeIterator	*displayModeIterator;

	if (!deckLinkIterator){
		ofLogError(LOG_NAME) <<  "This application requires the DeckLink drivers installed.";
		goto bail;
	}

	for(int i=0;i<deviceID+1;i++){
		result = deckLinkIterator->Next(&deckLink);
		if (result != S_OK){
			ofLogError(LOG_NAME) <<  "Couldn't open device" << deviceID;
			goto bail;
		}
	}

	if (deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK)
		goto bail;

	deckLinkInput->SetCallback(this);

	// Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK){
		ofLogError(LOG_NAME) << "Could not obtain the video output display mode iterator - result =" << result;
		goto bail;
	}


	if (g_videoModeIndex < 0){
		ofLogError(LOG_NAME) <<  "No video mode specified, specify it before initGrabber using setVideoMode";
		goto bail;
	}


	while (displayModeIterator->Next(&displayMode) == S_OK){
		if (g_videoModeIndex == displayMode->GetDisplayMode()){
			BMDDisplayModeSupport result;
			const char *displayModeName;

			foundDisplayMode = true;
			displayMode->GetName(&displayModeName);
			selectedDisplayMode = displayMode->GetDisplayMode();

			pixels[0].allocate(displayMode->GetWidth(),displayMode->GetHeight(),OF_IMAGE_COLOR);
			pixels[1].allocate(displayMode->GetWidth(),displayMode->GetHeight(),OF_IMAGE_COLOR);

			ofLogVerbose(LOG_NAME) << "device initialized:" << displayMode->GetWidth() << displayMode->GetHeight();

			deckLinkInput->DoesSupportVideoMode(selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &result, NULL);

			if (result == bmdDisplayModeNotSupported){
				ofLogError(LOG_NAME) <<  "The display mode" << displayModeName << "is not supported with the selected pixel format";
				goto bail;
			}

			if (inputFlags & bmdVideoInputDualStream3D){
				if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D)){
					ofLogError(LOG_NAME) <<  "The display mode" << displayModeName  << "is not supported with 3D";
					goto bail;
				}
			}

			break;
		}
		displayModeCount++;
		displayMode->Release();
	}

	if (!foundDisplayMode){
		ofLogError(LOG_NAME) <<  "Invalid mode" << g_videoModeIndex << "specified";
		goto bail;
	}

	result = deckLinkInput->EnableVideoInput(selectedDisplayMode, pixelFormat, inputFlags);
	if(result != S_OK){
		ofLogError(LOG_NAME) <<  "Failed to enable video input. Is another application using the card?";
		goto bail;
	}

#if 0  // no audio by now
	result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_audioSampleDepth, g_audioChannels);
	if(result != S_OK){
		goto bail;
	}
#endif

	result = deckLinkInput->StartStreams();
	if(result != S_OK){
		goto bail;
	}

	// All Okay.
	exitStatus = 0;
	return true;

bail:

	if (displayModeIterator != NULL){
		displayModeIterator->Release();
		displayModeIterator = NULL;
	}

	if (deckLinkIterator != NULL)
		deckLinkIterator->Release();

	close();

	return false;
}

void ofxBlackmagicGrabber::update() {
	pixelsMutex.lock();
	if(bNewFrameArrived){
		bIsNewFrame = true;
		bNewFrameArrived = false;
	}else{
		bIsNewFrame = false;
	}
	pixelsMutex.unlock();
}

bool ofxBlackmagicGrabber::isFrameNew() {
	return bIsNewFrame;
}


unsigned char * ofxBlackmagicGrabber::getPixels() {
	Poco::ScopedLock<ofMutex> lock(pixelsMutex);
	return currentPixels->getPixels();
}

ofPixels & ofxBlackmagicGrabber::getPixelsRef() {
	Poco::ScopedLock<ofMutex> lock(pixelsMutex);
	return *currentPixels;
}


void ofxBlackmagicGrabber::close() {
	if (deckLinkInput != NULL){
		deckLinkInput->Release();
		deckLinkInput = NULL;
	}

	if (deckLink != NULL){
		deckLink->Release();
		deckLink = NULL;
	}

	pixels[0].clear();
	pixels[1].clear();
	frameCount = 0;
}


float ofxBlackmagicGrabber::getHeight() {
	return pixels[0].getHeight();
}

float ofxBlackmagicGrabber::getWidth() {
	return pixels[0].getWidth();
}


void ofxBlackmagicGrabber::setVerbose(bool bTalkToMe) {
	if(bTalkToMe) ofSetLogLevel(LOG_NAME,OF_LOG_VERBOSE);
	else  ofSetLogLevel(LOG_NAME,OF_LOG_NOTICE);
}

void ofxBlackmagicGrabber::setDeviceID(int _deviceID) {
	deviceID = _deviceID;
}

void ofxBlackmagicGrabber::setDesiredFrameRate(int framerate) {

}

void ofxBlackmagicGrabber::videoSettings() {

}

void ofxBlackmagicGrabber::setPixelFormat(ofPixelFormat pixelFormat) {

}

ofPixelFormat ofxBlackmagicGrabber::getPixelFormat() {
	return OF_PIXELS_RGB;
}
