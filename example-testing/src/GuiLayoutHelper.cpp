/*

	Gui Layout helper (minimal imgui style immediate-mode layout)

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

	xx.xx.26 - Initial GuiLayoutHelper

*/
#include "GuiLayoutHelper.h"
#include "ofGraphics.h"

#ifndef FONT_SIZE
#   define FONT_SIZE 16
#endif

//--------------------------------------------------------------
GuiLayoutHelper::GuiLayoutHelper(int posX, int posY) : curPos(posX, posY){

}

//--------------------------------------------------------------
void GuiLayoutHelper::reserveSpace(int height){
    curPos.y += height;
}

//--------------------------------------------------------------
void GuiLayoutHelper::spacing(){
    curPos.y += .5f*FONT_SIZE;
}

//--------------------------------------------------------------
void GuiLayoutHelper::indent(){
    curPos.x += FONT_SIZE;
}

//--------------------------------------------------------------
void GuiLayoutHelper::unIndent(){
    curPos.x -= FONT_SIZE;
}

//--------------------------------------------------------------
void GuiLayoutHelper::separator(){
    ofPushStyle();
    ofNoFill();
    ofSetLineWidth(1);
    ofDrawLine(curPos.x, curPos.y, curPos.x+FONT_SIZE*20, curPos.y);
    ofPopStyle();
    reserveSpace(FONT_SIZE);
}

//--------------------------------------------------------------
void GuiLayoutHelper::drawString(std::string text){
    ofDrawBitmapString(text, curPos.x, curPos.y);
    int numLines = 1;
    std::size_t pos = 0;
    pos=text.find('\n', pos);
    while(pos != std::string::npos){
        if(pos+1<text.length()){
            numLines++;
            pos=text.find('\n', pos+1);
        }
        else
            break;
    }
    curPos.y += numLines*FONT_SIZE;
}

//--------------------------------------------------------------
void GuiLayoutHelper::drawStringFormated(std::string text, ...){
    static char buf[512];
    va_list args;
    va_start(args, text);
    vsnprintf(buf, sizeof(buf), text.c_str(), args);
    va_end(args);
    drawString(buf);
}
