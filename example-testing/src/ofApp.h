/*

	OpenFrameworks NDI tester sender/receiver example

	#define BUILDRECEIVER for conditional build

	Pair of sender and receiver apps which help debugging and introspecting NDI frame transmission.

	Copyright (C) 2016-2026 Lynn Jarvis.
	http://www.spout.zeal.co
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
*/
#pragma once

// Enable this define to buid a receiver rather that a sender
 #define BUILDRECEIVER

// Enable this define to also send/receive audio frames
 #define BUILDWITHAUDIO

 // Enable this to include abletonlink tests (https://github.com/2bbb/ofxAbletonLink)
 #define BUILDWITHABLETONLINK

#define TICKER_POS 0.75f
#define FONT_SIZE 16
//#define APP_TEST_MODES_MAX 5 // count+1
#define APPMODESTR "TestAppMode="

#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080
#define DEFAULT_COLORFORMAT GL_RGBA
#define DEFAULT_SAMPLERATE 48000
//#define DEFAULT_SAMPLERATE 44100

#include "ofMain.h"
#include "ofxNDI.h" // NDI classes
#include "ticker.h"
#include "GuiLayoutHelper.h"
#include "TestModeHelper.h"

#ifdef BUILDWITHABLETONLINK
#include "ofxAbletonLink.h"
#include <chrono>
#endif

#ifdef BUILDWITHAUDIO
#include "ofxNDIaudiobuffer.h"
#	ifdef BUILDRECEIVER
#		define AUDIODEFAULTMUTE true
#	else
#		define AUDIODEFAULTMUTE false
#	endif
#define AUDIO_COLOR ofColor(245, 58, 135)
#endif

#ifdef BUILDRECEIVER
#   define APPTITLE "Openframeworks NDI receiver"
#else
#   define APPTITLE "Openframeworks NDI Sender"
#endif

enum ofxNDItests : size_t {
#ifdef BUILDRECEIVER
    list_sources = 0,
#else
    list_clients = 0,
#endif
    frame_information,
    test_audio_video_delay,
    test_frame_blending,
    test_audio_video_sync,
    test_video_snapshots,
    test_metadata,
    test_audio,
    test_product_info,
    ofxNDItests_max
};

#include "pugixml.hpp" // Shipped within OF :)
struct ofxNDIproductinfo {
    std::string long_name;
    std::string short_name;
    std::string manufacturer;
    std::string model_name;
    std::string version;
    std::string serial;
    std::string session_name;

    bool isValid() const {
        return !long_name.empty() || !short_name.empty() || !manufacturer.empty();
    }

    // Helper for checking validity
    operator bool() const {
        return isValid();
    }

    // For comparing product info
    bool operator==(const ofxNDIproductinfo& other) const {
        return long_name == other.long_name &&
               manufacturer == other.manufacturer &&
               version == other.version;
    }

    // Parses string to product info, if valid
    static ofxNDIproductinfo FromMetadata(const char* str){
        ofxNDIproductinfo productInfo;
        if(!str) return productInfo;

        // Parse XML
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(str);
        if (!result) {
            return productInfo;
        }

        // Traverse
        pugi::xml_node product_node = doc.child("ndi_product");
        if (product_node) {
            productInfo.long_name = product_node.attribute("long_name").as_string();
            productInfo.short_name = product_node.attribute("short_name").as_string();
            productInfo.manufacturer = product_node.attribute("manufacturer").as_string();
            productInfo.model_name = product_node.attribute("model_name").as_string();
            productInfo.version = product_node.attribute("version").as_string();
            productInfo.serial = product_node.attribute("serial").as_string();
            productInfo.session_name = product_node.attribute("session_name").as_string();
        }

        return productInfo;
    }
};

class ofxNDIproducttracker {
    public:
        const std::list<ofxNDIproductinfo>& getClients(){
            return clients;
        }
        void addProductFromMetadata(const char* str){
            if(!str) return;
            ofxNDIproductinfo p = ofxNDIproductinfo::FromMetadata(str);
            addProduct(p);
        }

        void addProduct(const ofxNDIproductinfo& p){
            if(p.isValid()){
                // Already listed ?
                for(const auto& pi : clients){
                    if(pi==p){
                        return;
                    }
                }

                // Add
                clients.push_back(p);
            }
        }
        std::list<ofxNDIproductinfo> clients;
};

class ofApp : public ofBaseApp{

	public:

		void setup();
		void update();
		void draw();
		void exit();
		void keyPressed(int key);
		void ShowInfo();
		float getNowTime() const;
		void DrawGraphics();

#ifdef BUILDRECEIVER
		ofxNDIreceiver ndiReceiver; // NDI receiver
		ofFbo ndiFbo; // Fbo to receive
		ofTexture ndiTexture; // Texture to receive
		//ofImage ndiImage; // Image to receive
		//ofPixels ndiPixels; // Pixels to receive
		unsigned char *ndiChars; // unsigned char image array to receive
		unsigned int senderWidth = 0; // sender width and height needed to receive char pixels
		unsigned int senderHeight = 0;
		float latestSourcesRefresh = 0;
#else
		ofxNDIsender ndiSender;        // NDI sender
//		std::string senderName;        // Sender name
//		unsigned int senderWidth = 0;  // Width of the sender output
//		unsigned int senderHeight = 0; // Height of the sender output
		bool bInitialized = false;
		ofFbo m_fbo;                   // Fbo used for graphics and sending
		ofImage textureImage;          // Texture image for the 3D cube graphics
		float rotX = 0.0f;
		float rotY = 0.0f;             // Cube rotation increment
		ofImage ndiImage;              // Test image for sending

//		bool bReadback = true;
//		bool bAsync = true;
		bool bClockedAudio = false;
		double framerate = 60.0;
		bool isMetadataFrame = false;
#endif
//		int appTestMode = 0;
		Ticker ticker;
		ofxNDIproducttracker productTracker;
		std::atomic<float> highestFrameVolume = 0.f;
		TestModeHelper<ofxNDItests> appTestModes;
		std::string latestMetadataString = "";
		bool bSendMetaData = false;


#ifdef BUILDWITHAUDIO
		ofSoundStreamSettings audioSettings;
		ofSoundStream soundStream;
		virtual void audioOut(ofSoundBuffer &buffer);
		vector <float> audioSamples;
		bool enableAudio = true;
		float audioVolume = 1.f * AUDIODEFAULTMUTE; // openframeworks playback only !
		ofxNDIaudiobuffer audioBuffer;
		bool isAudioFrame = false;

#ifdef BUILDRECEIVER
		float lastAudioFrameTime = 0.f;
		float manualDelay = 0.f;
#else
		float audioBalance = 0; // -1 to 1
		unsigned int audioFreq = 800;
#endif // BUILDRECEIVER

#endif // BUILDWITHAUDIO

#ifdef BUILDWITHABLETONLINK
		ofxAbletonLink link;
#endif
};
