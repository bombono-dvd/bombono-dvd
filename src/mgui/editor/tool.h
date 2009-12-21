//
// mgui/editor/tool.h
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

#ifndef __MGUI_EDITOR_TOOL_H__
#define __MGUI_EDITOR_TOOL_H__

#include "select.h"
#include "txtool.h"

class ToolState: public EditorState
{
    public:

        virtual void  OnMouseDown(MEditorArea& edt_area, GdkEventButton* event);
        virtual void  OnMouseUp(MEditorArea& edt_area, GdkEventButton* event);
        virtual void  OnMouseMove(MEditorArea& edt_area, GdkEventMotion* event);
        virtual void  OnGetFocus(MEditorArea& edt_area);
        virtual void  OnLeaveFocus(MEditorArea& edt_area);
        virtual void  OnKeyPressEvent(MEditorArea& edt_area, GdkEventKey* event);
        virtual bool  IsEndState(MEditorArea& edt_area);

                      // смена инструмента
        static  void  ChangeTool(MEditorArea& edt_area, ToolState& stt);

                      // закончить промежуточное состояние, если находимся в таковом
                      // (например, при загрузке другого меню)
        virtual void  ForceEnd(MEditorArea& edt_area) = 0;

                      // вызов при смене инструмента
        virtual void  OnChange(MEditorArea& edt_area, bool is_in) = 0;

    protected:

                          // получить состояние самого инструмента
    virtual  EditorState* GetSubState(MEditorArea& edt_area) = 0;
};

namespace MEdt {

class ToolData: public SelectData,
                public TextData
{
    public:
              ToolData();

        void  ChangeState(ToolState* stt);

    protected:
        ToolState* toolStt;
        friend class ::ToolState;
};

} // namespace MEdt

////////////////////////////////////////////////////////////////////

class SelectTool: public ToolState, public Singleton<SelectTool>
{
    public:

     virtual         void  OnChange(MEditorArea& edt_area, bool is_in);
     virtual         void  ForceEnd(MEditorArea&) { }
    protected:
     virtual  EditorState* GetSubState(MEditorArea& edt_area);
};

class TextTool: public ToolState, public Singleton<TextTool>
{
    public:

     virtual         void  OnChange(MEditorArea& edt_area, bool is_in);
     virtual         void  ForceEnd(MEditorArea&);
    protected:
     virtual  EditorState* GetSubState(MEditorArea& edt_area);
};

#endif // __MGUI_EDITOR_TOOL_H__

