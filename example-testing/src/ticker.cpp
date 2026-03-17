/*

	Utility to display a metronome ticker.

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

	xx.xx.26 - Initial ticker utility

*/
#include "ticker.h"

//--------------------------------------------------------------
void Ticker::setup(){
	// Load font
	// Src : https://github.com/source-foundry/Hack/ (dual MIT and Bitstream Vera Licenses)
	font.load("Hack-Regular.ttf", 16*3, true, false, false);
}

//--------------------------------------------------------------
void Ticker::updateData(float _forceTime){
	time = _forceTime;
	loopPos = glm::mod(time, loopSeconds)/loopSeconds;
	playHeadPos = getPos(loopPos);

	for(auto& m : markers){
		m.amplitude = m.getAmplitude(loopPos);
	}
}

//--------------------------------------------------------------
float Ticker::getTime() const {
	return time;
}

//--------------------------------------------------------------
unsigned int Ticker::getTimeSeconds() const {
	return glm::floor(glm::mod(time, 60.f));
}

//--------------------------------------------------------------
unsigned int Ticker::getTimeMinutes() const {
	return glm::mod(glm::floor(time/60.f), 60.f);
}

//--------------------------------------------------------------
unsigned int Ticker::getTimeMilliseconds() const {
	return glm::floor(glm::mod(time, 1.f)*1000.f);
}

//--------------------------------------------------------------
float Ticker::getLoopTime() const {
	return loopPos;
}

//--------------------------------------------------------------
std::string Ticker::getTimeString() const {
	char buf[32];
	std::sprintf(buf, "%02u:%02u:%02u", getTimeMinutes(), getTimeSeconds(), getTimeMilliseconds()/10u);
	return buf;
}

//--------------------------------------------------------------
inline float Ticker::getPos(float _time) const {
	return (glm::cos(_time*glm::two_pi<float>()-glm::pi<float>())+1.f)*.5f;
}

//--------------------------------------------------------------
float Ticker::getPlayHeadPos() const {
	return playHeadPos;
}

//--------------------------------------------------------------
const std::array<Ticker::MarkerData,4>& Ticker::getMarkers() const {
	return markers;
}

//--------------------------------------------------------------
void Ticker::drawTime(glm::vec2 _pos, bool bDrawFrameNum){
	if(font.isLoaded()){
		ofPushStyle();
		ofSetColor(255);
		ofFill();
		std::string str;
		if(bDrawFrameNum){
			str = "Frame : ";
			str += std::to_string(ofGetFrameNum());
			font.drawString(str, _pos.x, _pos.y);
			_pos.y += font.getSize()+8;
		}
		str = "Time  : ";
		str += getTimeString();
		font.drawString(str, _pos.x, _pos.y);
		_pos.y += font.getSize()+8;
		ofPopStyle();
	}
}

//--------------------------------------------------------------
void Ticker::drawTickerTimeline(ofRectangle ticksRect, bool bDrawScale, bool bDrawMarkers, bool drawAudioSamples, bool bDrawPlayhead, ofColor playheadColor){
	ofPushStyle();

	// Timer scale / ticks
	if(bDrawScale){
		ofNoFill();
		ofSetLineWidth(2);
		for(int i=0; i<=20; i++){
			int xPos = ticksRect.x+((ticksRect.width/20.f)*i);
			ofDrawLine(xPos, ticksRect.y, xPos, ticksRect.y+ticksRect.height);
		}
	}

	// Markers
	if(bDrawMarkers){
		ofSetLineWidth(5);
		ofFill();
		for(const auto& m : getMarkers()){
			float markerPos = getPos(m.position);
			int xPos = ticksRect.x+ticksRect.width*markerPos;

			// Marker dot (when playhead is closeby 1 sec)
			float dist = glm::abs(getLoopTime()-m.position);
			if(glm::min(dist, 1.f-dist)<(1.f/loopSeconds)){

				ofSetColor(0, 0, 0);
				ofDrawCircle(xPos, ticksRect.y-10, 0, 10);
				int time = (glm::floor((getTime())/loopSeconds)+m.position)*loopSeconds;
				if(m.position==0 && dist>1.f-dist) time+=loopSeconds; // fixes loop-around bug
				int sec = time % 60;
				int min = glm::floor((time/60)%60);
				std::string str = std::string((min<10)?"0":"")+std::to_string(min)+":"+((sec<10)?"0":"")+std::to_string(sec);
				ofDrawBitmapStringHighlight(str, xPos-20, ticksRect.y+ticksRect.height+16, ofColor::white, ofColor::black);
			}
		}
		ofSetLineWidth(1); // restore
		ofSetColor(ofColor::white);
	}

	// Highest audio sample
	if(drawAudioSamples && fbo.isAllocated()){
		ofFill();
		ofSetColor(255,255,255);
		fbo.draw(ticksRect.x, ticksRect.y);
	}

	// Active marker (ticked) - above plain marker
	if(bDrawMarkers) {
		for(const auto& m : getMarkers()){
			if(m.amplitude>0){
				float markerPos = getPos(m.position);
				int xPos = ticksRect.x+ticksRect.width*markerPos;
				ofSetColor(255, 0, 0, 255*m.amplitude);
				ofNoFill();
				ofDrawLine(xPos, ticksRect.y, xPos, ticksRect.y+ticksRect.height);
				ofFill();
				ofDrawCircle(xPos, ticksRect.y-10, 0, 10);
			}
		}
	}

	// Loop Playhead
	if(bDrawPlayhead){
		ofFill();
		ofPushMatrix();
		ofSetCircleResolution(3); // circle = triangle !
		ofTranslate(ticksRect.x+(ticksRect.width*getPlayHeadPos()), ticksRect.y+ticksRect.height*.6f);
		ofRotateZDeg(-90);// so the triangle looks like a playhead
		ofSetColor(playheadColor.getInverted());
		ofDrawCircle(0, 0, 20);
		ofSetColor(playheadColor);
		ofDrawCircle(0, 0, 16);
		ofSetCircleResolution(22); // of Default !
		ofPopMatrix();
	}

	ofPopStyle();
}

//--------------------------------------------------------------
void Ticker::drawHighestAudioSample(float _volume, ofRectangle ticksRect, ofColor color){
	if(ticksRect.getWidth()<1 || ticksRect.getHeight()<1){
		return;
	}
	static const int width = 3;
	if(!fbo.isAllocated() || fbo.getWidth()!=ticksRect.width || fbo.getHeight()!=ticksRect.height){
		fbo.allocate(ticksRect.getWidth(), ticksRect.getHeight(), GL_RGBA);
		if(fbo.isAllocated()){
			fbo.begin();
			ofClear(0,0,0,0);
			fbo.end();
		}
	}
	if(!fbo.isAllocated()) return;

	fbo.begin();
	ofPushStyle();

	ofFill();
	// erase playhead
	glEnable(GL_SCISSOR_TEST);
	int scissorX = ticksRect.width*getPlayHeadPos()+width*((loopPos<0.5f)?+10.f:-10.f);
	//static GLint oldScissor[4]; glGetIntegerv(GL_SCISSOR_BOX, oldScissor);
	glScissor(scissorX, 0, width*10.f, ticksRect.height);          // clear the three columns over the whole height
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	//glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]); // Resets, needed for ofDrawBitmapString(Highlight)
	glDisable(GL_SCISSOR_TEST);
	ofSetColor(color);
	ofDrawCircle((ticksRect.width*getPlayHeadPos()), ticksRect.height-ticksRect.height*_volume, width);
	ofPopStyle();
	fbo.end();
}

//--------------------------------------------------------------
void Ticker::drawHighestAudioSampleFBO(glm::vec2 _pos){
	if(fbo.isAllocated()){
		ofFill();
		ofSetColor(255,255,255);
		fbo.draw(_pos);
	}
}
