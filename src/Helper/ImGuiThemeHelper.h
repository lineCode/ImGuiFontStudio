/*
 * Copyright 2020 Stephane Cuillerdier (aka Aiekick)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <ConfigAbstract.h>
#include <imgui.h>
#include <string>
#include <map>

class ImGuiThemeHelper : public conf::ConfigAbstract
{
private:
	std::map<std::string, ImVec4> m_FileTypeColors;

public:
	void DrawMenu();

public:
	std::string getXml(const std::string& vOffset);
	void setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent);

public:
	void ApplyStyleColorsDefault(ImGuiStyle* dst = 0);
	void ApplyStyleColorsClassic(ImGuiStyle* dst = 0);
	void ApplyStyleColorsDark(ImGuiStyle* dst = 0);
	void ApplyStyleColorsLight(ImGuiStyle* dst = 0);
	void ApplyStyleColorsDarcula(ImGuiStyle* dst = 0);

private:
	void ApplyFileTypeColors();

private:
	std::string GetStyleColorName(ImGuiCol idx);
	int GetImGuiColFromName(const std::string& vName);

public: // singleton
	static ImGuiThemeHelper *Instance()
	{
		static ImGuiThemeHelper *_instance = new ImGuiThemeHelper();
		return _instance;
	}

protected:
	ImGuiThemeHelper(); // Prevent construction
	ImGuiThemeHelper(const ImGuiThemeHelper&) {}; // Prevent construction by copying
	ImGuiThemeHelper& operator =(const ImGuiThemeHelper&) { return *this; }; // Prevent assignment
	~ImGuiThemeHelper(); // Prevent unwanted destruction
};

