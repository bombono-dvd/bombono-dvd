//
// mgui/timeline/layout.h
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

#ifndef __MGUI_TIMELINE_LAYOUT_H__
#define __MGUI_TIMELINE_LAYOUT_H__

#include <mdemux/util.h> 
#include <mbase/resources.h>
#include <mbase/project/media.h>
#include <mgui/win_utils.h> 
#include <mgui/design.h>

#include "select.h"
#include "monitor.h"

namespace Timeline 
{
class TLState;
}

// 
// Константы дорожки
// 

const int HEADER_WDH = 90;//85;
const int HEADER_HGT = 50;

const int TRACK_HGT = HEADER_HGT*2;

///////////////////////////////////////////////////////////////////////

//
// AppXPaned - "программный" GtkXPaned
// (т.е. для пользователя выглядит как GtkXBox, а для программиста - 
// обычный GtkXPaned)
//

// с помощью стилей уменьшаем размер разделителя до нуля
class AppHPaned: public Gtk::HPaned
{
    typedef Gtk::HPaned MyParent; 
    public:

                      AppHPaned() { set_name("AppXPaned"); }
        virtual bool  on_button_press_event(GdkEventButton* /*event*/)
                      { return false; }
        virtual void  on_realize();
        virtual void  on_state_changed(Gtk::StateType previous_state);
};

class AppVPaned: public Gtk::VPaned
{
    typedef Gtk::VPaned MyParent; 
    public:

                      AppVPaned() { set_name("AppXPaned"); }
        virtual bool  on_button_press_event(GdkEventButton* /*event*/)
                      { return false; }
        virtual void  on_realize();
        virtual void  on_state_changed(Gtk::StateType previous_state);
};

//
// SquareButton, CenteredLabel
// 

// 
// Это специальные кнопка и метка, сделанные для того, чтобы точно
// рассчитать размеры кнопок скалирования дорожек; это нужно для того,
// чтобы кнопки были правильной формы + одного размера с ползунком
// 
// Основные точки изменений - обработчики size_request и size_allocate
// (соответ. сигналы - основное средство перерасчета геометрии всей 
// конструкции виджетов). В силу того, что эти сигналы в Gtk созданы
// с опцией G_SIGNAL_RUN_FIRST единственным способом подмены обработчика
// по умолчанию является создание нового класса виджетов (просто
// добавленные обработчики к сигналу не могут вызываться раньше умолчального.)

class SquareButton: public Gtk::Button
{
    typedef Gtk::Button MyParent;
    public:

    virtual void  on_size_request(Gtk::Requisition* requisition);
    virtual void  on_size_allocate(Gtk::Allocation& all);
};

class CenteredLabel: public Gtk::Label
{
    typedef Gtk::Label MyParent;
    public:
                      CenteredLabel() {}
                      CenteredLabel(const char* str): MyParent(str) {}
            
        virtual void  on_size_allocate(Gtk::Allocation& all);
};

//
// TrackLayout
// 

//typedef Gtk::DrawingArea TrackParent;
typedef Gtk::Layout TrackParent;

// Окно монтажа (таймлиния, timeline)
class TrackLayout: public TrackParent, //public WidgetOwner,
                   public Timeline::SelectData
{
    typedef TrackParent MyParent;
    public:

                    TrackLayout(Timeline::Monitor& mon);

                    // масштаб - длина кадра в пикселях
            double  FrameScale();
                    // перевод расстояний из кадров в пикселы
            double  FramesSz(double fram_val) { return FrameScale()*fram_val; }
                    // смещение бегунков (scrollbars)
             Point  GetShift() 
                    { 
                        return Point( (int)TrkHScroll().get_value(), 
                                      (int)TrkVScroll().get_value() ); 
                    }
                    // установка позиции указателя
               int  CurPos() { return trkMon.CurPos(); }
              void  SetPos(int new_pos) { trkMon.SetPos(new_pos); }
                    // частота кадров для монтажного окна
            double  FrameFPS();
                    // длина дорожки в кадрах и пикселах
            double  GetFramesLength();
            double  GetTrackSize() { return FramesSz(GetFramesLength()); }

                    // при смене видео заново все загрузить
              void  Rebuild();
                    
   Gtk::Adjustment& TrkScale()   { return trkScale; }
   Gtk::Adjustment& TrkHScroll() { return trkHScroll; }
   Gtk::Adjustment& TrkVScroll() { return trkVScroll; }

       Gtk::HPaned& TrkHPaned() { return trkHPaned; }
       Gtk::VPaned& TrkVPaned() { return trkVPaned; }

      virtual bool  on_expose_event(GdkEventExpose* event);
      virtual void  on_size_allocate(Gtk::Allocation& allocation);
      virtual bool  on_button_press_event(GdkEventButton* event);
      virtual bool  on_button_release_event(GdkEventButton* event);
      virtual bool  on_motion_notify_event(GdkEventMotion* event);
      virtual bool  on_scroll_event(GdkEventScroll* event);
      virtual bool  on_key_press_event(GdkEventKey* event);

               int  GetGroundHeight();
               int  GetGroundOff();
 Timeline::Monitor& GetMonitor() { return trkMon; }

    protected:

      Timeline::Monitor& trkMon;
      Timeline::TLState* trkStt;

        Gtk::Adjustment  trkScale;
        Gtk::Adjustment  trkHScroll;
        Gtk::Adjustment  trkVScroll;

              AppHPaned  trkHPaned;
              AppVPaned  trkVPaned;

              void  ChangeState(Timeline::TLState* new_stt) { trkStt = new_stt; }
              friend class Timeline::TLState;

// масштабирование
void  OnFrameScale();
void  OnHScrollChange();
void  OnVScrollChange();

void  SetAdjs(Point sz);
void  HUpdate(Point sz);
void  VUpdate(Point sz);

};

// собрать дорожку в "композицию", и вложить в contr
// contr должен быть contr.has_screen() == true
void PackTrackLayout(Gtk::Container& contr, TrackLayout& layout);

void CloseTrackLayout(TrackLayout& layout);
typedef boost::function<void(TrackLayout& layout)> TLFunctor;
bool OpenTrackLayout(TrackLayout& layout, Project::VideoItem vd, TLFunctor on_open_fnr);

namespace Timeline 
{

struct time4_t
{
    int ff;
    int ss;
    int mm;
    int hh;
};

// записать в str время в формат чч:мм:сс;кк
time4_t FramesToTime(int cnt, double fps);
void FramesToTime(std::string& str, int cnt, double fps);

inline std::string CurPointerStr(TrackLayout& trk)
{
    std::string pos_str;
    FramesToTime(pos_str, trk.CurPos()>=0 ? trk.CurPos() : 0, trk.FrameFPS());
    return pos_str;
}

int TimeToFrames(double sec, double fps);
// высчитать номер кадра по часам, минутам, ...
bool TimeToFrames(long& pos, int hh, int mm, int ss, int ff, double fps);
inline long TimeToFrames(int hh, int mm, int ss, int ff, double fps)
{
    long pos;
    if( !TimeToFrames(pos, hh, mm, ss, ff, fps) )
        pos = -1;
    return pos;
}

// длина медиа в кадрах
inline double GetFramesLength(Mpeg::Player& plyr)
{
    return Mpeg::GetMediaSize(plyr) * Mpeg::GetFrameFPS(plyr);
}

} // namespace TimeLine

#endif // #ifndef __MGUI_TIMELINE_LAYOUT_H__


