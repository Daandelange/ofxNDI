/*

	OpenFrameworks ofxNDI + ofxImGui example

	Requires ofxImGui : https://github.com/jvcleave/ofxImGui/tree/develop

	Copyright (C) 2024 Daan de Lange.

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

	04.10.24 - Initial creation


*/
#include "ofApp.h"
#include "ofxNDIImGuiEx.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofBackground(0);
	ofSetColor(255);
	ofSetWindowTitle("OpenFrameworks ofxNDI + ofxImGui example");
	ofSetVerticalSync(false);
	ofSetFrameRate(60);

	// GUI Setup
	gui.setup();
	ofDisableArbTex(); // Needed for displaying imgui images !

	// Sender setup
	senderName = "ofxNDI + ofxImGui example";
	senderWidth  = 1920;
	senderHeight = 1080;

	// Create an RGBA fbo for collection of data
	senderFbo.allocate(senderWidth, senderHeight, GL_RGBA);

	// Sender
	ndiSender.SetReadback(false);
	ndiSender.SetFrameRate(30);
	ndiSender.SetAsync();
	ndiSender.CreateSender(senderName.c_str(), senderWidth, senderHeight);

	// Receiver setup
	//ndiReceiver.CreateReceiver();
	//ndiReceiver.CreateFinder();
	ndiReceiver.SetSenderName(senderName);

	std::cout << ndiReceiver.GetNDIversion() << " (https://www.ndi.tv/)" << std::endl;

	// ofTexture
	receiverTexture.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA, false); // Fixme : ofxNDI will override the ARB argument when the receiver size changes.

	// Cube rotation
	rotX = 0;
	rotY = 0;
}


//--------------------------------------------------------------
void ofApp::update() {

}

//--------------------------------------------------------------
void ofApp::draw() {

	// Receive ofTexture
	ndiReceiver.ReceiveImage(receiverTexture);

	// Render to FBO
	DrawSenderGraphics();

	// Send FBO
	ndiSender.SendImage(senderFbo);

	// Draw the GUI
	gui.begin();

	// Menu bar
	if(ImGui::BeginMainMenuBar()){

		// Openfrmeworks runtime settings
		if(ImGui::BeginMenu("OpenFrameworks")){
			ImGui::TextDisabled("Here you can change some OF settings.");
			ImGui::TextDisabled("They affect ofxNDI's performance.");
			static int frameRate = ofGetTargetFrameRate();
			ImGui::Text("FPS    : %03.0f / %03i", ofGetFrameRate(), frameRate);
			if(ImGui::InputInt("Target FPS", &frameRate, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue )){
				ofSetFrameRate(frameRate);
				frameRate = ofGetTargetFrameRate();
			}
			ImGui::Text("V-Sync : ");
			ImGui::SameLine();
			if(ImGui::Button("Disable")){
				ofSetVerticalSync(false);
			}
			ImGui::SameLine();
			if(ImGui::Button("Enable")){
				ofSetVerticalSync(true);
			}
			ImGui::Text("Uptime     : %.1f seconds", ofGetElapsedTimef() );
			ImGui::Text("Resolution : %i x %i (ratio %.2f)", ofGetWindowWidth(), ofGetWindowHeight(), (((float)ofGetWindowWidth())/ofGetWindowHeight()) );

			ImGui::EndMenu();
		}

		// Show some status messages in the menu
		if(ImGui::BeginMenu("Sender")){
			ImGui::SeparatorText("Sender");
			ImGuiEx::ofxNdiSenderStatusText(ndiSender);
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Receiver")){
			ImGui::SeparatorText("Receiver");
			ImGuiEx::ofxNdiReceiverStatusText(ndiReceiver);

			// A preview of the receiver image
			if(ImGui::BeginMenu("Receiver Image")){
				ImGuiEx::ofxNdiReceiverImage(receiverTexture, &ndiReceiver);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
	}
	ImGui::EndMainMenuBar();

	// Receiver window with settings
	if(ImGui::Begin("Receiver")){
		ImGui::SeparatorText("Receiver Settings");
		ImGuiEx::ofxNdiReceiverSetup(ndiReceiver, true);

		ImGui::SeparatorText("Server Selection");
		ImGuiEx::ofxNdiReceiverServerSelector(ndiReceiver, true);

		ImGui::SeparatorText("Frame Information");
		ImGuiEx::ofxNdiReceiverFrameInfo(ndiReceiver, true);
	}
	ImGui::End();

	// Sender window with settings
	if(ImGui::Begin("Sender")){
		ImGui::SeparatorText("Sender Setup");
		ImGuiEx::ofxNdiSenderSetup(ndiSender, senderName.c_str(), senderWidth, senderHeight);

		ImGui::Spacing();
		ImGui::SeparatorText("Sender Settings");
		ImGuiEx::ofxNdiSenderSettings(ndiSender);
	}
	ImGui::End();

	gui.end();
}

//--------------------------------------------------------------
void ofApp::DrawSenderGraphics() {

	// Draw graphics into an fbo
	senderFbo.begin();
	ofClear(0, 0, 0, 255);
	ofPushStyle();
	float elapsed = glm::mod(ofGetElapsedTimef()*0.2f, 1.f);

	// Draw receiver behind
	ofFill();
	ofSetColor(ofColor::fromHsb(elapsed*255, 20, 255, 255));
	static const int offset = 5;
	receiverTexture.draw(-offset, -offset, senderFbo.getWidth()+offset*2, senderFbo.getHeight()+offset*2);
	ofEnableDepthTest();
	ofPushMatrix();

	ofTranslate((float)senderWidth / 2.0, (float)senderHeight / 2.0, 0);
	ofRotateYDeg(rotX);
	ofRotateXDeg(rotY);

	// Draw box with texture
	ofSetColor(255,255,255,255);
	receiverTexture.bind();
	ofDrawBox(0.7 * (float)senderHeight);
	receiverTexture.unbind();

	// Box Lines
	ofNoFill();
	ofSetLineWidth(20);
	ofDrawBox(0.8 * (float)senderHeight);
	ofPopMatrix();

	// Some line behind the cube
	ofSetLineWidth(10);
	ofDrawLine(senderWidth*elapsed,0, senderWidth*elapsed, senderHeight);
	ofDrawLine(senderWidth-senderWidth*elapsed,0, senderWidth-senderWidth*elapsed, senderHeight);

	ofDisableDepthTest();
	ofPopStyle();
	senderFbo.end();

	// Rotate the cube
	rotX += 0.551*.5f;
	rotY += 0.624 *.5f;

	// Draw the fbo result fitted to the display window
	senderFbo.draw(0, 0, ofGetWidth(), ofGetHeight());
}

//--------------------------------------------------------------
void ofApp::exit() {

	// Release NDI objects
	ndiReceiver.ReleaseReceiver();
	ndiSender.ReleaseSender();

}
