/*
	NDI Receive

	using the NDI SDK to receive frames from the network

	https://ndi.video

	Copyright (C) 2016-2026 Lynn Jarvis.

	http://www.spout.zeal.co

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

	Function additions see ofxNDIreceive.cpp

	16.10.16 - common buffer copy utilities
	09.02.17 - Changes for NDI SDK Version 2
				ReceiveImage - remove swap RG flag - now done internally
				RefreshSenders - uint32_t timeout instead of DWORD
				Remove timer variables - timeouts done in new SDK functions
				FindGetSources - replacement for deprecated NDIlib_find_get_sources
			 - include changes by Harvey Buchan
				CreateReceiver - include colour format option
			 - SetLowBandWidth, Metadata
	17.02.17 - GetNDIversion - NDIlib_version
	31.03.18 - Update to NDI SDK Version 3
	11.06.18 - Update to NDI SDK Version 3.5
			 - Header function comments expanded so that they are visible to the user
			 - Add changes for OSX (https://github.com/ThomasLengeling/ofxNDI)
	11.07.18 - Change class name to ofxReceive
			   Class can be used independently of Openframeworks
	08.11.19 - Arch Linux x64 compatibility
			   https://github.com/hugoaboud/ofxNDI
			   To be tested
	15.11.19 - Change to dynamic load of Newtek NDI dlls
	19.11.19 - Add conditional audio receive
	06.12.19 - Add dynamic load class (https://github.com/IDArnhem/ofxNDI)
	27.02.20 - Add std::chrono functions for fps timing
	14.12.23 - Add m_VideoTimecode, GetVideoTimecode()
	19.01.25 - Update to NDI 6.1.1.0
	21.12.25 - Update to NDI version 6.2.1.0
	11.02.25 - Remove unused NDI_send_create_desc

*/
#pragma once
#ifndef __ofxNDIreceive__
#define __ofxNDIreceive__

#include <string>
#include <iostream>
#include <vector>
#include <assert.h>

#include "ofxNDIdynloader.h" // NDI library loader
#include "ofxNDIutils.h" // buffer copy utilities

#if defined(TARGET_WIN32)
#include <windows.h>
#include <intrin.h> // for _movsd
#include <math.h> //
#include <gl\GL.h>
#include <mmsystem.h> // for timegettime if ofMain is included
#include <Shellapi.h> // for shellexecute
#pragma comment(lib, "Winmm.lib") // for timegettime
#pragma comment(lib, "Shell32.lib")  // for shellexecute
#elif defined(__APPLE__)
#if not defined(__aarch64__)
#include <x86intrin.h> // for _movsd
#endif
#include <sys/time.h>
#else // Linux
#include <sys/time.h>
#include <cstring>
#include <cmath>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#endif

// Linux
// https://github.com/hugoaboud/ofxNDI
#if !defined(TARGET_WIN32)
typedef struct {
	long long QuadPart;
} LARGE_INTEGER;
typedef unsigned int DWORD;
#endif

typedef int ofxNDIframeinfoflags;
enum ofxNDIframeinfoflags_ {
	ofxNDIframeinfoflags_none			= 0,
	ofxNDIframeinfoflags_video			= 1 << 2,
	ofxNDIframeinfoflags_audio			= 1 << 3,
	ofxNDIframeinfoflags_metadata		= 1 << 4,
	ofxNDIframeinfoflags_statuschange	= 1 << 5,
	ofxNDIframeinfoflags_sourcechange	= 1 << 6,
	ofxNDIframeinfoflags_error	        = 1 << 7,
	//ofxNDIframeinfoflags_nodata			= 1 << 0,
};
ofxNDIframeinfoflags_ ToFrameInfoFlag(NDIlib_frame_type_e frametype);

std::string FrameInfoToString(const ofxNDIframeinfoflags& fif);

class ofxNDIreceive {

public:

	ofxNDIreceive();
	~ofxNDIreceive();

	// Open a receiver ready to receive
	bool OpenReceiver();

	// Create a receiver
	// Default format RGBA (x64) or BGRA (Win32)
	// - index | index in the sender list to connect to
	//   -1 - connect to the selected sender
	//        if none selected, connect to the first sender
	bool CreateReceiver(int index = -1);

	// Create a receiver with preferred colour format
	// - colorFormat | the preferred format
	// - index | index in the sender list to connect to
	//   -1 - connect to the selected sender
	//        if none selected connect to the first sender
	bool CreateReceiver(NDIlib_recv_color_format_e colorFormat, int index = -1);

	// Return whether the receiver has been created
	bool ReceiverCreated();

	// Return whether a receiver has connected to a sender
	bool ReceiverConnected();

	// Close receiver and release resources
	void ReleaseReceiver();

	// Receive image pixels to a buffer
	// - pixel | received pixel data
	// - width | received image width
	// - height | received image height
	// - bInvert | flip the image
	bool ReceiveImage(unsigned char *pixels,
		unsigned int &width, unsigned int &height,
		bool bInvert = false);

	// Receive image pixels without a receiving buffer
	// The received video frame is held in ofxReceive class.
	// Use the video frame data pointer externally with GetVideoData()
	// For success, the video frame must be freed with FreeVideoData().
	// - width | received image width
	// - height | received image height
	bool ReceiveImage(unsigned int &width, unsigned int &height);
	   
	// Get the video type received
	// The receiver should always receive RGBA.
	// This function is backup only - no error checking.
	// NDIlib_FourCC_type_e GetVideoType();
	NDIlib_FourCC_video_type_e GetVideoType();

	// Video frame line stride in bytes
	unsigned int GetVideoStride();

	// Get a pointer to the current video frame data
	unsigned char *GetVideoData();

	// Free NDI video frame buffers
	// Must be done after successful receive of a video frame
	// if using ReceiveImage without a receiving buffer
	void FreeVideoData();

	// Create an NDI finder to find existing senders
	void CreateFinder();

	// Release an NDI finder that has been created
	void ReleaseFinder();

	// Find all current NDI senders
	// Return - number of senders
	int FindSenders();

	// Find all current NDI senders
	// nSenders - number of senders
	// Return - true for network change
	bool FindSenders(int &nSenders);

	// Refresh sender list with the current network snapshot
	// No longer used
	int RefreshSenders(uint32_t timeout = 0);

	// Set current sender index in the sender list
	bool SetSenderIndex(int index);

	// Return the current index in the sender list
	int GetSenderIndex();

	// Get the index of a sender name
	bool GetSenderIndex(const char *sendername, int &index);

	// Get the index of a sender name
	bool GetSenderIndex(std::string sendername, int &index);

	// Set a sender name to receive from
	// Only applies for inital sender connection.
	void SetSenderName(std::string sendername);

	// Return the name string of a sender index
	// no index argument means the current sender
	std::string GetSenderName(int index = -1);

	// Get the name characters of a sender index
	// For back-compatibility
	bool GetSenderName(char *sendername);
	bool GetSenderName(char *sendername, int index);
	bool GetSenderName(char *sendername, int maxsize, int index);

	// Sender width
	unsigned int GetSenderWidth();

	// Sender height
	unsigned int GetSenderHeight();

	// Sender fps
	float GetSenderFps();

	// Return the number of senders
	int GetSenderCount();

	// Return the list of senders
	std::vector<std::string> GetSenderList();

	// Set NDI low bandwidth option
	// Refer to NDI documentation
	void SetLowBandwidth(bool bLow = true);

	// Set receiver preferred format
	void SetFormat(NDIlib_recv_color_format_e format);

	// Received frame type
	ofxNDIframeinfoflags GetFrameType();

	// Is the current frame MetaData ?
	// Use when ReceiveImage fails
	bool IsMetadata();

	// Returns the current frame MetaData string
	std::string GetMetadataString();

	// Set to receive Medadata
	void SetEnableMetadata(bool bEnableMetadata);

	// Get if Medadata is enabled
	bool GetEnableMetadata() const;

	// Connection metadata, sent each time a sender connects
	// By Default: Populated with an ofxNDI <ndi_product> tag.
	// Add yours or use ClearConnectionMetadataStrings() after CreateReceiver().
	bool AddConnectionMetadataString(std::string message);
	bool ClearConnectionMetadataStrings();

	// Return the current video frame timestamp
	int64_t GetVideoTimestamp();

	// Return the current video frame timecode
	int64_t GetVideoTimecode();

	// Return the current audio frame timestamp
	int64_t GetAudioTimestamp();

	// Return the current audio frame timecode
	int64_t GetAudioTimecode();

	// Set to receive Audio
	void SetAudio(bool bAudio);

	// Is the current frame Audio data ?
	// Use when ReceiveImage fails
	bool IsAudioFrame();

	// Number of audio channels
	int GetAudioChannels();

	// Number of audio samples
	int GetAudioSamples();

	// Audio sample rate
	int GetAudioSampleRate();

	// Get audio frame data pointer
	float* GetAudioData();

	// Get audio frame data size
	int GetAudioDataStride();

	// Return audio frame data
	void GetAudioData(float*& output, int& samplerate, int& samples, int& nChannels);

	// Free audio frame buffer
	void FreeAudioData();

	// The NDI SDK version number
	std::string GetNDIversion();

	// Timed received frame rate
	int GetFps();

	// Reset starting received frame rate
	void ResetFps(double fps);

	// Get queue stats (audio, metadata, image)
	NDIlib_recv_queue_t GetQueueLengths();

	struct ofxNDIPerformanceMetrics {
		NDIlib_recv_performance_t total;
		NDIlib_recv_performance_t dropped;
	};
	// Fetch bandwidth / performance
	ofxNDIPerformanceMetrics GetPerformanceMetrics();

	// Public for external use
	const NDIlib_v4* p_NDILib;

	// ====================================================================

private:

	ofxNDIdynloader libloader;
//	const NDIlib_v4* p_NDILib;

	const NDIlib_source_t* p_sources;
	uint32_t no_sources;
	NDIlib_find_instance_t pNDI_find;
	NDIlib_recv_instance_t pNDI_recv;
	NDIlib_video_frame_v2_t video_frame;
	//NDIlib_frame_type_e m_FrameType;
	ofxNDIframeinfoflags m_FrameType;

	unsigned int m_Width;
	unsigned int m_Height;
	NDIlib_recv_color_format_e m_Format;

	std::vector<std::string> NDIsenders; // List of sender names
	int m_nSenders;// Sender count
	int m_senderIndex; // Current sender index
	std::string m_senderName; // Current sender name
	bool bNDIinitialized; // Is NDI initialized properly
	bool bReceiverCreated; // Is the receiver created
	bool bReceiverConnected; // Is the receiver connected and receiving frames
	NDIlib_recv_bandwidth_e m_bandWidth; // Bandwidth receive option

	uint32_t dwStartTime; // For timing delay
	uint32_t dwElapsedTime;

	// For received frame fps calculations
	double PCFreq;
	int64_t CounterStart;
	double startTime, lastTime;
	void StartCounter();
	double GetCounter();
	double m_fps;
	double m_frameTimeTotal;
	double m_frameTimeNumber;
	void UpdateFps();

	// Metadata
	bool m_bMetadataFrame;
	bool m_bMetadataEnabled;
	std::string m_metadataString; // XML message format string NULL terminated

	// Video timecode, timestamp
	int64_t m_VideoTimecode;
	int64_t m_VideoTimestamp;

	// Audio frame received
	bool m_bAudio; // Audio enabled
	bool m_bAudioFrame;
	float* m_AudioData;
	int m_nAudioSampleRate;
	int m_nAudioSamples;
	int m_nAudioChannels;
	int m_AudioDataStride;
	int64_t m_AudioTimecode;
	int64_t m_AudioTimestamp;

	// Replacement function for deprecated NDIlib_find_get_sources
	// If no timeout specified, return the sources that exist right now
	// For a timeout, wait for that timeout and return the sources that exist then
	// If that fails, return NULL
	const NDIlib_source_t* FindGetSources(NDIlib_find_instance_t p_instance,
		uint32_t* p_no_sources,
		uint32_t timeout_in_ms);


};

#endif
