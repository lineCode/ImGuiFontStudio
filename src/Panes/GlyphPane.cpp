// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "GlyphPane.h"

#include "MainFrame.h"

#include "Gui/GuiLayout.h"
#include "Gui/ImGuiWidgets.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include <cTools.h>
#include <FileHelper.h>

#include <cinttypes> // printf zu

#include "sfntly/tag.h"
#include "sfntly/font.h"
#include "sfntly/font_factory.h"
#include "sfntly/data/memory_byte_array.h"
#include "sfntly/port/memory_output_stream.h"
#include "sfntly/port/file_input_stream.h"
#include "sfntly/table/truetype/loca_table.h"
#include "sfntly/table/core/cmap_table.h"
#include "sfntly/table/core/maximum_profile_table.h"
#include "sfntly/table/core/post_script_table.h"
#include "sfntly/table/core/horizontal_header_table.h"
#include "sfntly/table/core/horizontal_metrics_table.h"
#include "sfntly/port/type.h"
#include "sfntly/port/refcount.h"

static int GlyphPane_WidgetId = 0;

GlyphPane::GlyphPane()
{
	
}

GlyphPane::~GlyphPane()
{
	
}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

static float _progress = 1.0f;
static float _scale = 0.5f;
static bool _stroke = true;
static bool _controLines = true;

int GlyphPane::DrawGlyphPane(ProjectFile *vProjectFile, int vWidgetId)
{
	GlyphPane_WidgetId = vWidgetId;

	if (GuiLayout::m_Pane_Shown & PaneFlags::PANE_GLYPH)
	{
		if (ImGui::Begin<PaneFlags>(GLYPH_PANE,
			&GuiLayout::m_Pane_Shown, PaneFlags::PANE_GLYPH,
			//ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_MenuBar |
			//ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			//ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoBringToFrontOnFocus))
		{
			if (vProjectFile && vProjectFile->IsLoaded())
			{
				if (ImGui::BeginMenuBar())
				{
					ImGui::PushItemWidth(100.0f);
					ImGui::SliderFloat("Scale", &_scale, 0.01f, 1.0f);
					ImGui::PopItemWidth();

					ImGui::Checkbox("Stroke or Fill", &_stroke);
					ImGui::Checkbox("Control Lines", &_controLines);

					ImGui::EndMenuBar();
				}

				DrawSimpleGlyph(m_Glyph, vProjectFile->m_CurrentFont, _scale, _progress, _stroke, _controLines);
			}
		}

		ImGui::End();
	}

	return GlyphPane_WidgetId;
}

///////////////////////////////////////////////////////////////////////////////////
//// LOAD GLYPH ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// https://github.com/rillig/sfntly/tree/master/java/src/com/google/typography/font/tools/fontviewer

static int limitContour = 0;

bool GlyphPane::LoadGlyph(ProjectFile *vProjectFile, FontInfos* vFontInfos, GlyphInfos *vGlyphInfos)
{
	bool res = false;

	if (vFontInfos && vGlyphInfos)
	{
		std::string fontPathName = vProjectFile->GetAbsolutePath(vFontInfos->m_FontFilePathName);

		if (FileHelper::Instance()->IsFileExist(fontPathName))
		{
			FontHelper m_FontHelper;

			m_fontInstance.m_Font.Attach(m_FontHelper.LoadFontFile(fontPathName.c_str()));
			if (m_fontInstance.m_Font)
			{
				sfntly::Ptr<sfntly::CMapTable> cmap_table = down_cast<sfntly::CMapTable*>(m_fontInstance.m_Font->GetTable(sfntly::Tag::cmap));
				m_fontInstance.m_CMapTable.Attach(cmap_table->GetCMap(sfntly::CMapTable::WINDOWS_BMP));
				if (m_fontInstance.m_CMapTable)
				{
					m_fontInstance.m_GlyfTable = down_cast<sfntly::GlyphTable*>(m_fontInstance.m_Font->GetTable(sfntly::Tag::glyf));
					m_fontInstance.m_LocaTable = down_cast<sfntly::LocaTable*>(m_fontInstance.m_Font->GetTable(sfntly::Tag::loca));

					if (m_fontInstance.m_GlyfTable && m_fontInstance.m_LocaTable)
					{
						int codePoint = vGlyphInfos->glyph.Codepoint;
						int32_t glyphId = m_fontInstance.m_CMapTable->GlyphId(codePoint);
						int32_t length = m_fontInstance.m_LocaTable->GlyphLength(glyphId);
						int32_t offset = m_fontInstance.m_LocaTable->GlyphOffset(glyphId);

						// Get the GLYF table for the current glyph id.
						auto g = m_fontInstance.m_GlyfTable->GetGlyph(offset, length);
						
						if (g->GlyphType() == sfntly::GlyphType::kSimple)
						{
							auto glyph = down_cast<sfntly::GlyphTable::SimpleGlyph*>(g);
							if (glyph)
							{
								vGlyphInfos->simpleGlyph.LoadSimpleGlyph(glyph);
								limitContour = vGlyphInfos->simpleGlyph.GetCountContours();
								m_Glyph = vGlyphInfos;
								res = true;
							}
						}
					}
				}
			}
		}
	}

	return res;
}

// https://github.com/rillig/sfntly/tree/master/java/src/com/google/typography/font/tools/fontviewer
bool GlyphPane::DrawSimpleGlyph(GlyphInfos *vGlyph, FontInfos* vFontInfos,
	double vScale, double vProgress, bool vFill, bool vControlLines)
{
	if (vGlyph && vFontInfos)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		auto drawList = window->DrawList;

		/////////////////////////////////////////////////

		auto g = &vGlyph->simpleGlyph;
		if (g->isValid)
		{
			int cmax = (int)g->coords.size();
			ct::ivec4 rc = g->rc;

			ImVec2 contentSize = ImGui::GetContentRegionMax();
			ImRect glypRect = ImRect(
				rc.x * vScale, rc.y * vScale,
				rc.z * vScale, rc.w * vScale);
			ImVec2 glyphCenter = glypRect.GetCenter();
			ImVec2 pos = ImGui::GetCursorScreenPos() + contentSize * 0.5f - glyphCenter;

			if (ImGui::BeginMenuBar())
			{
				ImGui::PushItemWidth(100.0f);
				ImGui::SliderInt("Contours", &limitContour, 0, cmax);
				ImGui::PopItemWidth();

				ImGui::EndMenuBar();
			}

			// x 0
			drawList->AddLine(
				ct::toImVec2(g->Scale(ct::ivec2(0, rc.y), vScale)) + pos,
				ct::toImVec2(g->Scale(ct::ivec2(0, rc.w), vScale)) + pos,
				ImGui::GetColorU32(ImVec4(0, 0, 1, 1)), 2.0f);

			// Ascent
			drawList->AddLine(
				ct::toImVec2(g->Scale(ct::ivec2(rc.x, vFontInfos->m_Ascent), vScale)) + pos,
				ct::toImVec2(g->Scale(ct::ivec2(rc.z, vFontInfos->m_Ascent), vScale)) + pos,
				ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), 2.0f);

			// y 0
			drawList->AddLine(
				ct::toImVec2(g->Scale(ct::ivec2(rc.x, 0), vScale)) + pos,
				ct::toImVec2(g->Scale(ct::ivec2(rc.z, 0), vScale)) + pos,
				ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), 1.0f);

			// Descent
			drawList->AddLine(
				ct::toImVec2(g->Scale(ct::ivec2(rc.x, vFontInfos->m_Descent), vScale)) + pos,
				ct::toImVec2(g->Scale(ct::ivec2(rc.z, vFontInfos->m_Descent), vScale)) + pos,
				ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), 2.0f);

			for (int c = 0; c < cmax; c++)
			{
				if (c >= limitContour) break;

				int pmax = (int)g->coords[c].size();

				int firstOn = 0;
				for (int p = 0; p < pmax; p++)
				{
					if (g->IsOnCurve(c, p))
					{
						firstOn = p;
						break;
					}
				}

				// curve

				drawList->PathLineTo(ct::toImVec2(g->GetCoords(c, firstOn, vScale)) + pos);

				for (int i = 0; i < pmax; i++)
				{
					int icurr = firstOn + i + 1;
					int inext = firstOn + i + 2;
					ct::ivec2 cur = g->GetCoords(c, icurr, vScale);
					if (g->IsOnCurve(c, icurr))
					{
						drawList->PathLineTo(ct::toImVec2(cur) + pos);
					}
					else
					{
						ct::ivec2 nex = g->GetCoords(c, inext, vScale);
						if (!g->IsOnCurve(c, inext))
						{
							nex.x = (int)(((double)nex.x + (double)cur.x) * 0.5);
							nex.y = (int)(((double)nex.y + (double)cur.y) * 0.5);
						}
						drawList->PathQuadCurveTo(
							ct::toImVec2(cur) + pos,
							ct::toImVec2(nex) + pos, 20);
					}
				}

				if (_stroke)
				{
					drawList->PathStroke(ImGui::GetColorU32(ImGuiCol_Text), true);
				}
				else
				{
					drawList->PathFillConvex(ImGui::GetColorU32(ImGuiCol_Text));
				}

				// control lines
				if (vControlLines)
				{
					drawList->PathLineTo(ct::toImVec2(g->GetCoords(c, firstOn, vScale)) + pos);

					for (int i = 0; i < pmax; i++)
					{
						int icurr = firstOn + i + 1;
						int inext = firstOn + i + 2;
						ct::ivec2 cur = g->GetCoords(c, icurr, vScale);
						if (g->IsOnCurve(c, icurr))
						{
							drawList->PathLineTo(ct::toImVec2(cur) + pos);
						}
						else
						{
							ct::ivec2 nex = g->GetCoords(c, inext, vScale);
							if (!g->IsOnCurve(c, inext))
							{
								nex.x = (int)(((double)nex.x + (double)cur.x) * 0.5);
								nex.y = (int)(((double)nex.y + (double)cur.y) * 0.5); 
							}
							drawList->PathLineTo(ct::toImVec2(cur) + pos);
							drawList->PathLineTo(ct::toImVec2(nex) + pos);
						}
					}

					drawList->PathStroke(ImGui::GetColorU32(ImVec4(0, 0, 1, 1)), true);
				}
			}
		}
	}

	return true;
}