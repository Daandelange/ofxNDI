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
#pragma once

#include "ofMain.h"
#include "ofxNDI.h" // NDI classes
#include "ofxImGui.h" // Gui

class ofApp : public ofBaseApp{

	public:

		void setup();
		void update();
		void draw();
		void exit();

		// GUI
		ofxImGui::Gui gui;

		// Receiver
		ofxNDIreceiver ndiReceiver;
		ofTexture receiverTexture;

		// Sender
		ofxNDIsender ndiSender;        // NDI sender
		std::string senderName;        // Sender name
		unsigned int senderWidth = 0;  // Width of the sender output
		unsigned int senderHeight = 0; // Height of the sender output

		ofFbo senderFbo;               // Fbo used for graphics and sending
		float rotX = 0.0f;
		float rotY = 0.0f;             // Cube rotation increment
		void DrawSenderGraphics();     // Rotating cube draw

};
