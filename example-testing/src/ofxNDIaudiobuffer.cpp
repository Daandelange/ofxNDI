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

	xx.xx.26 - Initial ofxNDIaudiobuffer

*/
#include "ofxNDIaudiobuffer.h"

//--------------------------------------------------------------

