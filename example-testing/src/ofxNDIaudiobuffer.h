/*

	ofxNDIaudiobuffer - Circular ring buffer for forwarding NDI audio samples to ofSoundBuffer.

	Adaptation of Cinder's RingBuffer for OpenframeWorks & ofxNDI.
	https://github.com/cinder/Cinder/blob/master/include/cinder/audio/dsp/RingBuffer.h

	Copyright (c) 2014, The Cinder Project

	Copyright (C) 2026 Daan de Lange.

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

	xx.xx.26 - Initial ofxNDIaudiobuffer (unstable!?)

*/
#pragma once

#include <vector>
#include <mutex>
#include "ofxNDI.h"
#include "Processing.NDI.utilities.h"

// Ring-buffer helper for receiving audio frames and passing them to ofSoundBuffer
class ofxNDIaudiobuffer {

	public:
		// Adds the samples provided by ndiReceiver to the internal buffer
		bool writeSamplesToBuffer(ofxNDIreceiver& ndiReceiver){
			if(ndiReceiver.IsAudioFrame()) {
				// If ReceiveImage fails, query for an audio frame
				// Check for both the frame type and audio data
				if (ndiReceiver.GetFrameType() & ofxNDIframeinfoflags_audio) {
					float* ndiAudioData = ndiReceiver.GetAudioData();
					if (ndiAudioData) {

						// Audio sample rate
						sampleRate = ndiReceiver.GetAudioSampleRate();
						// Number of audio channels
						nChannels = ndiReceiver.GetAudioChannels();
						// Number of audio samples per channel
						nSamples = ndiReceiver.GetAudioSamples();
						// Audio data stride in bytes
						nStride = ndiReceiver.GetAudioDataStride();

						// nSamples and nStride may change every frame if
						// the sender has an alternating sequence per frame
						// For example : 48000hz audio at 29.97 video fps
						//   1602, 1601, 1602, 1601, 1602
						// 48000hz audio at 30 fps requires only one value
						//   48000/30 = 1600

						// printf("sampleRate = %d\n", sampleRate);
						// printf("nChannels  = %d\n", nChannels);
						// printf("nSamples   = %d\n", nSamples);
						// printf("nStride    = %d\n", nStride);

						// Set up soundstream to match with the sender audio
						//if (!bSoundStream) {
						//	SetupSoundStream();
						//	// Return for the next frame
						//	return;
						//}

						const size_t curWriteIndex = writeIndex.load( std::memory_order_relaxed );
						const size_t curReadIndex = readIndex.load( std::memory_order_acquire );


						//
						// NDI senders produce planar audio data
						//
						// NDIlib_audio_frame_v3_t audio frame has a
						// member "NDIlib_FourCC_audio_type_e FourCC"
						// which allows the type of audio data to be described.
						// The default is NDIlib_FourCC_audio_type_FLTP
						// (planar 32-bit float)
						//
						// The NDIlib_audio_frame_v2_t currently used in
						// the addon does not have a FourCC member and
						// the default planar type is used.
						//
						// There are nSamples of audio per channel
						// and nSamples*2 per frame for stereo
						// nStride is the number of bytes of audio data
						// for nSamples	per channel.
						//
						//     <-        nSamples*2       ->
						//     <- nSamples ->  <- nSamples ->
						//      L L L L L L L - R R R R R R R
						//

//						float* left  = ndiAudioData;
//						float* right = ndiAudioData + nStride/sizeof(float);
//						{
//							// Mutex lock for variables shared with audioOut
//							// (availableSamples and audioBuffer)
//							//std::lock_guard<std::mutex> lock(audioMutex);
//							//
//							// Get the left and right channels
//							//
////							lAudio.resize(nSamples);
////							rAudio.resize(nSamples);
//							for (int i = 0; i<nSamples; i++) {
//								// left channel
//								audioBuffer[writeIndex] = left[i];
//								writeIndex = (writeIndex+1) % bufferCapacity;
////								lAudio[i] = left[i];
//								// right channel
//								audioBuffer[writeIndex] = right[i];
//								writeIndex = (writeIndex+1) % bufferCapacity;
//								availableSamples += 2;
////								rAudio[i] = right[i];
//							}
//						} // end mutex lock

						// Convert from planar (NDI) to interleaved (ofSoundBuffer)
						//
						static ofSoundBuffer interlacingBuffer;
						if(interlacingBuffer.getSampleRate() != sampleRate) {
							interlacingBuffer.setSampleRate(sampleRate);
						}
						if(interlacingBuffer.getNumChannels() != nChannels ||
						   interlacingBuffer.getNumFrames() != nSamples) {
							interlacingBuffer.allocate(nSamples, nChannels);
						}

						float* bufferWritePos = &audioBuffer[0] + writeIndex;
						//NDIlib_audio_frame_interleaved_32f_t interleaved_frame(sampleRate, nChannels, nSamples, NDIlib_send_timecode_synthesize, static_cast<float*>(bufferWritePos));
						NDIlib_audio_frame_interleaved_32f_t interleaved_frame(sampleRate, nChannels, nSamples, NDIlib_send_timecode_synthesize, static_cast<float*>(interlacingBuffer.getBuffer().data()));

						NDIlib_audio_frame_v2_t v2_frame(sampleRate, nChannels, nSamples, NDIlib_send_timecode_synthesize, reinterpret_cast<float*>(ndiAudioData), nStride);
						//NDIlib_util_audio_to_interleaved_32f_v2(&v2_frame, &interleaved_frame);

						ndiReceiver.NDIreceiver.p_NDILib->NDIlib_util_audio_to_interleaved_32f_v2(&v2_frame, &interleaved_frame);

						// Copy transformed data to buffer
						const size_t count = nChannels*nSamples;

						// Not enough space ?
						if( count > getAvailableWrite( writeIndex, readIndex ) ){
							ofLogWarning("ofxNDIaudiobuffer") << "Buffer overflow detected !";
							//std::cout << "Capacity=" << bufferCapacity << ", r=" << curReadIndex << ", w=" << curWriteIndex << std::endl;
							return false;
						}
						if( count > bufferCapacity ){
							ofLogWarning("ofxNDIaudiobuffer") << "Wrongly setup : please use a capacity bigger than the NDI buffer.";
							return false;
						}
						size_t writeIndexAfter = writeIndex + count;

						if( writeIndex + count > bufferCapacity ) {
							size_t countA = bufferCapacity - writeIndex;
							size_t countB = count - countA;

							std::memcpy( audioBuffer + writeIndex, interlacingBuffer.getBuffer().data(), countA * sizeof( float ) );
							std::memcpy( audioBuffer, interlacingBuffer.getBuffer().data() + countA, countB * sizeof( float ) );
							writeIndexAfter -= bufferCapacity;
						}
						else {
							std::memcpy( audioBuffer + curWriteIndex, interlacingBuffer.getBuffer().data(), count * sizeof( float ) );
							if( writeIndexAfter == bufferCapacity )
								writeIndexAfter = 0;
						}

						writeIndex.store( writeIndexAfter, std::memory_order_release );
						return true;

					} // endif received audio data
				}
				else if (ndiReceiver.GetFrameType() & ofxNDIframeinfoflags_audio) {
					std::cout << "MissedAudioFrame!" << std::endl;
				}
			}
			return false;
		}

		// Writes samples from the internal buffer to the sound buffer
		// Thread-safe if called from the audio thread only (or the main thread)
		bool readSamplesFromBuffer(ofSoundBuffer& buffer){
			// !! Possibly called from audio thread !!
			//std::lock_guard<std::mutex> lock(audioMutex);

			for (size_t i = 0; i < buffer.size(); i++) {
				if (availableSamples > 0) {
					buffer[i] = audioBuffer[readIndex];
					readIndex = (readIndex + 1) % bufferCapacity;
					availableSamples--;
				}
				else {
					buffer[i] = 0.0f;
				}
			}

			// Get buffer pos
			const size_t curWriteIndex = writeIndex.load( std::memory_order_acquire );
			const size_t curReadIndex = readIndex.load( std::memory_order_relaxed );
			const size_t count = buffer.getNumChannels()*buffer.getNumFrames();

			// Check count
			if( count > getAvailableRead( curWriteIndex, curReadIndex ) )
				return false;

			// Calc next read pos
			size_t readIndexAfter = curReadIndex + count;

			// Copy either 1 or 2 memory  blocks (depending on loop state)
			if( curReadIndex + count > bufferCapacity ) {
				size_t countA = bufferCapacity - curReadIndex;
				size_t countB = count - countA;

				std::memcpy( buffer.getBuffer().data(), audioBuffer + curReadIndex, countA * sizeof( float ) );
				std::memcpy( buffer.getBuffer().data() + countA, audioBuffer, countB * sizeof( float ) );

				readIndexAfter -= bufferCapacity;
			}
			else {
				std::memcpy( buffer.getBuffer().data(), audioBuffer + curReadIndex, count * sizeof( float ) );
				if( readIndexAfter == bufferCapacity )
					readIndexAfter = 0;
			}

			readIndex.store( readIndexAfter, std::memory_order_release );
			return true;
		}

		size_t getAvailableWrite() const {
			return getAvailableWrite( writeIndex, readIndex );
		}
		//! Returns the number of elements available for wrtiing. \note Only safe to call from the read thread.
		size_t getAvailableRead() const {
			return getAvailableRead( writeIndex, readIndex );
		}

		void setBufferSize(int newSize){

		}

		size_t getBufferSize() const{
			return bufferCapacity - 1;
		}

		void clear(){
			writeIndex = 0;
			readIndex = 0;
		}

		void resize( std::size_t count ){
			const size_t allocatedSize = count + 1; // one bin is used to distinguish between the read and write indices when full.

			if( bufferCapacity )
				audioBuffer = (float *)::realloc( audioBuffer, allocatedSize * sizeof( float ) );
			else
				audioBuffer = (float *)::calloc( allocatedSize, sizeof( float ) );

			assert( audioBuffer ); // Allocation error ?

			bufferCapacity = allocatedSize;
			clear();
		}

		// Returns the number of samples the buffer is piling up (current buffer read delay)
		std::size_t getBufferReadOffsetSamples() const {
			size_t w = writeIndex.load(std::memory_order_acquire);
			size_t r = readIndex.load(std::memory_order_acquire);
//			size_t used = (getBufferSize()+w-r) % getBufferSize();//?(w - r):r-w;
//			int used = glm::abs(((int)w) - r);
			int used = w>r?(((int)w) - r):((getBufferSize()+(int)w) - r);
			return used;
		}

		float getBufferLoad() const {
//			size_t w = writeIndex.load(std::memory_order_acquire);
//			size_t r = readIndex.load(std::memory_order_acquire);
////			size_t used = (getBufferSize()+w-r) % getBufferSize();//?(w - r):r-w;
////			int used = glm::abs(((int)w) - r);
//			int used = w>r?(((int)w) - r):((getBufferSize()+(int)w) - r);
			//std::cout << "getBufferLoad = " << (used) << "  ("<< w << " - " << r << ")" <<std::endl;
			std::size_t used = getBufferReadOffsetSamples();
			return static_cast<float>(used) / static_cast<float>(getBufferSize());
//			return static_cast<float>(used) / static_cast<float>(bufferCapacity);
		}

	protected:
		// Audio
		float* ndiAudioData = nullptr; // NDI audio pointer
		int nChannels = 2;//4800;//NUMCHANNELS;
		int nSamples = 0;
		int nStride = 0;
		int sampleRate = 4800;
//		size_t writeIndex = 0u;
//		size_t readIndex = 0u;
		std::atomic<size_t> writeIndex = 0u;
		std::atomic<size_t> readIndex = 0u;
		size_t bufferCapacity = 0u;
		size_t availableSamples = 0u;

		// Buffer
//		std::vector<float> audioBuffer;  // Buffer for the audio data
		float* audioBuffer = nullptr;
//		std::mutex audioMutex;
		int bufferSize = 0;         // Buffer size (change with sample number)

		size_t getAvailableWrite( size_t writeIndex, size_t readIndex ) const {
			size_t result = readIndex - writeIndex - 1;
			if( writeIndex >= readIndex )
				result += bufferCapacity;

			return result;
		}

		size_t getAvailableRead( size_t writeIndex, size_t readIndex ) const {
			if( writeIndex >= readIndex )
				return writeIndex - readIndex;

			return writeIndex + bufferCapacity - readIndex;
		}
};
