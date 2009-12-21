//
// mgui/editor/txtool.cpp
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

#include <mgui/_pc_.h>

#include "txtool.h"

#include "text.h"
#include "kit.h"
#include "toolbar.h"

#include <mgui/key.h>
#include <mlib/string.h>

void TextState::ChangeState(MEditorArea& edt_area, TextState& stt)
{
    edt_area.MEdt::TextData::ChangeState(&stt);
}

namespace MEdt {

TextData::TextData(): txtStt(0)
{
    // начальное состояние
    ChangeState(&NormalText::Instance());
}

void TextData::ChangeState(TextState* new_stt)
{
    txtStt = new_stt;
}

} // namespace MEdt

class TextFindVis: public SelVis
{
    typedef SelVis MyParent;
    public:
                       TextFindVis(int x, int y): MyParent(x, y), txtRes(0) {}

      EdtTextRenderer* TxtRes() { return txtRes; }

        virtual  void  Visit(TextObj& t_obj);
        virtual  void  Visit(FrameThemeObj& fto);
    protected:
        EdtTextRenderer* txtRes;
};

void TextFindVis::Visit(TextObj& t_obj)
{
    MyParent::VisitMediaObj(t_obj);
    if( selPos == objPos )
        txtRes = &t_obj.GetData<EdtTextRenderer>();
}

void TextFindVis::Visit(FrameThemeObj& fto)
{
    MyParent::VisitMediaObj(fto);
    if( selPos == objPos )
        txtRes = 0;
}

void ReStyle(TextObj& t_obj, const Editor::TextStyle& style)
{
    t_obj.SetStyle(style);
    t_obj.CalcDimensions(true);
}

static EdtTextRenderer* InsertNewText(MEditorArea& edt_area, GdkEventButton* event)
{
    EdtTextRenderer* edt_txt = 0;

    Point lct((int)event->x, (int)event->y);
    if( edt_area.FramePlacement().Contains(lct) )
    {
        // создаем новый
        Planed::Transition tr = edt_area.Transition();
        lct = tr.RelToAbs(tr.DevToRel(lct));

        // 1 создаем
        TextObj* t_obj = new TextObj;
    	// стиль
        ReStyle(*t_obj, edt_area.Toolbar().GetFontDesc());
        t_obj->SetLocation(lct);
        //edt_area.CurMenuRegion().Ins(*t_obj);
        Project::AddMenuItem(edt_area.CurMenuRegion(), t_obj);

        // 2 инициализируем
        edt_txt = &t_obj->GetData<EdtTextRenderer>();
        edt_txt->Init(edt_area.Canvas());
        edt_txt->SetEditor(&edt_area);
        edt_txt->DoLayout();
    }
    return edt_txt;
}

static void SetDownText(EdtTextRenderer* edt_txt, MEditorArea& edt_area)
{
    edt_txt->ShowCursor(false);

    // удаление пустых (перерисовка не нужна - все пусто)
    TextObj* t_obj = &edt_txt->GetTextObj();
    if( t_obj->Text().size() == 0 )
        edt_area.CurMenuRegion().Clear(t_obj);
}

void NormalText::OnMouseDown(MEditorArea& edt_area, GdkEventButton* event)
{
    if( !IsLeftButton(event) )
        return;

    Point lct((int)event->x, (int)event->y);

    TextFindVis tfv(lct.x, lct.y);
    edt_area.CurMenuRegion().Accept(tfv);
    EdtTextRenderer* edt_txt = tfv.TxtRes();
    if( !edt_txt )
        edt_txt = InsertNewText(edt_area, event);

    if( edt_txt )
    {
        EditText& stt = EditText::Instance();
        stt.Init(edt_txt, event, edt_area);
        ChangeState(edt_area, stt);
    }
}

void EditText::Init(EdtTextRenderer* txt, GdkEventButton* event, MEditorArea& edt_area,
                    bool reinit)
{
    Data& dat = edt_area;
    if( reinit )
        SetDownText(dat.edtTxt, edt_area);
    dat.edtTxt = txt;

    txt->ShowCursor(true, true);
    txt->OnButtonPressEvent(event);
}

EdtTextRenderer* EditText::GetTextRenderer(MEditorArea& edt_area)
{
    Data& dat = edt_area;
    ASSERT( dat.edtTxt );
    return dat.edtTxt;
}

void EditText::End(MEditorArea& edt_area)
{
    Data& dat = edt_area;
    ASSERT( dat.edtTxt );
    SetDownText(dat.edtTxt, edt_area);

    dat.edtTxt = 0;
    ChangeState(edt_area, NormalText::Instance());
}

void EditText::OnMouseDown(MEditorArea& edt_area, GdkEventButton* event)
{
    if( IsLeftButton(event) )
    {
        TextFindVis tfv((int)event->x, (int)event->y);
        edt_area.CurMenuRegion().Accept(tfv);
        if( EdtTextRenderer* txt = tfv.TxtRes() )
        {
            EdtTextRenderer* rndr = GetTextRenderer(edt_area);
            if( txt == rndr )
                rndr->OnButtonPressEvent(event);
            else
                Init(txt, event, edt_area, true);
        }
        else
        {
            if( EdtTextRenderer* new_txt = InsertNewText(edt_area, event) )
                Init(new_txt, event, edt_area, true);
            else
                End(edt_area);
        }
    }
}

void EditText::OnGetFocus(MEditorArea& edt_area)
{
    GetTextRenderer(edt_area)->OnFocusInEvent();
}

void EditText::OnLeaveFocus(MEditorArea& edt_area)
{
    GetTextRenderer(edt_area)->OnFocusOutEvent();
}

void EditText::OnKeyPressEvent(MEditorArea& edt_area, GdkEventKey* event)
{
    if( event->keyval == GDK_Escape )
    {
        End(edt_area);
        return;
    }

    GetTextRenderer(edt_area)->OnKeyPressEvent(event);
}

bool EditText::IsEndState(MEditorArea&)
{
    // :TODO: так как пока переход между инструментами 
    // возможен только с клавы, то нельзя
    // ( когда будет панель инструментов сделать true и 
    // на OnChange() делать End() )
    return false;
}

