/*

	Ticker utility, to simplify the example code.

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
*/
#pragma once

#include "ofMain.h"
#include <array>
#include "ofFbo.h"
#include "ofTrueTypeFont.h"
#include "ofUtils.h" // ofGetElapsedTimef
#include "ofMath.h" // glm stuff

struct Ticker {
	struct MarkerData {
		const float position = 0.f;
		const float treshold = 0.02f;
		const int freq = 400;
		float amplitude = 0.f;

		inline float getAmplitude(float _time) const {
			return glm::smoothstep(cos(treshold*PI), glm::cos(0), glm::cos((position+_time)*TWO_PI));
		}
	};

	void setup();
	void updateData(float _forceTime=ofGetElapsedTimef());

	float getTime() const;
	unsigned int getTimeSeconds() const;
	unsigned int getTimeMinutes() const;
	unsigned int getTimeMilliseconds() const;
	float getLoopTime() const;
	std::string getTimeString() const;

	// Utils
	inline float getPos(float _time) const;
	float getPlayHeadPos() const;
	const std::array<MarkerData,4>& getMarkers() const;

	void drawTime(glm::vec2 _pos, bool bDrawFrameNum=true);
	void drawTickerTimeline(ofRectangle ticksRect, bool bDrawScale=true, bool bDrawMarkers=true, bool drawAudioSamples=true, bool bDrawPlayhead=true, ofColor playheadColor=ofColor::black);
	void drawHighestAudioSample(float _volume, ofRectangle ticksRect, ofColor color=ofColor::red);
	void drawHighestAudioSampleFBO(glm::vec2 _pos={0,0});

	const float loopSeconds = 4;

protected:
	float time = 0.f;
	std::array<float,4> amplitudes = {0.f};
	float loopPos = 0.f;
	float playHeadPos = 0.f;
	std::array<MarkerData, 4> markers {{
		{ 0.0f,  0.1f, 500, 0.f },
		{ 0.25f, 0.04f, 1600, 0.f },
		{ 0.5f,  0.1f, 500, 0.f },
		{ 0.75f, 0.04f, 1600, 0.f },
	}};

	ofFbo fbo;
	ofTrueTypeFont font;
};
