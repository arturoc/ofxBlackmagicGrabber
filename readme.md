# ofxBlackMagicGrabber

Grabs live video from [Black Magic](http://www.blackmagic-design.com/). This addon has been tested on OSX with the UltraStudio Mini Recorder.

First, install the []Black Magic software](http://www.blackmagicdesign.com/support). If you are using an UltraStudio Mini Recorder, you should download [Desktop Video 9.8 for Macintosh](http://www.blackmagicdesign.com/support/detail?sid=3958&pid=31781&leg=false&os=mac). After installation, go to System Preferences, click "Black Magic Design" and make sure "Use 1080p not 1080PsF" is checked (this option is only available when the capture card is plugged in).

Then go to the [support](http://www.blackmagicdesign.com/support/sdks) page and download the DeckLink SDK (currently at version 9.7.7). After unzipping the SDK open the app `Mac/Samples/bin/CapturePreview` and select the video format of your device and hit "Start". If you have the right mode selected you should see the video streaming.

One you see the demo app working, try building and running the `example/` that comes with `ofxBlackMagicGrabber`.