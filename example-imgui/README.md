# ofxNDI + ImGui Helpers
Providing easy to use widgets for controlling ofxNDI components with ofxImGui.   
ofxImGui is an alternative GUI for ofApps.

# IMPORTANT SUPPORT NOTE !
This GUI layer is provided by contributors and is not actively maintained nor supported by the addon author.  
**Please don't ask for support for it in the ofxNDI repo** (but you can in the ofxImGui repo).  
Community pull requests are welcome for updating the codebase (the example and `ofxNDIImGuiEx.h` + `ofxNDIImGuiEx.cpp`).  

## Install
You need [the ofxImGui develop branch](https://github.com/jvcleave/ofxImGui/tree/develop).

## General Usage
Enable the ImGui widgets within any ofxAddon with the `ofxAddons_ENABLE_IMGUI` define.  
Without this define, the GUI layer and the ofxImGui dependency stay off.  
For more information, refer to [the ofxImGui repo docs](https://github.com/jvcleave/ofxImGui/blob/develop/Developers.md#ofxaddons-with-custom-ofximgui-widgets).  

## Example Usage
This folder (`example-imgui`) shows how to use the provided widgets.  
Compile the code and learn how to implement the ofxNDI gui widgets into your ofApp.  
