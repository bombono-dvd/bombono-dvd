//
// mgui/timeline/select.h
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

#ifndef __MGUI_TIMELINE_SELECT_H__
#define __MGUI_TIMELINE_SELECT_H__

#include <mgui/common_tool.h>

class TrackLayout;

namespace Timeline
{

class TLState: public ::ObjectState<TrackLayout>
{
    public:

        virtual void  OnGetFocus(TrackLayout& ) {}
        virtual void  OnLeaveFocus(TrackLayout&) {}
        virtual void  OnKeyPressEvent(TrackLayout&, GdkEventKey*) {}
        virtual bool  IsEndState(TrackLayout&) { return true; }

                void  ChangeTo(TrackLayout& trk) { ChangeState(trk, *this); }
    protected:

                void  ChangeState(TrackLayout& trk, TLState& stt);
};

// таймлиния закрыта
class ClosedTL: public TLState, public Singleton<ClosedTL>
{
    public:

        virtual void  OnMouseDown(TrackLayout&, GdkEventButton*) {}
        virtual void  OnMouseUp(TrackLayout&, GdkEventButton*) {}
        virtual void  OnMouseMove(TrackLayout&, GdkEventMotion*) {}
};


class NormalTL: public TLState, public Singleton<NormalTL>
{
    public:

        virtual void  OnMouseDown(TrackLayout& trk, GdkEventButton* event);
        virtual void  OnMouseUp(TrackLayout& /*trk*/, GdkEventButton* /*event*/) {}
        virtual void  OnMouseMove(TrackLayout& trk, GdkEventMotion* event);
        virtual void  OnKeyPressEvent(TrackLayout& trk, GdkEventKey* event);
};

class ActionTL: public TLState
{
    public:

                      // начать/перейти в состояние
        virtual void  BeginState(TrackLayout& /*trk*/) = 0;

        virtual void  OnMouseDown(TrackLayout& /*trk*/, GdkEventButton* /*event*/) {}
        virtual void  OnMouseMove(TrackLayout& /*trk*/, GdkEventMotion* /*event*/) {}
};

class MoverTL: public ActionTL
{
    public:

        struct Data
        {
            DPoint  phisCoord; // координаты в момент нажатия
            DPoint  userCoord;
               int  startPos;

              bool  atPointer;
               int  dvdIdx;
        };

                void  SetCursorPos(TrackLayout& trk, const DPoint& phis_pos, const DPoint& user_pos);
                 int  CalcFrame(double pos_x, TrackLayout& trk);


        virtual void  OnMouseUp(TrackLayout& trk, GdkEventButton* event);

    protected:

        virtual void  OnDragEnd(TrackLayout& /*trk*/) {}
};

class PointerMoverTL: public MoverTL, public Singleton<PointerMoverTL>
{
    public:

        virtual void  OnMouseMove(TrackLayout& trk, GdkEventMotion* event);
        virtual void  BeginState(TrackLayout& trk);
};

class DVDLabelMoverTL: public MoverTL, public Singleton<DVDLabelMoverTL>
{
    public:

                void  SetDVDIdx(TrackLayout& trk, int dvdIdx);

        virtual void  OnMouseMove(TrackLayout& trk, GdkEventMotion* event);
        virtual void  BeginState(TrackLayout& trk);
        virtual void  OnDragEnd(TrackLayout& trk);
};

class EditBigLabelTL: public ActionTL, public Singleton<EditBigLabelTL>
{
    public:

        struct Data
        {
            RefPtr<Gtk::Entry> edt;
        };
            

        virtual void  OnMouseDown(TrackLayout& trk, GdkEventButton* event);
        virtual void  OnMouseUp(TrackLayout& /*trk*/, GdkEventButton* /*event*/) {}

        virtual void  BeginState(TrackLayout& );
                void  EndState(TrackLayout& trk, bool accept);
};

// разбираем "hh:mm:ss;ff" в номер кадра
bool ParsePointerPos(long& pos, const char* str, double fps);

Rect GetBigLabelLocation(TrackLayout& trk);
bool SetPointer(int new_pos, TrackLayout& trk);
void InsertDVDMark(TrackLayout& trk_lay);
void RedrawDVDMark(TrackLayout& trk, int idx);

//
// Timeline::SelectData
// 

class SelectData: public MoverTL::Data,
                  public EditBigLabelTL::Data
{
};


} // namespace Timeline

#endif // #ifndef __MGUI_TIMELINE_SELECT_H__

