//
// mgui/editor/txtool.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#ifndef __MGUI_EDITOR_TXTOOL_H__
#define __MGUI_EDITOR_TXTOOL_H__

#include <mgui/common_tool.h>

#include <mlib/patterns.h> // Singleton

class TextState: public EditorState
{
    public:

     virtual void  OnMouseUp(MEditorArea&, GdkEventButton*) { }
     virtual void  OnMouseMove(MEditorArea&, GdkEventMotion*) { }
     
                   // см. ToolState
     virtual void  ForceEnd(MEditorArea&) = 0;

     protected:

             void  ChangeState(MEditorArea& edt_area, TextState& stt);
};

class NormalText: public TextState, public Singleton<NormalText>
{
    public:

        virtual void  OnMouseDown(MEditorArea& edt_area, GdkEventButton* event);
        virtual void  OnGetFocus(MEditorArea&) { }
        virtual void  OnLeaveFocus(MEditorArea&) { }
        virtual void  OnKeyPressEvent(MEditorArea&, GdkEventKey*) { }
        virtual bool  IsEndState(MEditorArea&) { return true; }
        virtual void  ForceEnd(MEditorArea&) { }
};

class EdtTextRenderer;

class EditText: public TextState, public Singleton<EditText>
{
    public:
                struct Data
                {
                    EdtTextRenderer* edtTxt;
                };

        virtual void  OnMouseDown(MEditorArea& edt_area, GdkEventButton* event);
        virtual void  OnGetFocus(MEditorArea& edt_area);
        virtual void  OnLeaveFocus(MEditorArea& edt_area);
        virtual void  OnKeyPressEvent(MEditorArea& edt_area, GdkEventKey* event);
        virtual bool  IsEndState(MEditorArea&);
        virtual void  ForceEnd(MEditorArea& edt_area) { End(edt_area); }

                void  Init(EdtTextRenderer* txt, GdkEventButton* event, MEditorArea& edt_area,
                           bool reinit = false);
                void  End(MEditorArea& edt_area);
     EdtTextRenderer* GetTextRenderer(MEditorArea& edt_area);
};


namespace MEdt {

class TextData: public EditText::Data
{
    public:
                TextData();

     TextState* TxtState() { return txtStt; }

    private:

        TextState* txtStt; // состояние редактора (выделение/движение)

           void  ChangeState(TextState* new_stt);
           friend class ::TextState;
};

} // namespace MEdt

#endif // __MGUI_EDITOR_TXTOOL_H__

