/*
	ofxNDI + ImGui Helpers

	Providing easy to use widgets for controlling ofxNDI components with ofxImGui.

	Enable with `ofxAddons_ENABLE_IMGUI`, automatically defined when ofxImGui is included.

	Note: You need the develop branch available here. https://github.com/jvcleave/ofxImGui/tree/develop

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

    29.09.24 - Initial Implementation with ofxNDIsender gui helpers

*/

#pragma once

#ifndef __ofxNDIImGuiEx__
#define __ofxNDIImGuiEx__

// Only compile this when ofxImGui is enabled
#ifdef ofxAddons_ENABLE_IMGUI

#include "ofMain.h"
#include "imgui.h"
#include "ofxNDIsender.h"

#ifndef IMGUI_HELPMARKER
#define IMGUI_HELPMARKER(STR) \
    ImGui::SameLine();\
    ImGui::TextDisabled("[?]");\
    if (ImGui::IsItemHovered()) {\
        if (ImGui::BeginTooltip()) {\
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);\
            ImGui::TextUnformatted(STR);\
            ImGui::PopTextWrapPos();\
            ImGui::EndTooltip();\
        }\
    }
#endif // IMGUI_HELPMARKER

namespace ImGuiEx {

    // Draws Sender settings
    // Server name, width and height : only if ofxNdiSenderImGuiSettingsFlags_EnableDisable is set, for when the server is enabled.
    // Either all 3 optional args are needed or none is considered (they are bound together, changing one requires resetting the whole server)
    bool ofxNdiSenderSetup(ofxNDIsender& sender, const char* serverNameToCreate=nullptr, unsigned int serverWidth=0u, unsigned int serverHeight=0u);

    // Runtime settings
    bool ofxNdiSenderSettings(ofxNDIsender& sender);
    inline bool ofxNdiSenderFrameRate(ofxNDIsender& ndiSender);
    inline bool ofxNdiSenderAsync(ofxNDIsender& ndiSender);
    inline bool ofxNdiSenderReadback(ofxNDIsender& ndiSender);
    inline bool ofxNdiSenderProgressive(ofxNDIsender& ndiSender);
    inline bool ofxNdiSenderClockVideo(ofxNDIsender& ndiSender);
    inline bool ofxNdiSenderFormat(ofxNDIsender& ndiSender);

    // Draws the whole status as text
    void ofxNdiSenderStatusText(ofxNDIsender& sender);
}

#endif // ofxAddons_ENABLE_IMGUI

#endif // ofxNDIImGuiEx
