//
// mgui/editor/select.h
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

#ifndef __MGUI_EDITOR_SELECT_H__
#define __MGUI_EDITOR_SELECT_H__

#include <mgui/common_tool.h>

#include <mlib/ptr.h>
#include <mlib/patterns.h> // Singleton
#include <mlib/geom2d.h>

class SelectState: public EditorState
{
    public:
     virtual void  OnGetFocus(MEditorArea&) { }
     virtual void  OnLeaveFocus(MEditorArea&) { }
     virtual void  OnKeyPressEvent(MEditorArea&, GdkEventKey*) { }
    protected:
             void  ChangeState(MEditorArea& edt_area, SelectState& stt);
};

// форма действия (курсора) в режиме выделения
enum SelActionType
{
    sctNORMAL,  // обычный курсор
    sctMOVE,    // курсор-передвижение

    // изменение размеров
    sctRESIZE_LEFT,     // по сторонам
    sctRESIZE_RIGHT,
    sctRESIZE_TOP,
    sctRESIZE_BTM,

    sctRESIZE_LT,       // left-top
    sctRESIZE_RT,
    sctRESIZE_RB,
    sctRESIZE_LB
};

class PressState;

// основное состояние 
class NormalSelect: public SelectState, public Singleton<NormalSelect>
{
    public:

        struct Data
        {
            ptr::one<PressState> prssStt; // левая кнопка мыши
                          Point  msCoord; // координаты в момент нажатия
                  SelActionType  curTyp;
                            int  curPos;  // над каким объектом
                           bool  isAdd;   // выбор в формате добавления

                    Data(): isAdd(false) {}
        };

        virtual void  OnMouseDown(MEditorArea& edt_area, GdkEventButton* event);
        virtual void  OnMouseUp(MEditorArea& edt_area, GdkEventButton* event);
        virtual void  OnMouseMove(MEditorArea& edt_area, GdkEventMotion* event);
        virtual void  OnKeyPressEvent(MEditorArea& edt_area, GdkEventKey* event);
        virtual bool  IsEndState(MEditorArea&);

                void  ClearPress(MEditorArea& edt_area);
};

// класс - стратегия при нажатии на кнопку (в NormalSelect)
class PressState: CommonState
{
    public:

        virtual void  OnPressDown(MEditorArea& edt_area, NormalSelect::Data& dat) = 0;
        virtual void  OnPressUp(MEditorArea& edt_area, NormalSelect::Data& dat) = 0;
};

class MovePress: public PressState
{
    public:

        virtual void  OnPressDown(MEditorArea&, NormalSelect::Data&) { }
        virtual void  OnPressUp(MEditorArea& edt_area, NormalSelect::Data& dat);
};

class SelectPress: public PressState
{
    public:

        virtual void  OnPressDown(MEditorArea& edt_area, NormalSelect::Data& dat);
        virtual void  OnPressUp(MEditorArea&, NormalSelect::Data&) { }
};

// общая функциональность "двигательных" состояний
class MoveSelect: public SelectState
{
    public:

        struct Data
        {
                    Point  origCoord; // координаты в момент нажатия
            SelActionType  curTyp;

                    Data(): curTyp(sctNORMAL) {}
        };

                      // установить начальные данные
                void  InitData(const NormalSelect::Data& dat, MEditorArea& edt_area);

        virtual void  OnMouseDown(MEditorArea& edt_area, GdkEventButton* event);
        virtual void  OnMouseUp(MEditorArea& edt_area, GdkEventButton* event);
        virtual bool  IsEndState(MEditorArea&) { return false; }

    protected:

        virtual void  InitDataExt(const NormalSelect::Data&, MEditorArea&) { }
};

// состояние - в процессе перемещения объекта(ов)
class RepositionSelect: public MoveSelect, public Singleton<RepositionSelect>
{
    public:

        virtual void  OnMouseMove(MEditorArea& edt_area, GdkEventMotion* event);
};

// состояние - в процессе изменения размеров объекта(ов)
class ResizeSelect: public MoveSelect, public Singleton<ResizeSelect>
{
    public:

        virtual void  InitDataExt(const NormalSelect::Data& dat, MEditorArea& edt_area);
        virtual void  OnMouseMove(MEditorArea& edt_area, GdkEventMotion* event);
};

class NothingSelect: public MoveSelect, public Singleton<NothingSelect>
{
        virtual void  OnMouseMove(MEditorArea&, GdkEventMotion*) { }
};

//////////////////////////////////////////////////////////
// Инструмент выбора (Selection Tool)


namespace MEdt {

class SelectData: public NormalSelect::Data,
                  public MoveSelect::Data
{
    public:
                SelectData();

   SelectState* SelState() { return selStt; }

    private:

        SelectState* selStt; // состояние редактора (выделение/движение)

           void  ChangeState(SelectState* new_stt);
           friend class ::SelectState;
};

} // namespace MEdt

#endif // __MGUI_EDITOR_SELECT_H__

