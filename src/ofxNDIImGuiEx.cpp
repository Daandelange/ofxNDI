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

#ifdef ofxAddons_ENABLE_IMGUI

#include "ofxNDIImGuiEx.h"
#include <string>
#include <map>
#include "ofAppRunner.h"

namespace ImGuiEx {

// Private Helpers
// - - - - - - - - -
// Defined in CPP so nobody relies on it, for internal use only.
// Mainly used to cache the server name, width, height which are unset when turned off.
struct ofxNDIsenderCreationSettings {
    char name[256];
    unsigned int width = 0;
    unsigned int height = 0;
    ofxNDIsenderCreationSettings() = delete;
    ofxNDIsenderCreationSettings(const char* serverNameToCreate, unsigned int serverWidth, unsigned int serverHeight) :
        name(""),
        width(serverWidth),
        height(serverHeight)
    {
        if(serverNameToCreate!=nullptr)
            std::strncpy(&name[0], &serverNameToCreate[0], IM_ARRAYSIZE(name)-1);
    }
    ofxNDIsenderCreationSettings(ofxNDIsender& sender) :
        name(""),
        width(0),
        height(0)
    {
        matchSender(sender);
    }
    void matchSender(ofxNDIsender& sender){
        if(sender.SenderCreated()){
            std::strncpy(&name[0], sender.GetSenderName().c_str(), IM_ARRAYSIZE(name)-1);
            width = sender.GetWidth();
            height = sender.GetHeight();
        }
    }
};

std::map<ofxNDIsender*, ofxNDIsenderCreationSettings> ndiSenderIdentities;
std::pair<ofxNDIsender*, ofxNDIsenderCreationSettings> ndiSenderEditing = {nullptr, {"", 0u, 0u}};

inline ofxNDIsenderCreationSettings& getIdentity(ofxNDIsender& sender, const char* serverNameToCreate, unsigned int serverWidth, unsigned int serverHeight){
    auto it = ndiSenderIdentities.find(&sender);
    if(it==ndiSenderIdentities.end()){
        // Init first entry with args
        if(sender.SenderCreated()){
            // Use sender for actual settings
            it = ndiSenderIdentities.emplace(&sender, sender).first;
        }
        else {
            // Init from desired settings
            if(serverNameToCreate!=nullptr && strlen(serverNameToCreate)!=0 && serverWidth!=0 && serverHeight!=0){
                it = ndiSenderIdentities.emplace(std::make_pair<ofxNDIsender*, ofxNDIsenderCreationSettings>(&sender, {serverNameToCreate, serverWidth, serverHeight})).first;
            }
            else {
                it = ndiSenderIdentities.emplace(std::make_pair<ofxNDIsender*, ofxNDIsenderCreationSettings>(&sender, {"ofxNDI Sender", (unsigned int)ofGetWidth(), (unsigned int)ofGetHeight()})).first;
            }
        }
    }
    // If not editing and hints are different, sync them
    const bool bIsEditing = &sender == ndiSenderEditing.first;
    if(!bIsEditing){
        if(serverNameToCreate!=nullptr && std::strcmp(serverNameToCreate, it->second.name) != 0 ){
            std::strncpy(&it->second.name[0], serverNameToCreate, IM_ARRAYSIZE(it->second.name)-1);
        }
    }
    return it->second;
}

// Maths from ofxNDIsend::SetFormat
struct ofxNDIsenderFormatGui {
    NDIlib_FourCC_video_type_e code;
    const char name[5] = "";
    ofxNDIsenderFormatGui()=delete;
    ofxNDIsenderFormatGui(NDIlib_FourCC_video_type_e format) :
        code(format),
        name { static_cast<char>(format & 0xFF), static_cast<char>((format >> 8) & 0xFF), static_cast<char>((format >> 16) & 0xFF), static_cast<char>((format >> 24) & 0xFF), '\0' }
    {}
};
const static ofxNDIsenderFormatGui supportedFormats[] = {
  {NDIlib_FourCC_video_type_BGRA},
  {NDIlib_FourCC_video_type_BGRX},
  {NDIlib_FourCC_video_type_RGBA},
  {NDIlib_FourCC_video_type_RGBX},
  {NDIlib_FourCC_video_type_UYVY}
};

// Public functions
// - - - - - - - - -

// Draws sender setup settings
bool ofxNdiSenderSetup(ofxNDIsender& ndiSender, const char* serverNameToCreate, unsigned int serverWidth, unsigned int serverHeight){
    bool didChange = false;
    const bool bIsEditing = &ndiSender == ndiSenderEditing.first;
    bool bNdiEnabled = ndiSender.SenderCreated();
    ofxNDIsenderCreationSettings& id = getIdentity(ndiSender, serverNameToCreate, serverWidth, serverHeight);

    // Enabler button (Can always be disabled, enabled only with name)
    const bool bCanBeDisabled = bNdiEnabled || (std::strlen(id.name)!=0 && id.width!=0 && id.height!=0);
    if(!bCanBeDisabled) ImGui::BeginDisabled();
    if(ImGui::Checkbox("Enable NDI output", &bNdiEnabled)){
        if(bNdiEnabled && !ndiSender.SenderCreated()){
            if(!ndiSender.CreateSender(id.name, id.width, id.height)){
                ofLogWarning("ImGuiEx::ofxNdiSenderSetup()") << "Couldn't create NDI server !";
            }
        }
        else if(!bNdiEnabled && ndiSender.SenderCreated()){
            // Store last used settings
            id.matchSender(ndiSender);
            // Release
            ndiSender.ReleaseSender();
        }
        didChange = true;
    }
    if(!bCanBeDisabled) ImGui::EndDisabled();

    // Name
    if(!bIsEditing) ImGui::BeginDisabled();
    if(ImGui::InputText("Server name", bIsEditing?&ndiSenderEditing.second.name[0]:&id.name[0], IM_ARRAYSIZE(id.name), ImGuiInputTextFlags_EnterReturnsTrue)){
        didChange = true;
    }

    // Width
    unsigned int whStep = 1u, whStepFast=100;
    if(ImGui::InputScalar("Width", ImGuiDataType_U32, bIsEditing?&ndiSenderEditing.second.width:&id.width, &whStep, &whStepFast, "%u", ImGuiInputTextFlags_EnterReturnsTrue)){
        didChange = true;
    }

    // Height
    if(ImGui::InputScalar("Height", ImGuiDataType_U32, bIsEditing?&ndiSenderEditing.second.height:&id.height, &whStep, &whStepFast, "%u", ImGuiInputTextFlags_EnterReturnsTrue)){
        didChange = true;
    }
    if(!bIsEditing) ImGui::EndDisabled();

    // Apply + Edit btn
    if(!bIsEditing){
        if(ImGui::Button("Change Server Setup")){
            ndiSenderEditing.first = &ndiSender;
            ndiSenderEditing.second.matchSender(ndiSender);
        }
    }
    else {
        if(ImGui::Button("Cancel")){
            ndiSenderEditing.first = nullptr;
        }
        ImGui::SameLine();
        if(ImGui::Button("Apply and (re)Start") || didChange){
            didChange = true;
        }
    }

    // Apply
    if(bIsEditing && didChange){
        std::strncpy(id.name, ndiSenderEditing.second.name, IM_ARRAYSIZE(id.name)-1);
        id.width = ndiSenderEditing.second.width;
        id.height = ndiSenderEditing.second.height;
        if(ndiSender.SenderCreated()) ndiSender.ReleaseSender();
        if(!ndiSender.CreateSender(id.name, id.width, id.height)){
            ofLogWarning("ImGuiEx::ofxNdiSenderSetup()") << "Couldn't create NDI server !";
        }
        ndiSenderEditing.first = nullptr;
    }

    return didChange;
}

// Draws sender runtime settings
bool ofxNdiSenderSettings(ofxNDIsender& ndiSender){
    bool didChange = false;
    didChange |= ofxNdiSenderFrameRate(ndiSender);
    didChange |= ofxNdiSenderFormat(ndiSender);
    didChange |= ofxNdiSenderAsync(ndiSender);
    didChange |= ofxNdiSenderReadback(ndiSender);
    didChange |= ofxNdiSenderProgressive(ndiSender);
    didChange |= ofxNdiSenderClockVideo(ndiSender);
    return didChange;
}

inline bool ofxNdiSenderFrameRate(ofxNDIsender& ndiSender){
    double ndiFpsCap = ndiSender.GetFrameRate();
    if(ImGui::InputDouble("FPS Cap", &ndiFpsCap, 1.f, 1.f, "%.0f", ImGuiInputTextFlags_EnterReturnsTrue)){
        ndiSender.SetFrameRate(ndiFpsCap);
        return true;
    }
    return false;
}

inline bool ofxNdiSenderAsync(ofxNDIsender& ndiSender){
    bool didChange = false;
    bool bNdiAsync = ndiSender.GetAsync();
    if(ImGui::Checkbox("Asynchronous", &bNdiAsync)){
        ndiSender.SetAsync(bNdiAsync);
        didChange = true;
    }
    IMGUI_HELPMARKER("Disables clocked video. If enabled, best without vsync. If disabled, the render rate is clocked to the sending framerate.");
    return didChange;
}

inline bool ofxNdiSenderReadback(ofxNDIsender& ndiSender){
    bool didChange = false;
    bool bNdiRb = ndiSender.GetReadback();
    if(ImGui::Checkbox("ReadBack", &bNdiRb)){
        ndiSender.SetReadback(bNdiRb);
        didChange = true;
    }
    IMGUI_HELPMARKER("Alternative way of transfering pixel data from the GPU, performance depends on your CPU and GPU.");
    return didChange;
}

inline bool ofxNdiSenderProgressive(ofxNDIsender& ndiSender){
    bool bNdiProgressive = ndiSender.GetProgressive();
    if(ImGui::Checkbox("Progressive", &bNdiProgressive)){
        ndiSender.SetProgressive(bNdiProgressive);
        return true;
    }
    return false;
}

inline bool ofxNdiSenderClockVideo(ofxNDIsender& ndiSender){
    bool bNdiClockedVideo = ndiSender.GetClockVideo();
    if(ImGui::Checkbox("Clocked Video", &bNdiClockedVideo)){
        ndiSender.SetClockVideo(bNdiClockedVideo);
        return true;
    }
    return false;
}

inline bool ofxNdiSenderFormat(ofxNDIsender& ndiSender){
    bool didChange = false;
    auto currentCC = ofxNDIsenderFormatGui(ndiSender.GetFormat());
    if(ImGui::BeginCombo("Color Format", currentCC.name)){
        bool knownSelected = false;
        for(auto fcc : supportedFormats){
            bool selected = (fcc.code == currentCC.code);
            if(ImGui::Selectable(fcc.name, selected)){
                ndiSender.SetFormat(fcc.code);
                didChange = true;
            }
            knownSelected |= selected;
        }
        if(!knownSelected){
            ImGui::Selectable("Other / Unknown", true);
        }

        ImGui::EndCombo();
    }
    IMGUI_HELPMARKER("Not recommended to change!");
    return didChange;
}

// Draws the whole status as text
// todo: make arg const when ofxNDIsender getters are marked const.
void ofxNdiSenderStatusText(ofxNDIsender& ndiSender){
    ImGui::Text("Initialised : %s", ndiSender.SenderCreated()?"Yes":"No");
    ImGui::Text("NDI name    : %s", ndiSender.GetNDIname().c_str());
    ImGui::Text("Server name : %s", ndiSender.GetSenderName().c_str());
    static float ndiRatio; ndiSender.GetAspectRatio(ndiRatio);
    ImGui::Text("Resolution  : %u x %u (%.3f)", ndiSender.GetWidth(), ndiSender.GetHeight(), ndiRatio);

    ImGui::Text("Target FPS  : %.3f", ndiSender.GetFrameRate());
    ImGui::Text("Real FPS    : %.3f", ndiSender.GetFps());

    auto currentCC = ofxNDIsenderFormatGui(ndiSender.GetFormat());
    ImGui::Text("PixelFormat : %s\n", currentCC.name);

    const char* ndiFormatStr = "Other / Unknown";
    switch(ndiSender.GetFormat()){
        case NDIlib_FourCC_video_type_UYVY :
            ndiFormatStr = "YUV";
            break;
        case NDIlib_FourCC_video_type_RGBA :
            ndiFormatStr = "RGBA";
            break;
        default:
            break;
    }
    ImGui::Text("ofxNDI mode : %s", ndiFormatStr);
    IMGUI_HELPMARKER("The YUV codec decodes data on the GPU. Performance varies depending on your GPU.");

    ImGui::Text("Asynchronous: %s", ndiSender.GetAsync()?"Yes":"No");
    ImGui::Text("Readback    : %s", ndiSender.GetReadback()?"Yes":"No");
    ImGui::Text("Progressive : %s", ndiSender.GetProgressive()?"Yes":"No");
    ImGui::Text("ClockVideo  : %s", ndiSender.GetClockVideo()?"Yes":"No");

    ImGui::Text("NDI version : %s", ndiSender.GetNDIversion().c_str());
}

} // namespace ImGuiEx

#endif // ofxAddons_ENABLE_IMGUI
