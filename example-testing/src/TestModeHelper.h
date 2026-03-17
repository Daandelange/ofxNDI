/*

	TestModeHelper - Utility to manage "app states".

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

	xx.xx.26 - Initial TestModeHelper

*/
#pragma once

// TestModes Helper
#include <functional>
#include <string>
#include <map>
#include <cstddef>
#include "GuiLayoutHelper.h"

using RenderCallback = std::function<void(GuiLayoutHelper&)>;
struct TestModesEntry {
	std::string   title;
	RenderCallback cb;
};
static const TestModesEntry dummyEntry = { "- undefined -", [](GuiLayoutHelper&){}};
template<typename MODES_ENUM>
class TestModeHelper {
public:
    static_assert(std::is_enum<MODES_ENUM>::value, "MODES_ENUM must be an enum type");

    void registerTest(MODES_ENUM mode, const std::string& title, RenderCallback callback) {
        entries[mode] = TestModesEntry{ title, callback };
    }

    void setCurrentMode(MODES_ENUM mode) {
        if(entries.find(mode) == entries.end()){
            //std::cout << "Invalid mode = " << mode << std::endl;
            return;
        }
        currentMode = mode;
    }
    void incrementMode(int direction) {
        if(direction==0) return;
        auto it = entries.find(currentMode);
        if (it != entries.end()) {
            if(direction>0){
                it++;
                if (it == entries.end()) {
                    it = entries.begin();
                }
                currentMode = it->first;
            }
            else {
                if (it == entries.begin()) {
                    it = entries.end();
                }
                --it;
                currentMode = it->first;
            }
        }
    }

    MODES_ENUM getCurrentMode() const {
        return currentMode;
    }

    bool isMode(MODES_ENUM mode) const {
       return currentMode == mode;
    }

    const TestModesEntry& getCurrentTest() const {
        auto it = entries.find(currentMode);
        if (it != entries.end()) {
            return it->second;
        }
        return dummyEntry;
    }

    const std::string& getCurrentTitle() const {
        return getCurrentTest().title;
    }

    void render(GuiLayoutHelper& gui) {
        getCurrentTest().cb(gui);
    }

private:
    std::map<MODES_ENUM, TestModesEntry> entries;
    MODES_ENUM currentMode;
};
