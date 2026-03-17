/*

	OpenFrameworks NDI tester receiver or sender

	#define BUILDRECEIVER in ofApp.h for conditional build

	Pair of sender and receiver apps which help debugging and introspecting NDI frame transmission.

	Copyright (C) 2016-2026 Lynn Jarvis.
	https://www.spout.zeal.co
	Copyright (C) 2026 Daan de Lange.
	https://daandelange.com/

	=========================================================================
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	=========================================================================

	13.10.16 - Addon receiver example created
	14.10.16 - Included received frame rate
	03.11.16 - Receive into image pixels directly
			 - Add a sender selection dialog
	05.11.16 - Note - dialog is Windows only
			   the ofxNDIdialog class is used separately by the application
			   and can be omitted along with resources
	09.02.17 - Updated to ofxNDI with Version 2 NDI SDK
			 - Added changes by Harvey Buchan to optionally
			   specify preferred pixel format in CreateReceiver
	22.02.17 - updated to Openframeworks 0.9.8
			 - corrected reallocate on size change
	03.11.17 - update for ofxNDI with NDI Version 3
			 - remove sender dialog
			 - change NDI deprecated functions to Vers 3
	01.04.18 - Revise for NDI Version 3
			 - RGBA format not used due to SDK problem
	11.06.18 - Updated for NDI vers 3.5
			   ReceiveImage without memory copy to a pixel buffer
	12.07.18 - ReceiveImage ofFbo/ofTexture/ofImage
			 - All size change checks in ofxDNIreceiver class
	06.08.18 - Include all receiving options in example
	27.03.19 - Add example of using ReceiverCreated, ReceiverConnected and GetSenderFps
	10.11.19 - Revise for ofxNDI for NDI SDK Version 4.0
	28.02.20 - Remove initial texture clear.
			   Add received fps to on-screen display
	08.12.20 - Change from sprintf to std::string for on-screen display
	02.12,21 - Update pixel receive examples and comments
	04.12.21 - Use Setfromexternalpixels to update display image
	24.04.22 - Update examples for Visual Studio 2022
	04.07.22 - Update with revised ofxNDI. Rebuild x64/MD.
	05-08-22 - Update to NDI 5.5 (ofxNDI and bin\Processing.NDI.Lib.x64.dll)
	20-11-22 - Update to NDI 5.5.2 (ofxNDI and bin\Processing.NDI.Lib.x64.dll)
	27-04-23 - Update to NDI 5.5.4 (ofxNDI and bin\Processing.NDI.Lib.x64.dll)
			   Rebuild example executables x64/MD
	14-12-24 - Add #define BUILDRECEIVER in header for conditional build
	09.05.24 - Update to NDI 6.0.0
	17.05.24 - Update to NDI 6.0.1.0
	19.05.24 - ofxNDI async texture pixel load (LoadTexturePixels) for receiver
	20.05 24 - Add SetUpload to activate async pixel load
			   Extend comments for asynchronous sending
	27.05.24 - ofxNDIsender - SendImage
			     ofTexture - RGBA only
			   ofxNDIreceive - FindSenders
			     check for a name change at the same index and update m_senderName
			   ofxNDIsend - ReleaseSender
			     clear metadata, CreateSender - add sender name to metadata
			   Rebuild example sender/receiver x64/MD
	07.01.25 - Update to NDI 6.1.1.0
	10.01.25 - Sender - try a different sender name if initialization fails.
			   If it fails again, warn and quit.
	16.03.25 - SetUpload option default false for a receiver
			   SetReadback option default false for a sender
	18.03.25 - #ifdef for MessageBox Windows only
			   Console out OpenGL and Openframeworks versions
	11.04.25 - Use ofSystemAlertDialog in place of MessageBox (issue #60)
	21.07.25 - Update to NDI 6.2.0.3
	xx.xx.26 - Add audio features

*/
#include "ofApp.h"
#include <inttypes.h> // PRId64
#include <chrono>
#include <pugixml.hpp>

// Parse remote appmode from string
ofxNDItests ParseAppModeFromMetadata(const std::string& metadata){
	if(metadata.length()<=0u){
		return ofxNDItests_max;
	}
#ifdef HEADER_PUGIXML_HPP
	// Load PUGI
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string(metadata.c_str());
	if (!result) {
		return ofxNDItests_max;
	}

    // Traverse
    pugi::xml_node appModeNode = doc.child("appmode");
    if (appModeNode) {
        pugi::xml_text xmlValue = appModeNode.text();
        if(!xmlValue.empty()){
            std::string stringValue = xmlValue.as_string();
            std::size_t pos = stringValue.find(APPMODESTR);
            if(pos == 0){
                stringValue = stringValue.substr(strlen(APPMODESTR));
                int value = std::stoi(stringValue);
                if(value >= 0 && value < ofxNDItests_max){
                    //std::cout << "NewMode=" << (ofxNDItests)value << " // " << value << std::endl;
                    return (ofxNDItests) value;
                }
            }
        }
    }
#else
	// Without PUGI
	if(metadata.find("<appmode>")==0u){
		std::size_t cursor = strlen("<appmode>");
		if(metadata.find(APPMODESTR, cursor)==cursor){
			cursor += strlen(APPMODESTR);
			std::size_t cursorEnd = metadata.find("</", cursor);
			if(cursorEnd > cursor && cursorEnd != metadata.npos){
				// Take next 2 chars
				int nextMode = std::stoi(metadata.substr(cursor, cursor+2));
				//std::cout << "TestSenderAppMode=" << nextMode << " // " << metadata.substr(cursor, cursorEnd-cursor) << std::endl;
				return (ofxNDItests)nextMode;
			}
		}
	}
#endif
	return ofxNDItests_max;
}

//--------------------------------------------------------------
void ofApp::setup(){

	ofBackground(0);
	ofSetColor(255);
	ofSetWindowTitle(APPTITLE);

	// Query the OpenGL version string
	const char * version = (const char *)glGetString(GL_VERSION);
	std::cout << "OpenGL (" << version << ")" << std::endl;
	// Get Openframemorks and renderer OpenGL version
	int major = ofGetVersionMajor();
	int minor = ofGetVersionMinor();
	int glmajor = ofGetGLRenderer()->getGLVersionMajor();
	int glminor = ofGetGLRenderer()->getGLVersionMinor();
	std::cout << "Openframeworks " << major << "." << minor << " - OpenGL " << glmajor << "." << glminor << std::endl;
	// Check for :
	// glGenBuffersARB,	glDeleteBuffersARB, glBindBufferARB
	// glBufferDataARB, glMapBufferARB, glUnmapBufferARB
	if (GLEW_ARB_vertex_buffer_object) {
		std::cout << "GL_ARB_vertex_buffer_object is supported\n" << std::endl;
	}
	else {
		std::cout << "GL_ARB_vertex_buffer_object is not supported\n" << std::endl;
	}

	// Audio Setup
#ifdef BUILDWITHAUDIO
	// Select device
	auto devices = soundStream.getMatchingDevices("default");
	if(!devices.empty()){
		// Use default device
		audioSettings.setOutDevice(devices[0]);
	}
	else {
		// Use first device
		audioSettings.setOutDevice(soundStream.getDeviceList()[0]);
	}

	// Settings
	audioSettings.setOutListener(this);
	audioSettings.sampleRate = DEFAULT_SAMPLERATE;
	audioSettings.numOutputChannels = 2;
	audioSettings.numInputChannels = 0;
	audioSettings.bufferSize = 512;//4800;//512;//2048;//1024;//2048;//512;

	// Create
	if(!soundStream.setup(audioSettings)){
		ofLogWarning("ofApp::Setup") << "ofSoundStream setup failed !";
	}
	audioSamples.assign(audioSettings.bufferSize*audioSettings.numOutputChannels, 0.0);

	// NDI Setup
#	ifdef BUILDRECEIVER
	ndiReceiver.SetAudio(enableAudio);
	//ndiReceiver.SetMetadata(true);

	// use a 2 sec buffer
	audioBuffer.resize(4800*audioSettings.numOutputChannels*2);
#	else // BUILDRECEIVER
	ndiSender.SetAudio(enableAudio);
	ndiSender.SetAudioSampleRate(audioSettings.sampleRate);
	ndiSender.SetAudioChannels(audioSettings.numOutputChannels);
	ndiSender.SetAudioSamples(audioSettings.bufferSize);
	ndiSender.SetAudioTimecode(NDIlib_send_timecode_synthesize);
	ndiSender.SetAsync(false);
	ndiSender.SetMetadata(true);
#	endif // BUILDSENDER
#endif // BUILDWITHAUDIO

#ifdef BUILDRECEIVER

#ifdef _WIN64
	std::cout << "\nofxNDI example receiver - 64 bit" << std::endl;
#else // _WIN64
	std::cout << "\nofxNDI example receiver - 32 bit" << std::endl;
#endif // _WIN64

	std::cout << "Press 'SPACE' to list NDI senders" << std::endl;

	// ofTexture
	ndiTexture.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);

	ndiChars = new unsigned char[senderWidth*senderHeight * 4];

	// Sender dimensions and fps are not known yet
	senderWidth = (unsigned char)ofGetWidth();
	senderHeight = (unsigned char)ofGetHeight();

	//
	// ndiReceiver.SetAudio(true);
#else // BUILDRECEIVER -> SENDER

#ifdef _WIN64
	std::cout << "\nofxNDI example sender - 64 bit" << std::endl;
#else // _WIN64
	std::cout << "\nofxNDI example sender - 32 bit" << std::endl;
#endif // _WIN64

	// Create an RGBA fbo for collection of data
	m_fbo.allocate(DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_COLORFORMAT);

	// Option : set readback
	// Pixel data extraction from fbo or texture
	// is optimised using two OpenGL pixel buffers (pbo's)
	// Note that the speed can vary with different CPUs
	// Default false
	// ndiSender.SetReadback(true);

	// Option : set the framerate
	// NDI sending will clock at the set frame rate 
	// The application cycle will also be clocked at that rate
	//
	// Can be set as a whole number, e.g. 60, 30, 25 etc
	// ndiSender.SetFrameRate(30);
	//
	// Or as a decimal number e.g. 29.97
	// ndiSender.SetFrameRate(29.97);
	//
	// Or as a fraction numerator and denominator
	// as specified by the NDI SDK - e.g. 
	// NTSC 1080 : 30000, 1001 for 29.97 fps
	// NTSC  720 : 60000, 1001 for 59.94fps
	// PAL  1080 : 30000, 1200 for 25fps
	// PAL   720 : 60000, 1200 for 50fps
	// ndiSender.SetFrameRate(30000, 1001);
	//
	// Note that the NDI sender frame rate should match the render rate
	// so that it's displayed smoothly with NDI Studio Monitor.
	//
	// ndiSender.SetFrameRate(30); // Enable this line for 30 fps instead of default 60.

	// Option : set NDI asynchronous sending
	// If disabled, the render rate is clocked to the sending framerate. 
	// Note that when sending is asynchronous, frames can be sent at a higher
	// rate than the receiver can process them and hesitations may be evident.
	// ndiSender.SetAsync(true);

	ndiSender.SetClockVideo(false);
	ndiSender.SetAudioType(audio_frame_interleaved_32f_t);

	// Create a sender with RGBA output format
	bInitialized = ndiSender.CreateSender(APPTITLE, DEFAULT_WIDTH, DEFAULT_HEIGHT);//senderWidth, senderHeight);

	// A Sender with the same name cannot be created.
	// In case the executable has been renamed, and there
	// is already a sender of the same NDI name running,
	// increment the NDI name and try again.
	if (!bInitialized) {
		std::string str = "Could not create sender [";
		str += APPTITLE;	str += "]";
		printf("Could not create %s\n", str.c_str());
		// Try until 10
		for(int i=2; i<10 && !bInitialized; i++){
			bInitialized = ndiSender.CreateSender(std::string(APPTITLE).append("_").append(std::to_string(i)).c_str(), DEFAULT_WIDTH, DEFAULT_HEIGHT);
		}
		// If that still fails warn the user and quit
		if (!bInitialized) {
			ofSystemAlertDialog(str);
			exit();
		}
	}
	printf("Created sender [%s]\n", ndiSender.GetSenderName().c_str());

	// 
	// 3D drawing setup for the demo graphics
	glEnable(GL_DEPTH_TEST); // enable depth comparisons and update the depth buffer
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Really Nice Perspective Calculations
	//ofDisableAlphaBlending(); // Or we can get trails with the rotating cube

	// ofDisableArbTex is needed to create a texture with
	// normalized coordinates for bind in DrawGraphics
	ofDisableArbTex();
	textureImage.load("NDI_Box.png");

	// Back to default pixel coordinates
	ofEnableArbTex();

	// Workaround for mirrored texture with ofDrawBox and ofBoxPrimitive for Openframeworks 10.
#if OF_VERSION_MINOR >= 10
	textureImage.mirror(false, true);
#endif

	// Cube rotation
	rotX = 0;
	rotY = 0;

	// Image for pixel sending examples
	// Loads as RGB but is converted to RGBA by sending functions
	ndiImage.load("NDI_Box.png");

	// Make it the same size as the sender
	ndiImage.resize(ndiSender.GetWidth(), ndiSender.GetHeight());

	// If Wait For Vertical Sync is applied by the driver,
	// frame rate will be limited to multiples of the sync interval.
	// Disable it here and use async NDI send for best performance.
	ofSetVerticalSync(false);

	// Limit frame rate using timing instead
	ofSetFrameRate(framerate);

#endif // BUILDSENDER

	// Ableton Link Setup
#ifdef BUILDWITHABLETONLINK
	link.setup();
#endif

	ticker.setup();

	// Register tests
#ifdef BUILDRECEIVER
	appTestModes.registerTest(ofxNDItests::list_sources, "List Available Sources", [this](GuiLayoutHelper& gui){
		int nsenders = ndiReceiver.GetSenderCount();
		gui.drawStringFormated("Available sources : (%i)", nsenders);
		gui.spacing();

		gui.indent();
		if(nsenders<=0){
			gui.drawString("[ no sources available ]");
		}
		else {
			char name[256];
			for (int i = 0; i < nsenders; i++) {
				ndiReceiver.GetSenderName(name, 256, i);
				gui.drawStringFormated("%02i : %s", i, name);
			}
		}
		gui.unIndent();

		gui.spacing();
		gui.drawString(
			"Press '0-9'   to select a source.\n"
			"Press 'R'     to refresh the sources.\n"
			"Press 'SPACE' to list senders to the console."
		);

		gui.spacing();
	});
#else
	appTestModes.registerTest(ofxNDItests::list_clients, "List Clients", [this](GuiLayoutHelper& gui){
		gui.drawStringFormated(
			"Sending as        : %s\n"
			"Connected clients : %lu",
			ndiSender.GetSenderName().c_str(),
			ndiSender.GetNumClients()
		);
		gui.spacing();
	});
#endif

	appTestModes.registerTest(ofxNDItests::frame_information, "Frame Information & Settings", [this](GuiLayoutHelper& gui){
		gui.drawString(
			"This test shows as much frame information as possible."
		);
		gui.spacing();

#ifdef BUILDRECEIVER
		gui.drawStringFormated("Sender Name : %s", ndiReceiver.GetSenderName().c_str());
#else
		gui.drawStringFormated("Sender Name : %s", ndiSender.GetSenderName().c_str());
#endif
		gui.spacing();

		// OF-APP
		gui.drawString("ofApp :");
		gui.indent();
		gui.drawStringFormated("Window         : %ix%i px", ofGetWidth(), ofGetHeight());
#ifdef BUILDRECEIVER
		gui.drawStringFormated("NDI Fbo    (S) : % .0fx%.0f px", ndiTexture.getWidth(), ndiTexture.getHeight());
		gui.drawStringFormated("NDI receiver   : %ux%u px", ndiReceiver.GetSenderWidth(), ndiReceiver.GetSenderHeight());
#else
		gui.drawStringFormated("NDI sender (S) : %ix%i px", ndiSender.GetWidth(), ndiSender.GetHeight());
#endif
		// 4K (3840x2160) can help assess performance of different options.
		gui.drawStringFormated("Frame rate (G) : %.0ffps (real : % 5.2f fps, #%" PRId64 ")", ofGetTargetFrameRate(), ofGetFrameRate(), ofGetFrameNum());
		gui.unIndent();
		gui.spacing();

		// CONFIG
		gui.drawString("NDI Configuration :");
		gui.indent();
		gui.drawStringFormated("ofxNDI version    : %s", ofxNDIutils::GetVersion().c_str());
#ifdef BUILDRECEIVER
		gui.drawStringFormated("NDI version       : %s", ndiReceiver.GetNDIversion().c_str());
		gui.drawStringFormated("GetUpload (U)     : %s", ndiReceiver.GetUpload()?"yes (asynchronous)":"no");
#else
		gui.drawStringFormated("NDI version       : %s", ndiSender.GetNDIversion().c_str());
		gui.drawStringFormated("NDI name          : %s", ndiSender.GetNDIname().c_str());
		gui.drawStringFormated("Async sending (A) : %s", ndiSender.GetAsync()?"yes":"no");
		gui.drawStringFormated("Readback      (O) : %s", ndiSender.GetReadback()?"yes":"no");
		//gui.drawStringFormated("Audio     () : %s", ndiSender.GetAudio()?"yes":"no");
		gui.drawStringFormated("Progressive   (P) : %s", ndiSender.GetProgressive()?"yes":"no");
		gui.drawStringFormated("ClockVideo    (V) : %s", ndiSender.GetClockVideo()?"yes":"no");
#endif
		gui.unIndent();
		gui.spacing();

		// VIDEO
		gui.drawString("Video :");
		gui.indent();
#ifdef BUILDRECEIVER
		NDIlib_FourCC_video_type_e videoFourCC = ndiReceiver.GetVideoType();
		ofxNDIreceive::ofxNDIPerformanceMetrics metrics = ndiReceiver.GetPerformanceMetrics();
		NDIlib_recv_queue_t queues = ndiReceiver.GetQueueLengths();
#else
		NDIlib_FourCC_video_type_e videoFourCC = ndiSender.GetFormat();
#endif
		std::string videoFormat { static_cast<char>(videoFourCC & 0xFF), static_cast<char>((videoFourCC >> 8) & 0xFF), static_cast<char>((videoFourCC >> 16) & 0xFF), static_cast<char>((videoFourCC >> 24) & 0xFF) };
#ifdef BUILDRECEIVER
		gui.drawStringFormated(
			"Dimensions   : %u x %u @ % 5.2f fps\n"
			"Timecode     : %" PRId64 "\n"
			"Timestamp    : %" PRId64 "\n"
			"Video format : %s\n"
			"isVideoFrame : %s\n"
			"Performance  : %lli frames (%lli dropped)\n"
			"Queue size   : %i",
			ndiReceiver.GetSenderWidth(),
			ndiReceiver.GetSenderHeight(),
			ndiReceiver.GetSenderFps(),
			ndiReceiver.GetVideoTimecode(),
			ndiReceiver.GetVideoTimestamp(),
			videoFormat.c_str(),
			ndiReceiver.GetFrameType()&ofxNDIframeinfoflags_video?"x":" ",
			metrics.total.video_frames,
			metrics.dropped.video_frames,
			queues.audio_frames
		);
#else
		static int frameRate[2]={0,0}; static float aspectRatio = 1.f;
		ndiSender.GetAspectRatio(aspectRatio);
		ndiSender.GetFrameRate(frameRate[0], frameRate[1]);
		gui.drawStringFormated(
			"Dimensions     (S) : %u x %u (%.5f)\n"
			"Frame rate         : %05.2f fps (%i/%i)\n"
			"Video format (Y/R) : %s",
			ndiSender.GetWidth(),
			ndiSender.GetHeight(),
			aspectRatio,
			ndiSender.GetFrameRate(),
			frameRate[0], frameRate[1],
			videoFormat.c_str()
		);
#endif
		gui.unIndent();
		gui.spacing();

		gui.drawString("Audio :");
		gui.indent();
#ifdef BUILDRECEIVER
		gui.drawStringFormated(
			"Sample Rate  : %i\n"
			"Channels     : %i\n"
			"Buffer size  : %i\n"
			"Stride       : %i\n"
			"isAudioFrame : %s\n"
			"Performance  : %lli frames (%lli dropped)\n"
			"Queue size   : %i",
			ndiReceiver.GetAudioSampleRate(),
			ndiReceiver.GetAudioChannels(),
			ndiReceiver.GetAudioSamples(),
			ndiReceiver.GetAudioDataStride(),
			ndiReceiver.IsAudioFrame()?"x":" ",
			metrics.total.metadata_frames,
			metrics.dropped.metadata_frames,
			queues.metadata_frames
		);
#else
		std::string audioType = "-unknown-";
		switch(ndiSender.GetAudioType()){
			case audio_frame_v2_t:
				audioType = "v2";
				break;
			case audio_frame_interleaved_16s_t:
				audioType = "16s";
				break;
			case audio_frame_interleaved_32s:
				audioType = "32s";
				break;
			case audio_frame_interleaved_32f_t:
				audioType = "32f";
				break;
		}

		gui.drawStringFormated(
			"Enabled      : %s\n"
			"Frame type   : %i (%s / %s)\n"
			"Channels     : %i\n"
			"Sample rate  : %i hz\n"
			"Sample count : %i\n"
			"Timecode     : %" PRId64 "\n"
			"Timestamp    : %" PRId64 "\n"
			"Stride       : %i bytes",
			ndiSender.GetAudio()?"yes":"no",
			ndiSender.GetAudioType(),
			audioType.c_str(),
			(ndiSender.GetAudioType()==audio_frame_v2_t)?"planar":"interleaved",
			ndiSender.GetAudioFrame().no_channels,
			ndiSender.GetAudioFrame().sample_rate,
			ndiSender.GetAudioFrame().no_samples,
			ndiSender.GetAudioFrame().timecode,
			ndiSender.GetAudioFrame().timestamp,
			ndiSender.GetAudioFrame().channel_stride_in_bytes
		);
#endif
		gui.unIndent();
		gui.spacing();

		// METADATA
		gui.drawString("Metadata :");
		gui.indent();
#ifdef BUILDRECEIVER
		gui.drawStringFormated(
			"Last string : %s\n"
			"isMetaFrame : %s\n"
			"Performance  : %lli frames (%lli dropped)\n"
			"Queue size   : %i",
			latestMetadataString.c_str(),
			ndiReceiver.IsMetadata()?"x":" ",
			metrics.total.metadata_frames,
			metrics.dropped.metadata_frames,
			queues.metadata_frames
		);
#else
		gui.drawStringFormated(
			"A sender can also receive metadata from a receiver !\n"
			"Last received string : %s",
			latestMetadataString.c_str()
		);
#endif
		gui.unIndent();
		gui.spacing();

#ifdef BUILDRECEIVER
		gui.drawString("Other:");
		gui.indent();
		gui.drawStringFormated(
			"IsSourceChange : %s\n"
			"IsStatusChange : %s\n"
			"IsErrorFrame   : %s",
			ndiReceiver.GetFrameType() & ofxNDIframeinfoflags_sourcechange ? "x":" ",
			ndiReceiver.GetFrameType() & ofxNDIframeinfoflags_statuschange?"x":" ",
			ndiReceiver.GetFrameType() & ofxNDIframeinfoflags_error ?"x":" "
		);
		gui.unIndent();
		gui.spacing();
#endif
	});

	appTestModes.registerTest(ofxNDItests::test_audio_video_delay, "Audio & Video Receive Delay", [this](GuiLayoutHelper& gui){
#ifdef BUILDRECEIVER
		// Explanations
		if(ofGetWidth()>=900){
			gui.drawString(
				"Adjust the delay to sync the green playhead and/or the audio samples.\n"
				"When they align, you have found the approximate current delay."
			);
			gui.spacing();
		}

		gui.drawStringFormated("Manual Delay (-/+): %.3f sec (adjustable)", manualDelay);

		// Delays
		int64_t frameTime = ndiReceiver.GetVideoTimecode();
		if(frameTime>0){
			//auto ndiTime = std::chrono::duration<float>(frameTime).count();
			//auto ndiTime = std::chrono::duration<float>(std::chrono::microseconds(frameTime)).count();
			auto ndiTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::microseconds(frameTime)).count();
			//auto originalTime = std::chrono::duration<float>(link.getTime()).count();
			auto originalTime = std::chrono::duration_cast<std::chrono::milliseconds>(link.getTime()).count();
			float estimatedDelay = 0.001f*(originalTime-ndiTime);//frameTime;//ndiTime;
			gui.drawStringFormated("Estimated Delay   : %.3f sec (link_time - ndi_frame_timecode)", estimatedDelay);
		}
		else gui.drawStringFormated("NDI VideoTimecode: %lli sec", frameTime);
		gui.spacing();

		if(ofGetHeight()>=600){
			gui.drawString(
				"Some reference delays :\n"
				" - sender @30fps HD no-audio --> 0.050 to 0.80ms\n"
				" - sender @60fps HD no-audio --> 0.180ms"
			);
			gui.spacing();
		}

		if(ofGetHeight()>=500){
			gui.drawString(
				"Parameters that affect delay :\n"
				" - Image : Enabled, FPS, color format, resolution.\n"
				" - Audio : Enabled, format, interlacing.\n"
				" - MacOS : if the sender is in front / visible on screen."
			);
			gui.spacing();
		}
#else
	gui.drawString("Look at the receiver to estimate the reception delay !");
	gui.spacing();
#endif
	});

	appTestModes.registerTest(ofxNDItests::test_frame_blending, "Frame Blending", [this](GuiLayoutHelper& gui){
		gui.drawString(
			"In the sender & receiver apps, change the FPS (eg: 10,20,30,60).\n"
			"Look at the playhead to see how the received frames blend."
		);
		gui.spacing();
	});

#ifdef BUILDWITHAUDIO
	appTestModes.registerTest(ofxNDItests::test_audio_video_sync, "Audio & Video (de)Synchronisation", [this](GuiLayoutHelper& gui){
		gui.drawString(
			"Depending on audio & video sync settings in the receiver & sender,\n"
			"video and audio can slightly desynchronize."
		);
		gui.spacing();

		gui.drawString(
			"Look at the audio samples (on the timeline) and judge how they align.\n"
			"You can also check how the samples align over time (when they overwrite eachother)."
		);
		gui.spacing();

#ifdef BUILDRECEIVER
		int64_t audioTime = ndiReceiver.GetAudioTimecode();//)).count();
		int64_t videoTime = ndiReceiver.GetVideoTimecode();
#ifdef BUILDWITHABLETONLINK
		int64_t linkTime = link.getTime().count();
		gui.drawStringFormated(
			"Link  time     : %lli us\n",
			linkTime
		);
#else
		int64_t linkTime =
#endif // BUILDWITHABLETONLINK
		gui.drawStringFormated(
			"Video time     : %lli us\n"
			"Audio time     : %lli us",
			videoTime,
			audioTime
		);
		gui.spacing();

		// Timecode is not mandatory to be set !
		if(audioTime>0){
			float audioDelay = std::chrono::duration<float>(std::chrono::microseconds(linkTime-audioTime)).count();
			float videoDiff  = std::chrono::duration<float>(std::chrono::microseconds(videoTime-audioTime)).count();

			gui.drawStringFormated(
				"Audio delay    : %.3f sec (link_time  - audio_time)\n"
				"VideoAudioDiff : %.3f sec (video_time - audio_time)",
				audioDelay,
				videoDiff
			);
			gui.spacing();
		}
#endif // BUILDRECEIVER
	});
#endif // BUILDWITHAUDIO

	appTestModes.registerTest(ofxNDItests::test_video_snapshots, "Video Snapshots", [this](GuiLayoutHelper& gui){
		gui.drawString(
			"The video & time are only updated every second.\n"
			"Allowing you to compare the timestamps and calculate the delay."
		);
		gui.spacing();
	});

	appTestModes.registerTest(ofxNDItests::test_metadata, "Meta Data", [this](GuiLayoutHelper& gui){
		gui.drawString(
			"NDI also allows to send & receive metadata\n"
			"from both the receiver and sender."
		);
		gui.spacing();
		gui.drawStringFormated(
			"Is Metadata Frame              : %s",
#ifdef BUILDRECEIVER
			ndiReceiver.IsMetadata()?"x":" "
#else
			isMetadataFrame?"x":" "
#endif
		);
//#ifdef BUILDRECEIVER
		gui.drawString("Latest received metadata string:");
		gui.indent();
		if(latestMetadataString.length()>0){
			gui.drawString(latestMetadataString);
		}
		else {
			gui.drawString(" - No metadata received yet. - ");
		}
		gui.unIndent();
		gui.spacing();
//#else
		gui.drawString("Press 'TAB' to send a metadata message.");
//#endif
	});

	appTestModes.registerTest(ofxNDItests::test_metadata, "NDI Client product information", [this](GuiLayoutHelper& gui){
		gui.drawString(
			"NDI has setup a standard for sending client information trough metadata frames.\n"
			"They are optionally sent/received once when clients connect together.");
		gui.spacing();

		gui.drawString("Client information : (discovered via metadata <ndi_product>)");
		gui.indent();
		const std::list<ofxNDIproductinfo>& clients = productTracker.getClients();
		gui.spacing();
		if(clients.size()>0){
			for(const auto& client : clients){
				gui.drawStringFormated(
					" - %s by %s (version %s)\n"
					"   «%s», model=%s, session=%s, sn=%s\n",
					client.short_name.c_str(),
					client.manufacturer.c_str(),
					client.version.c_str(),
					client.long_name.c_str(),
					client.model_name.c_str(),
					client.session_name.c_str(),
					client.serial.c_str()
				);
				gui.spacing();
			}
		}
		else {
			gui.drawString(" - none - ");
		}
		gui.unIndent();
		gui.spacing();
	});

}


//--------------------------------------------------------------
void ofApp::update() {
#ifdef BUILDRECEIVER
	// Auto-refresh sources list
	const float now = ofGetElapsedTimef();
	if(ndiReceiver.ReceiverCreated() && latestSourcesRefresh < now+10.f){ // Every 10 sec
		ndiReceiver.FindSenders();
		latestSourcesRefresh = now;
	}
#endif
}

//--------------------------------------------------------------
void ofApp::draw() {

	GuiLayoutHelper gui(20, 20);

#ifdef BUILDRECEIVER

	bool tryReceive = true;
	static float snapShotNow = -1.f;

	// Update ticker
	float now = getNowTime();
	// Snapshot test mode behaviour
	if(appTestModes.getCurrentMode()==ofxNDItests::test_video_snapshots){
		tryReceive = false;
		// Update ? / new snapshot
		if(glm::floor(snapShotNow)<glm::floor(now)){
			tryReceive = true;
		}
		// Use cache
		else now = snapShotNow;
	}
	ticker.updateData(now);

	// Receive ofTexture
	if(tryReceive){
		if(ndiReceiver.ReceiveImage(ndiTexture)){
			snapShotNow = now;
		}
		//std::cout << "NDi frame: " << FrameInfoToString(ndiReceiver.GetFrameType()) << std::endl;
	}
	else {
		// dummy receive (otherwise NDI stops receiving frames & becomes "unstable")
		static ofPixels dummyPixels;
		if(!dummyPixels.isAllocated()) dummyPixels.allocate(ndiReceiver.GetSenderWidth(), ndiReceiver.GetSenderHeight(), OF_IMAGE_COLOR_ALPHA);
		ndiReceiver.ReceiveImage(dummyPixels);
	}
	// Receive Audio frames
	audioBuffer.writeSamplesToBuffer(ndiReceiver);

	// Receive metadata (sync with sender tester example)
	if(ndiReceiver.IsMetadata()){
		std::string metadata = ndiReceiver.GetMetadataString();
		// Parse remote appmode ?
		ofxNDItests nextMode = ParseAppModeFromMetadata(metadata);
		if(nextMode!=ofxNDItests_max){
			appTestModes.setCurrentMode(nextMode);
		}
		// Parse productinfo ?
		else productTracker.addProductFromMetadata(metadata.c_str());
		// Save latest
		latestMetadataString = metadata;
	}

	ndiTexture.draw(0, 0, ofGetWidth(), ofGetHeight());

#else

	// Check success of CreateSender
	if (!bInitialized)
		return;

	// Check for metadata message
	std::string receivedMetadata = ndiSender.ReceiveMetadataString();
	isMetadataFrame = receivedMetadata.length()>0;
	if(isMetadataFrame){
		latestMetadataString = receivedMetadata;
		//std::cout << "Metadata Received ! --> " << receivedMetadata << std::endl;

		// Parse remote appmode ?
		ofxNDItests nextMode = ParseAppModeFromMetadata(receivedMetadata);
		if(nextMode!=ofxNDItests_max){
			appTestModes.setCurrentMode(nextMode);
		}
		// Parse productinfo ?
		else productTracker.addProductFromMetadata(receivedMetadata.c_str());
	}

#endif

	// Send metadata ? (required to be called from draw()?)
	if(bSendMetaData){
#ifdef BUILDRECEIVER
		// Hacky : we use the "connection" metadata to send metadata (only way?)
		ndiReceiver.ClearConnectionMetadataStrings();
		ndiReceiver.AddConnectionMetadataString("<text>Hello from ofxNDI receiver !</text>");
#else
		ndiSender.SetMetadataString("<text>Hello from ofxNDI sender !</text>");
#endif
		//std::cout << "Metadata sent !" << std::endl;
		bSendMetaData = false;
	}

	// Render graphics layer
	DrawGraphics();

	// Send ofFbo
#ifndef BUILDRECEIVER
#ifdef BUILDWITHABLETONLINK
	// Set frame time so we can calc the delay ?
	//ndiSender.SetVideoTimecode(std::chrono::duration_cast<std::chrono::microseconds>(link.getTime()).count());
	ndiSender.SetVideoTimecode(link.getTime().count());
#endif
	ndiSender.SendImage(m_fbo);
#endif

	// Show info layer
	ShowInfo();

}

void ofApp::ShowInfo() {

	std::string str;
	GuiLayoutHelper gui(20, 20);

#ifdef BUILDRECEIVER

	int nsenders = ndiReceiver.GetSenderCount();
	if (nsenders > 0) {
		if (ndiReceiver.ReceiverCreated()) {
			if (ndiReceiver.ReceiverConnected()) {
				gui.drawStringFormated("Source (0-9) : %s", ndiReceiver.GetSenderName().c_str());
				gui.drawStringFormated(
					"Video : %u x %u @ % 5.2f fps (Receiving @ % 5.2f fps)",
					ndiReceiver.GetSenderWidth(),
					ndiReceiver.GetSenderHeight(),
					ndiReceiver.GetSenderFps(),
					ndiReceiver.GetFps()
				);

#ifdef BUILDWITHAUDIO
				// Audio
				if(enableAudio){
					gui.drawStringFormated(
						"Audio : %ihz, %i channels, buffer=%i (disable with N)",
						ndiReceiver.GetAudioSampleRate(),
						ndiReceiver.GetAudioChannels(),
						ndiReceiver.GetAudioSamples()
					);
				}
				else {
					gui.drawString("Audio : Disabled (enable with N)");
				}
#endif

				// Testers
				gui.separator();
				gui.drawStringFormated("Tester   ([/]) : %s", appTestModes.getCurrentTitle().c_str());
				gui.reserveSpace(FONT_SIZE);
				appTestModes.render(gui);
			}
		}
	}
	else {
		gui.drawString("Connecting . . .");
	}
#else
	if (ndiSender.SenderCreated()) {
		
		gui.drawStringFormated(
			"Sending as   : %s\n"
			"Video format : %ix%i @ % 5.2f fps",
			ndiSender.GetSenderName().c_str(),
			ndiSender.GetWidth(),
			ndiSender.GetHeight(),
			ofGetTargetFrameRate()
		);
#ifdef BUILDWITHAUDIO
		gui.drawStringFormated(
			"Audio        : %i ch @ %ihz (buffer: %i bytes)",
			ndiSender.GetAudioFrame().no_channels,
			ndiSender.GetAudioFrame().sample_rate,
			ndiSender.GetAudioFrame().no_samples
		);
#endif

		// Testers
		gui.separator();
		gui.drawStringFormated("Tester   ([/]) : %s", appTestModes.getCurrentTitle().c_str());
		gui.reserveSpace(FONT_SIZE);
		appTestModes.render(gui);
	}
#endif

	// TODO: use guilayout instead of str
#ifdef BUILDWITHAUDIO
	// Audio controls
	int width = glm::max(240, ofGetWidth()/3);
	ofRectangle audioZone(ofGetWidth()-20-width, 60, width, 50);

	str = "Audio Controls";
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);

	str = " Volume  (""M"") : ";
	str += std::to_string((int)audioVolume);
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);

#ifndef BUILDRECEIVER
	str = " Balance     (""B"") : ";
	str += std::to_string((int)audioBalance);
	if(audioBalance <= -1) str += " (left)";
	else if(audioBalance >=  1) str += " (right)";
	else if(audioBalance == 0) str += " (center)";
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);

	str = " Frequency (""+""""/""""-"") : ";
	str += std::to_string((int)audioFreq);
	str += "hz";
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);
#endif

	audioZone.translateY(5);

#ifdef BUILDRECEIVER
	str = "Incoming Audio   NDI / OpenFrameworks";
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);

	str = " Samplerate    : ";
	str += std::to_string(ndiReceiver.GetAudioSampleRate());
	str += " / ";
	str += std::to_string(soundStream.getSampleRate());
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);

	str = " Channels      : ";
	str += std::to_string(ndiReceiver.GetAudioChannels());
	str += " / ";
	str += std::to_string(soundStream.getNumOutputChannels());
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);

	str = " BufferSize    : ";
	str += std::to_string(ndiReceiver.GetAudioSamples());
	str += " / ";
	str += std::to_string(soundStream.getBufferSize());
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);

	str = " BufferLoad    : ";
	str += std::to_string((int)glm::round(100*audioBuffer.getBufferLoad()));
	str += "%";
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	audioZone.translateY(FONT_SIZE);

	const float timeSinceLastData = glm::clamp((getNowTime()-lastAudioFrameTime)*(2.f), 0.f, 1.f);
	str = " Incoming Data : ";
	ofDrawBitmapString(str, audioZone.x, audioZone.y);
	ofPushStyle();
	ofFill();
	ofSetColor(255-255*timeSinceLastData);
	ofDrawCircle(glm::vec2(audioZone.x+str.length()*8+6, audioZone.y-4), 6);
	ofPopStyle();
	audioZone.translateY(FONT_SIZE+5);
#endif // !BUILDRECEIVER

	// Draw audio signals
	for(std::size_t ch=0; ch<audioSettings.numOutputChannels; ch++){
		ofPushStyle();
		ofNoFill();

		ofSetColor(225);
		string chName = "Channel #";
		chName += std::to_string(ch);
		ofDrawBitmapString(chName, audioZone.x, audioZone.y);
		audioZone.translateY(5);

		ofSetLineWidth(1);
		ofDrawRectangle(audioZone);

		ofSetColor(AUDIO_COLOR);
		ofSetLineWidth(2);

		ofBeginShape();
		for (std::size_t i = 0; i < audioSettings.bufferSize; i++){
			float x =  ofMap(i, 0, audioSettings.bufferSize, 0, audioZone.width, true);
			ofVertex(audioZone.x+x, audioZone.y + audioZone.height*.5 + audioSamples[i*audioSettings.numOutputChannels+ch]*-1.f*audioZone.height*.5);
		}
		ofEndShape(false);

		ofPopStyle();

		audioZone.translateY(audioZone.height+FONT_SIZE);
	}
#endif // BUILDWITHAUDIO
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

#ifdef BUILDRECEIVER
	char name[256];
	int index = key - 48;
	int nsenders = ndiReceiver.GetSenderCount();

	// Use keynum to switch sources
	if (nsenders > 0 && index >= 0 && index < nsenders) {
		// Update the receiver with the returned index
		// Returns false if the current sender is selected
		if (ndiReceiver.SetSenderIndex(index)){
			std::cout << "Selected [" << ndiReceiver.GetSenderName(index) << "]" << std::endl;
			// reset metadatastring (not to confuse sources)
			latestMetadataString = "";
		}
		else
			std::cout << "Same sender" << std::endl;
	}
	else
#else
	framerate = ndiSender.GetFrameRate(); // update global fps value
#endif

	switch (key) {
		// AppModes
		case '{':
		case '}':
		case '[':
		case ']':
			{
				int dir = (key=='{'||key=='[') ? -1 : 1;
				appTestModes.incrementMode(dir);

				// Send mode switch
				std::string message = std::string("<appmode>") + APPMODESTR + std::to_string((int)appTestModes.getCurrentMode())+"</appmode>";
#ifndef BUILDRECEIVER
				ndiSender.SetMetadataString(message);
#else
				// Hacky !
				ndiReceiver.ClearConnectionMetadataStrings();
				ndiReceiver.AddConnectionMetadataString(message);
#endif
			}
			break;
		// Metadata sending
		case '\t':
			bSendMetaData = true;
			break;

		// Audio Controls
#ifdef BUILDWITHAUDIO
		// Volume
		case 'm':
		case 'M':
			audioVolume = (audioVolume==0) ? 1.f : 0.f;
			break;
		// Enable/Disable
		case 'n':
		case 'N':
			enableAudio = !enableAudio;
#	ifdef BUILDRECEIVER
			ndiReceiver.SetAudio(enableAudio);
#	else
			ndiSender.SetAudio(enableAudio);
#	endif
			break;

#	ifdef BUILDRECEIVER

#	else // SENDER AUDIO ACTIONS
		// Balance
		case 'b':
		case 'B':
			if(audioBalance==0){
				audioBalance = 1.f;
			}
			else if(audioBalance == 1){
				audioBalance = -1.f;
			}
			else audioBalance = 0.f;
			break;
		// Freq
		case '+':
		case '-':
			audioFreq = glm::clamp(audioFreq + (key=='+'?100:-100), 100u, 2000u);
			break;
		// Audio Clocking
		case 'c':
		case 'C':
			bClockedAudio = !bClockedAudio;
			ndiSender.SetClockAudio(bClockedAudio);
			break;
#	endif

#endif // BUILDWITHAUDIO

		// RECEIVER ACTIONS
#ifdef BUILDRECEIVER
		// List senders
		case ' ':
			// List all the senders
			{
				if (nsenders > 0) {
					std::cout << "Number of NDI senders found: " << nsenders << std::endl;
					for (int i = 0; i < nsenders; i++) {
						ndiReceiver.GetSenderName(name, 256, i);
						std::cout << "    Sender " << i << " [" << name << "]" << std::endl;
					}
					if (nsenders > 1)
						std::cout << "Press key [0] to [" << nsenders - 1 << "] to select a sender" << std::endl;
				}
				else
					std::cout << "No NDI senders found" << std::endl;
			}
			break;
		// Delay adjust
		case '+':
		case '-':
			manualDelay = glm::max(0.f, manualDelay + (key=='+'?0.001f:-0.001f)*(ofGetKeyPressed(OF_KEY_SHIFT)?10.f:1.f));
			break;
		// Refresh
		case 'R':
		case 'r':
			ndiReceiver.FindSenders();
			break;
		case 'U':
		case 'u':
			ndiReceiver.SetUpload(!ndiReceiver.GetUpload());

#else	// SENDER ACTIONS
		case 'f':
		case 'F':
			{
				std::string str;
				double fps = framerate; // for entry
				str = std::to_string(framerate);
				size_t s = str.rfind(".");
				str = str.substr(0, s + 3);
				str = ofSystemTextBoxDialog("Frame rate", str);
				if (!str.empty()) {
					fps = stod(str);
					if (fps <= 60.0 && fps >= 10.0) {
						framerate = fps;
						ndiSender.SetFrameRate(fps);
					}
				}
			}
			break;

		case 'a':
		case 'A':
			ndiSender.SetAsync(!ndiSender.GetAsync());
			break;

		case 'o':
		case 'O':
			ndiSender.SetReadback(!ndiSender.GetReadback());
			break;

		case 'p':
		case 'P':
			ndiSender.SetProgressive(!ndiSender.GetProgressive());
			break;

		case 'y':
		case 'Y':
			ndiSender.SetFormat(NDIlib_FourCC_video_type_UYVY);
			break;

		case 'r':
		case 'R':
			ndiSender.SetFormat(NDIlib_FourCC_video_type_RGBA);
			break;

		case 's':
		case 'S':
			{
				int senderWidth = ndiSender.GetWidth();
				int senderHeight = ndiSender.GetHeight();
				int width = 0, height = 0;
				std::string str;
				str = ofSystemTextBoxDialog("Image size", std::to_string(senderWidth) + "x" + std::to_string(senderHeight));
				if (!str.empty()) {
					width = height = 0;
					if (!str.empty()) {
						std::size_t pos = str.find("x");
						std::string w = str.substr(0, pos);
						width = stoi(w);
						std::string h = str.substr(pos + 1, str.length());
						height = stoi(h);
						if (width > 99 && width < 4100 && height > 100 && height <= 4100) {
							std::string name = ndiSender.GetSenderName();
							senderWidth = (unsigned int)width;
							senderHeight = (unsigned int)height;
							// Resize the drawing fbo to the same size as the sender
							m_fbo.allocate(senderWidth, senderHeight, GL_RGBA);
							// And the demo image
							ndiImage.resize(senderWidth, senderHeight);
							// Adapt NDI sender to the size change
							if (ndiSender.SenderCreated())
								ndiSender.UpdateSender(senderWidth, senderHeight);
						}
					}
				}
			}
			break;
		case 'v':
		case 'V':
			ndiSender.SetClockVideo(!ndiSender.GetClockVideo());
			break;
#endif
	}// end switch


	// Show the main window
#ifdef TARGET_WIN32
	BringWindowToTop(ofGetWin32Window());
#endif

}


void ofApp::DrawGraphics() {
#ifndef BUILDRECEIVER

	int senderWidth = ndiSender.GetWidth();
	int senderHeight = ndiSender.GetHeight();

	// Update & Prepare ticker data
	ticker.updateData(getNowTime());
	//int hPos = senderHeight*TICKER_POS;
	GuiLayoutHelper layout (20, senderHeight*TICKER_POS);
	ofRectangle ticksRect(20*2, layout.curPos.y, senderWidth-20*2*2, FONT_SIZE*3);
	ticker.drawHighestAudioSample(highestFrameVolume, ticksRect, AUDIO_COLOR);
	highestFrameVolume = 0.f; // reset

	// Draw graphics into an fbo used for the examples
	m_fbo.begin();

	// Rotating cube
	ofEnableDepthTest();
	ofClear(13, 25, 76, 255);
	ofPushMatrix();
	ofTranslate((float)senderWidth / 2.0, (float)senderHeight / 2.0, 0);
	ofRotateYDeg(rotX);
	ofRotateXDeg(rotY);
	textureImage.getTexture().bind();
	ofDrawBox(0.4 * (float)senderHeight);
	ofPopMatrix();
	ofDisableDepthTest();

	// Timer overlay
	ofPushStyle();
	ofPushMatrix();

	// Timer text
	ticker.drawTime(layout.curPos, true);
	layout.reserveSpace((FONT_SIZE*3+8)*2);


	// Ticker
	ticksRect.setPosition(layout.curPos.x, layout.curPos.y);
	ticker.drawTickerTimeline(ticksRect, true, true, true, true, ofColor::black);

	ofPopMatrix();
	ofPopStyle();

	m_fbo.end();

	// Rotate the cube
	rotX += 0.75;
	rotY += 0.75;

	// Draw the fbo result fitted to the display window
	m_fbo.draw(0, 0, ofGetWidth(), ofGetHeight());
#else

	ofPushStyle();
	ofPushMatrix();
	glm::vec2 scale = {1.f, 1.f };
	if(ndiReceiver.GetSenderHeight()>0 && ndiReceiver.GetSenderWidth()>0){
		scale.x = ((float)ofGetWidth())/ndiReceiver.GetSenderWidth();
		scale.y = ((float)ofGetHeight())/ndiReceiver.GetSenderHeight();
	}
	ofScale(scale.x, scale.y);

	//int hPos = ndiReceiver.GetSenderHeight()*TICKER_POS;
	GuiLayoutHelper layout (20, ndiReceiver.GetSenderHeight()*TICKER_POS);
#ifdef BUILDWITHABLETONLINK
	layout.curPos.y -= (FONT_SIZE*3+8);
	ofDrawBitmapStringHighlight("AbletonLink Time", 20+FONT_SIZE*8*2.5f, layout.curPos.y-FONT_SIZE*3-10);
	ticker.drawTime(layout.curPos, false);
	layout.curPos.y += (FONT_SIZE*3+8);
#endif
	layout.curPos.y+=(FONT_SIZE*3+8)*2; // Skip sender timer zone

	if(ndiReceiver.ReceiverConnected()){
		ofRectangle ticksRect(20*2, layout.curPos.y, (ndiReceiver.GetSenderWidth()-20*2*2), FONT_SIZE*3); // Like sender
		ticker.drawHighestAudioSample(highestFrameVolume, ticksRect, ofColor::green);
		highestFrameVolume = 0.f; // reset
		if(
			appTestModes.getCurrentMode()==ofxNDItests::test_audio_video_delay ||
			appTestModes.getCurrentMode()==ofxNDItests::test_audio_video_sync ||
			appTestModes.getCurrentMode()==ofxNDItests::test_frame_blending
		){
			ticker.drawTickerTimeline(ticksRect, false, false, true, appTestModes.getCurrentMode()==ofxNDItests::test_audio_video_delay, ofColor::green);
		}
		layout.curPos.y += ticksRect.height + FONT_SIZE*3;

		// Audio samples legend
		if(
			appTestModes.getCurrentMode()==ofxNDItests::test_audio_video_delay ||
			appTestModes.getCurrentMode()==ofxNDItests::test_audio_video_sync
		){
			ofSetColor(ofColor::green);
			std::string str;
			str = "Green dots   = received audio max samples. Green playhead = AbletonLink-synced playhead";
			layout.drawString(str);
			ofSetColor(AUDIO_COLOR);
			str = "Magenta dots = emitted audio max samples.  Black playhead = Received NDI video.";
			layout.drawString(str);
		}
	}

	ofPopMatrix();

	ofPopStyle();
#endif
}

//--------------------------------------------------------------
void ofApp::exit() {
#ifdef BUILDRECEIVER
	ndiReceiver.ReleaseReceiver();
#else
	// The sender must be released 
	// or NDI sender discovery will still find it
	ndiSender.ReleaseSender();
#endif

#ifdef BUILDWITHAUDIO
	soundStream.close();
#endif
}

//--------------------------------------------------------------
float ofApp::getNowTime() const {
#ifndef BUILDRECEIVER
	constexpr const float manualDelay = 0.f; // Dummy variable
#endif

#ifdef BUILDWITHABLETONLINK
	if(link.isEnabled()){
		return std::chrono::duration<float>(link.getTime()).count()-manualDelay;
	}
#endif
	return ofGetElapsedTimef()-manualDelay;
}

//--------------------------------------------------------------
#ifdef BUILDWITHAUDIO
void ofApp::audioOut(ofSoundBuffer & buffer){
	if(enableAudio){

		const float numChannels = buffer.getNumChannels();
		const float numFrames = buffer.getNumFrames();

#ifndef BUILDRECEIVER
		float highest = 1.f;
		unsigned int mFreq = audioFreq;
		const Ticker::MarkerData* curMarker = nullptr;
		if(true){ // todo: make optional
			ticker.updateData(getNowTime());
			highest = 0.f;
			for(const auto& m : ticker.getMarkers()){
				if(m.amplitude>highest || m.getAmplitude(ticker.getLoopTime()+(numFrames/static_cast<float>(audioSettings.sampleRate)))){
					highest = m.amplitude;
					mFreq = m.freq;
					curMarker = &m;
				}
			}
		}

		// Synthetise a sinewave
		const float phaseAdder = ((glm::two_pi<float>() * mFreq) / static_cast<float>(audioSettings.sampleRate));
		static float phase = 0;

		float mult = ofGetMouseY()/ofGetHeight(); mult=0.99f;
		for (size_t i = 0; i < numFrames; i++){
			const float sample = sin(phase);

			float noteVolume = 1.f;
			if(true){
				noteVolume = 0.f;
				if(highest>0 && curMarker){
					noteVolume = (curMarker->getAmplitude(ticker.getLoopTime()+((1.f/ticker.loopSeconds)/static_cast<float>(audioSettings.sampleRate)*i)));
				}
			}
			for(size_t ch = 0; ch<numChannels; ch++){
				float channelBalanceMult = audioBalance==0.f;
				if(ch==0) channelBalanceMult = glm::clamp(1.f-audioBalance, 0.f, 1.f);
				else if(ch==1) channelBalanceMult = glm::clamp(1.f+audioBalance, 0.f, 1.f);
				audioSamples[i*numChannels+ch] = sample * channelBalanceMult * noteVolume;
				buffer[i*numChannels+ch] = sample * channelBalanceMult * audioVolume * noteVolume;
			}
			if(glm::abs(audioSamples[i*numChannels])>highestFrameVolume) highestFrameVolume = glm::abs(audioSamples[i*numChannels]);
			phase += phaseAdder;
		}
		phase = glm::mod(phase, glm::two_pi<float>());

		// Send audio frames to NDI
#	ifdef BUILDWITHABLETONLINK
		ndiSender.SetAudioTimecode(link.getTime().count()); // NDIlib_send_timecode_synthesize
#	endif
		ndiSender.SetAudioData(&audioSamples[0]);
		ndiSender.SendAudio();
#else
		// Receive audio frames and play them
//		audioBuffer.writeSamplesToBuffer(ndiReceiver);
		audioBuffer.readSamplesFromBuffer(buffer);
		return;

		float* sound = ndiReceiver.GetAudioData();
		if(sound!=nullptr){
			int numFrames = ndiReceiver.GetAudioSamples();
			int numChannels = ndiReceiver.GetAudioChannels();
			int sampleRate = ndiReceiver.GetAudioSampleRate();
			int nStride = ndiReceiver.GetAudioDataStride();
			lastAudioFrameTime = getNowTime();

			// Does the buffer match ?
			// (logging and allocating is not recomended from the audio process!!!)
//			if(buffer.getSampleRate() != sampleRate || buffer.getNumChannels() != numChannels || buffer.getNumFrames() != numFrames){
//				ofLogNotice("ofApp::audioOut") << "Buffer size don't match : Resizing ! sr=" << buffer.getSampleRate() << "/" << sampleRate << " frame=" << buffer.getNumFrames() << "/" << numFrames << " ch=" << buffer.getNumChannels() << "/" << numChannels;
////				audioSettings.numOutputChannels = numChannels;
////				audioSettings.sampleRate = sampleRate;
////				audioSettings.bufferSize = numFrames;
////				soundStream.setup(audioSettings);
////				return;
//			}

			// power of 2 for soundstream
			int bufferSize = std::pow(2.0, std::ceil(std::log2(numFrames)));
			if(audioSamples.size() != bufferSize){
				audioSamples.resize(bufferSize, 0);
			}

			// Noworks...
			//ndiReceiver.GetAudioData(*const_cast<float*>(buffer.getBuffer().data()), sampleRate, numFrames, numChannels);

			// Note: Sometimes, buffer size don't match !
			// ("NDI Test Patterns.app" sends 4800 samples, then OF sets it to 4096)
			for (std::size_t i = 0; i < numFrames; i++){
				for(std::size_t ch = 0; ch<numChannels; ch++){
					const std::size_t indexInterleaved = i*numChannels+ch; // Interleaved
					const std::size_t indexNormal = i+ch*numFrames; // non-interleaved

					audioSamples[indexInterleaved] = sound[indexNormal];
					if(glm::abs(audioSamples[indexInterleaved])>highestFrameVolume) highestFrameVolume = glm::abs(audioSamples[indexInterleaved]);
					if(indexInterleaved < buffer.size()) // Drops frames = crackling
						buffer[indexInterleaved] = sound[indexNormal]*audioVolume;
				}
			}
		}
#endif
	}
}
#endif // BUILDWITHAUDIO
