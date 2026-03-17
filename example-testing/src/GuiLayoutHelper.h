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
#pragma once

// Gui Helper
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "ofVec2f.h" // glm stuff

struct GuiLayoutHelper {
    glm::vec<2, int> curPos;
    GuiLayoutHelper(int posX, int posY);
    void reserveSpace(int height);
    void spacing();
    void indent();
    void unIndent();
    void separator();
    void drawString(std::string text);
    void drawStringFormated(std::string text, ...);
};
