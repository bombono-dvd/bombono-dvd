//
// mgui/editor/tool.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

#include <mgui/_pc_.h>

#include "tool.h"

#include "actions.h"
#include "kit.h"

#include <mgui/key.h>
#include <mgui/win_utils.h>
#include <mlib/sdk/logger.h>


void ToolState::ChangeTool(MEditorArea& edt_area, ToolState& stt)
{
    if( edt_area.toolStt )
        edt_area.toolStt->OnChange(edt_area, false);

    edt_area.MEdt::ToolData::ChangeState(&stt);

    if( edt_area.toolStt )
        edt_area.toolStt->OnChange(edt_area, true);
}

namespace MEdt {

ToolData::ToolData(): toolStt(0)
{
    //ChangeState(&SelectTool::Instance());
}

void ToolData::ChangeState(ToolState* new_stt)
{
    toolStt = new_stt;
}

} // namespace MEdt

void ToolState::OnMouseDown(MEditorArea& edt_area, GdkEventButton* event)
{
    GetSubState(edt_area)->OnMouseDown(edt_area, event);
}

void ToolState::OnMouseUp(MEditorArea& edt_area, GdkEventButton* event)
{
    GetSubState(edt_area)->OnMouseUp(edt_area, event);
}

void ToolState::OnMouseMove(MEditorArea& edt_area, GdkEventMotion* event)
{
    GetSubState(edt_area)->OnMouseMove(edt_area, event);
}

void ToolState::OnGetFocus(MEditorArea& edt_area)
{
    GetSubState(edt_area)->OnGetFocus(edt_area);
}

void ToolState::OnLeaveFocus(MEditorArea& edt_area)
{
    GetSubState(edt_area)->OnLeaveFocus(edt_area);
}

static void LogToolChange(const char* tool_name)
{
    LOG_INF << "Tool changed to: " << tool_name << io::endl;
}

void ChangeToSelectTool(MEditorArea& edt_area)
{
    ToolState::ChangeTool(edt_area, SelectTool::Instance());
    edt_area.Toolbar().selTool.set_active();

    SetCursorForWdg(edt_area);
    LogToolChange("Selection Tool (S)");
}

void ChangeToTextTool(MEditorArea& edt_area)
{
    ToolState::ChangeTool(edt_area, TextTool::Instance());
    edt_area.Toolbar().txtTool.set_active();

    Gdk::Cursor curs(Gdk::XTERM);
    SetCursorForWdg(edt_area, &curs);
    LogToolChange("Text Tool (T)");
}

void ToolState::OnKeyPressEvent(MEditorArea& edt_area, GdkEventKey* event)
{
    // см. :TODO: "keybinding"
    if( IsEndState(edt_area) && CanShiftOnly(event->state) )
    {
        // чтобы работало во всех раскладках управляем по 
        // "железным" кодам
        switch( event->hardware_keycode )
        {
        case 39: // 'S', Select Tool
            //ChangeState(edt_area, SelectTool::Instance());
            ChangeToSelectTool(edt_area);
            return;
        case 28: // 'T', Text Tool
            //ChangeState(edt_area, TextTool::Instance());
            ChangeToTextTool(edt_area);

            return;
        default:
            break;
        }
    }

    GetSubState(edt_area)->OnKeyPressEvent(edt_area, event);
}

bool ToolState::IsEndState(MEditorArea& edt_area)
{
    return GetSubState(edt_area)->IsEndState(edt_area);
}

////////////////////////////////////////////////////////////////////

EditorState* SelectTool::GetSubState(MEditorArea& edt_area)
{
    return edt_area.SelState();
}

void SelectTool::OnChange(MEditorArea& edt_area, bool is_in)
{
    if( !is_in )
    {
        // обнулим список выбранных
        //int_array lst = edt_area.SelArr();
        //edt_area.ClearSel();
        //RenderFrameDiff(edt_area, lst);
        ClearRenderFrames(edt_area);
    }
}

EditorState* TextTool::GetSubState(MEditorArea& edt_area)
{
    return edt_area.TxtState();
}

void TextTool::OnChange(MEditorArea& edt_area, bool is_in )
{
    if( !is_in )
        ForceEnd(edt_area);
}

void TextTool::ForceEnd(MEditorArea& edt_area)
{
    edt_area.TxtState()->ForceEnd(edt_area);
}

