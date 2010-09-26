//
// mgui/editor/actions.h
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

#ifndef __MGUI_EDITOR_ACTIONS_H__
#define __MGUI_EDITOR_ACTIONS_H__

#include "kit.h"

void RenderForRegion(MEditorArea& edt_area, RectListRgn& rct_lst);
void RenderForRegion(MEditorArea& edt_area, const Rect& rel_rct);

void RenderFrameDiff(MEditorArea& edt_area, int_array& old_lst);
// убрать выделение + перерисовка
void ClearRenderFrames(MEditorArea& edt_area);

Point DisplaySizeOrDef(MenuRegion* m_rgn);
Point DisplayAspectOrDef(MenuRegion* m_rgn);

inline Editor::Toolbar& MenuToolbar()
{
    return MenuEditor().Toolbar();
}

inline MenuRegion& CurMenuRegion()
{
    return MenuEditor().CurMenuRegion();
}

#endif // __MGUI_EDITOR_ACTIONS_H__


