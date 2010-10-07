//
// mgui/common_tool.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#ifndef __MGUI_COMMON_TOOL_H__
#define __MGUI_COMMON_TOOL_H__

#include <mgui/editor/const.h>

#include <mlib/tech.h> // ASSERT()

class CommonState
{
    public:

     virtual      ~CommonState() {}
};

template<typename Obj>
class ObjectState: public CommonState
{
    public:
                   /* Управление состояниями */
                   // нажали/отпустили кнопку мыши
     virtual void  OnMouseDown(Obj& obj, GdkEventButton* event) = 0;
     virtual void  OnMouseUp(Obj& obj, GdkEventButton* event) = 0;
                   // переместили мышь
     virtual void  OnMouseMove(Obj& obj, GdkEventMotion* event)  = 0;
                   // объект получил/потерял фокус
     virtual void  OnGetFocus(Obj& obj) = 0;
     virtual void  OnLeaveFocus(Obj& obj) = 0;
                   // нажали клавишу
     virtual void  OnKeyPressEvent(Obj& obj, GdkEventKey* event) = 0;
                   // является ли состояние конечным (не промежуточным),
                   // т.е. можно ли с этого состояния начать/закончить процесс/работу
                   // Пример: возможность смены инструмента проверяется с помощью IsEndState()
     virtual bool  IsEndState(Obj& obj) = 0;
};

typedef ObjectState<MEditorArea> EditorState;

//////////////////////////////////////////////////////

template<typename Obj>
bool OnButtonPressEvent(Obj& obj, ObjectState<Obj>* stt, GdkEventButton* event)
{
    if( event->type == GDK_BUTTON_PRESS )
    {
        stt->OnMouseDown(obj, event);
    }
    return false;
}

template<typename Obj>
bool OnButtonReleaseEvent(Obj& obj, ObjectState<Obj>* stt, GdkEventButton* event)
{
    ASSERT( event->type == GDK_BUTTON_RELEASE );
    stt->OnMouseUp(obj, event);
    return false;
}

// если event->is_hint, то чтобы пришло след. сообщение,
// необходимо вызвать get_pointer()
class MouseHintAdapter
{
    GdkEventMotion* event;
    public:
            MouseHintAdapter(GdkEventMotion* ev);
           ~MouseHintAdapter();
};

template<typename Obj>
bool OnMotionNotifyEvent(Obj& obj, ObjectState<Obj>* stt, GdkEventMotion* event)
{
    MouseHintAdapter mha(event);
    stt->OnMouseMove(obj, event);
    return false;
}

template<typename Obj>
bool OnFocusInEvent(Obj& obj, ObjectState<Obj>* stt, GdkEventFocus* event)
{
    ASSERT_OR_UNUSED_VAR( event->in, event );
    stt->OnGetFocus(obj);
    return false;
}

template<typename Obj>
bool OnFocusOutEvent(Obj& obj, ObjectState<Obj>* stt, GdkEventFocus* event)
{
    ASSERT_OR_UNUSED_VAR( !(event->in), event );
    stt->OnLeaveFocus(obj);
    return false;
}

template<typename Obj>
bool OnKeyPressEvent(Obj& obj, ObjectState<Obj>* stt, GdkEventKey* event)
{
    stt->OnKeyPressEvent(obj, event);
    return false;
}


#endif // __MGUI_COMMON_TOOL_H__

