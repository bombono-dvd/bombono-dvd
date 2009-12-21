//
// mgui/timeline/monitor.h
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

#ifndef __MGUI_TIMELINE_MONITOR_H__
#define __MGUI_TIMELINE_MONITOR_H__

#include <mdemux/player.h>
#include <mgui/img_utils.h>

namespace Timeline
{

class Monitor
{
    public:
                      Monitor();
             virtual ~Monitor() {}

                 int  CurPos() { return curPos; }
                void  SetPos(int new_pos)
                      {
                          curPos = new_pos;
                          OnSetPos();
                      }
        Mpeg::Player& GetPlayer() { return plyr; }
                      // заполним буфер контентом позиции frame_pos
    virtual     void  GetFrame(RefPtr<Gdk::Pixbuf> pix, int frame_pos) = 0;

    protected:

        Mpeg::FwdPlayer  plyr;
                    int  curPos; // текущее положение указателя

                      // обновление позиции монитора
    virtual     void  OnSetPos() = 0;
};

class DAMonitor: public /*Gtk::DrawingArea*/ DisplayArea, 
                 public Monitor, public VideoArea
{
    typedef Gtk::DrawingArea MyParent;
    public:
                      DAMonitor();

                      // Monitor
    virtual     void  OnSetPos();
    virtual     void  GetFrame(RefPtr<Gdk::Pixbuf> pix, int frame_pos);
                      // VideoArea
    virtual    Point  GetAspectRadio();
                      // DisplayArea
   virtual VideoArea& GetVA() { return *this; }

    protected:
                      // VideoArea
    virtual     void  DoOnConfigure(bool is_update);

void  UpdateCanvas();
};

} // namespace Timeline

#endif // #ifndef __MGUI_TIMELINE_MONITOR_H__

