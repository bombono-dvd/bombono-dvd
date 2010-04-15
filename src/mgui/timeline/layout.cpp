//
// mgui/timeline/layout.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2010 Ilya Murav'jov
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

#include "layout.h"
#include "select.h"
#include "service.h"

#include <mgui/img-factory.h>
#include <mgui/project/handler.h>
#include <mgui/sdk/widget.h>
#include <mgui/sdk/packing.h>
#include <mgui/gettext.h>

#include <mlib/sigc.h> 

#include <gtk/gtkpaned.h>

#include <cmath> // pow()

using namespace Timeline;

// 
// AppXPaned
// 

void AppHPaned::on_realize()
{
    set_sensitive(false);
    MyParent::on_realize();
    set_sensitive(true);
}

void AppHPaned::on_state_changed(Gtk::StateType previous_state)
{
    MyParent::on_state_changed(previous_state);
    if( is_realized() && is_sensitive() )
    {
        Gtk::Paned* th = (Gtk::Paned*)this;
        gdk_window_set_cursor(th->gobj()->handle, NULL);
    }
}

void AppVPaned::on_realize()
{
    set_sensitive(false);
    MyParent::on_realize();
    set_sensitive(true);
}

void AppVPaned::on_state_changed(Gtk::StateType previous_state)
{
    MyParent::on_state_changed(previous_state);
    if( is_realized() && is_sensitive() )
    {
        Gtk::Paned* th = (Gtk::Paned*)this;
        gdk_window_set_cursor(th->gobj()->handle, NULL);
    }
}

//
// SquareButton, CenteredLabel
// 

void SquareButton::on_size_request(Gtk::Requisition* req)
{
    MyParent::on_size_request(req);
    // так как вложенный виджет (метка) требует слишком большую высоту,
    // уменьшим ее после всех расчетов до ширины (квадрат)
    req->height = req->width;
}

void SquareButton::on_size_allocate(Gtk::Allocation& all)
{
    // тут наоборот, надо перед тем, как кнопка непосредственно получит
    // данные о размещении, подправить размещение:
    // - делаем квадратом
    // - центрируем по высоте
    int diff = all.get_height() - all.get_width();
    int y = all.get_y() + diff/2;
    int w = all.get_width();
    all.set_y(y);
    all.set_height(w);

//     int diff = all.get_height() - all.get_width();
//     int x = all.get_x() - diff/2;
//     int h = all.get_height();
//     all.set_x(x);
//     all.set_width(h);

    MyParent::on_size_allocate(all);
}

void CenteredLabel::on_size_allocate(Gtk::Allocation& all)
{
    // общая цель - чтоб текст был по центру
    Gtk::Requisition req = get_requisition();
    // изменяем all относительно req
    // x
    int diff_x = req.width - all.get_width();
    all.set_x( all.get_x() - diff_x/2 );
    all.set_width(req.width);
    // y
    int diff_y = req.height - all.get_height();
    all.set_y( all.get_y() - diff_y/2 );
    all.set_height(req.height);

    MyParent::on_size_allocate(all);
}

//
// TrackLayout
// 

// диапазон длины кадра на дорожке
const double FRAME_MIN_LEN = 0.03;
const double FRAME_MAX_LEN = 35.0;

const double SCALE_MIN  = 0.0;
const double SCALE_MAX  = 10.0;
const double SCALE_STEP = 0.1;


TrackLayout::TrackLayout(Timeline::Monitor& mon):
    trkMon(mon),
    trkStt(&Timeline::NormalTL::Instance()),
    trkScale(SCALE_MIN, SCALE_MIN, SCALE_MAX, SCALE_STEP, SCALE_MAX/10.0, 0.),
    trkHScroll(0., 0., 1., 50., 5., 1.),
    trkVScroll(0., 0., 1., 5., 5., 1.)
{
    AddStandardEvents(*this);
    trkScale.signal_value_changed().connect( sigc::mem_fun(*this, &TrackLayout::OnFrameScale) );
    trkHScroll.signal_value_changed().connect( sigc::mem_fun(*this, &TrackLayout::OnHScrollChange) );
    trkVScroll.signal_value_changed().connect( sigc::mem_fun(*this, &TrackLayout::OnVScrollChange) );

    // события фокуса
    property_can_focus() = true;

    // :TRICKY:
    //get_window()->set_debug_updates();

    if( !GetMonitor().GetPlayer().IsOpened() )
        CloseTrackLayout(*this);
}

double TrackLayout::FrameScale()
{
    // распределяем диапазон длины кадра [0.03, 30.0] экспоненциальной
    // функцией, потому что так естественней (почему-то) смотрится
    double res = trkScale.get_value();
    res = pow(2.0, res)/32.0;

    ASSERT( res > FRAME_MIN_LEN && res < FRAME_MAX_LEN );
    return res; 
}

static void SetAdjValue(Gtk::Adjustment& adj, double val)
{
    double lower   = adj.get_lower();
    double upper   = adj.get_upper();
    double p_size  = adj.get_page_size();
    double max_val = std::max(lower, upper-p_size);

    val = std::max(lower,   val);
    val = std::min(max_val, val);
    adj.set_value(val);
}

static void SetAdjBounds(Gtk::Adjustment& adj, int size, int page_size)
{
    adj.set_upper(size);
    adj.set_page_size(page_size);
    adj.set_page_increment(0.3*page_size);

    SetAdjValue(adj, adj.get_value());
}

double TrackLayout::FrameFPS()
{
    return Mpeg::GetFrameFPS(trkMon.GetPlayer());
}

double TrackLayout::GetFramesLength() 
{ 
    return Timeline::GetFramesLength(trkMon.GetPlayer()); 
}

int TrackLayout::GetGroundHeight()
{
    // + 2 пикселя границы
    return TRACK_HGT+2;
}

int TrackLayout::GetGroundOff()
{
    // наезжаем под разделительную линию
    return HEADER_HGT-1;
}

void TrackLayout::SetAdjs(Point sz)
{
    int win_wdh = std::max(sz.x - HEADER_WDH,     0);
    int win_hgt = std::max(sz.y - GetGroundOff(), 0);

    double wdh = GetTrackSize();
    wdh += 0.3*win_wdh; // плюс пустое пространcтво

    SetAdjBounds(trkHScroll, (int)wdh, win_wdh);
    SetAdjBounds(trkVScroll, (int)GetGroundHeight(), win_hgt);
}

void TrackLayout::Rebuild()
{
    Point sz(WidgetSize(*this));
    SetAdjs(sz);

    if( sz.x >= HEADER_WDH && trkHPaned.get_position() != HEADER_WDH )
        trkHPaned.set_position(HEADER_WDH);
    if( sz.y >= HEADER_HGT && trkVPaned.get_position() != HEADER_HGT )
        trkVPaned.set_position(HEADER_HGT);

    queue_draw();
}

void TrackLayout::on_size_allocate(Gtk::Allocation& allocation)
{
    MyParent::on_size_allocate(allocation);
    Rebuild();
}

void TrackLayout::HUpdate(Point sz)
{
    queue_draw_area( HEADER_WDH, 0, sz.x, sz.y );
}

void TrackLayout::VUpdate(Point sz)
{
    queue_draw_area( 0, HEADER_HGT, sz.x, sz.y );
}

void TrackLayout::OnFrameScale()
{
    ASSERT( is_realized() );
    Point sz(WidgetSize(*this));

    SetAdjs(sz);
    HUpdate(sz);
}

void TrackLayout::OnHScrollChange()
{
    HUpdate(WidgetSize(*this));
}

void TrackLayout::OnVScrollChange()
{
    VUpdate(WidgetSize(*this));
}

bool TrackLayout::on_button_press_event(GdkEventButton* event)
{
    GrabFocus(*this);
    return OnButtonPressEvent(*this, trkStt, event);
}

bool TrackLayout::on_button_release_event(GdkEventButton* event)
{
    return OnButtonReleaseEvent(*this, trkStt, event);
}

bool TrackLayout::on_motion_notify_event(GdkEventMotion* event)
{
    return OnMotionNotifyEvent(*this, trkStt, event);
}

bool TrackLayout::on_scroll_event(GdkEventScroll* event)
{
    int step = 0;
    switch( event->direction )
    {
    case GDK_SCROLL_UP:
        step = -1;
        break;
    case GDK_SCROLL_DOWN:
        step = 1;
        break;
    default:
        break;
    }
    double val = trkHScroll.get_value() + step * trkHScroll.get_step_increment();
    SetAdjValue(trkHScroll, val);
    trkHScroll.changed();
    return false;
}

bool TrackLayout::on_key_press_event(GdkEventKey* event)
{
  return OnKeyPressEvent(*this, trkStt, event);
}


// svg
//#include <mgui/sdk/cairo_utils.h>

bool TrackLayout::on_expose_event(GdkEventExpose* event)
{
    RefPtr<Gdk::Window> p_win = get_bin_window();
    Cairo::RefPtr<Cairo::Context> cr = p_win->create_cairo_context();
//     Cairo::RefPtr<Cairo::Context> cr = CreateSVGCairoContext("../TrackLayout", sz);

    Timeline::RenderSvc svc(cr, *this, DRect(MakeRect(event->area)));
    svc.FormLayout();

    // даем отрисоваться родителю, иначе некоторые потомки-виджеты, не имеющие
    // своего окна, не будут отрисовываться (кнопки, например; а для поля ввода - никакой разницы)
    return MyParent::on_expose_event(event);
}

void SetNonActiveButton(Gtk::Button& btn, const char* style_name)
{
    // см. gtk_button_size_request() как уменьшить размер кнопки
    btn.set_name(style_name);

    btn.set_relief(Gtk::RELIEF_NONE); //RELIEF_HALF);
    btn.set_focus_on_click(false);
    btn.property_can_focus() = false;
}

// str_sign = ["-"|"+"]
static void SetupScaleLabel(Gtk::Label& lbl, const char* str_sign)
{
    std::string str = "<span font_desc=\"Courier Bold 12\" color=\"#" + ColorToString(BLUE_CLR) + "\">";
    str += str_sign;
    str += "</span>";

    lbl.set_text(str);
    lbl.set_use_markup(true);
}

static void MoveAdj(Gtk::Adjustment& adj, int add, bool is_page)
{
    double shift = add * ( !is_page ? adj.get_step_increment() : adj.get_page_increment() );
    double val = adj.get_value();
    if( shift>0 )
        val = std::min(val+shift, adj.get_upper());
    else
        val = std::max(val+shift, adj.get_lower());

    adj.set_value(val);
    adj.value_changed();
}

Gtk::Button* CreateScaleButton(TrackLayout& trk_lay, bool is_plus)
{
    Gtk::Button* btn = Gtk::manage(new SquareButton);
    SetNonActiveButton(*btn, "ScaleTrackButton");

    using namespace boost;
    int add = is_plus ? 1 : -1 ;
    btn->signal_clicked().connect( 
        wrap_return<void>(lambda::bind(&MoveAdj, boost::ref(trk_lay.TrkScale()), add, true)) );

    return btn;
}

class TrackLayoutVis: public Project::ObjVisitor 
{
    public:
                      TrackLayoutVis(TrackLayout& lay): layout(lay) {}
    protected:
            TrackLayout& layout;
};

class TLOnDeleteVis: public TrackLayoutVis
{
    typedef TrackLayoutVis MyParent;
    public:
                      TLOnDeleteVis(TrackLayout& lay): MyParent(lay) {}

    virtual     void  Visit(Project::VideoChapterMD& obj);
    virtual     void  Visit(Project::VideoMD& obj);
};

void TLOnDeleteVis::Visit(Project::VideoChapterMD& obj)
{
    if( obj.owner == CurrVideo )
        RedrawDVDMark(layout, Project::ChapterPosInt(&obj));
}

void TLOnDeleteVis::Visit(Project::VideoMD& obj)
{
    if( &obj == CurrVideo )
    {
        CloseTrackLayout(layout);
        Timeline::SetCurrentVideo(0);
    }
}

class TLOnChangeNameVis: public TrackLayoutVis
{
    typedef TrackLayoutVis MyParent;
    public:
                      TLOnChangeNameVis(TrackLayout& lay): MyParent(lay) {}

    virtual     void  Visit(Project::VideoMD& obj);
};

class TrackNameCoverSvc: public Timeline::CoverSvc
{
    typedef CoverSvc MyParent;
    public:

                    TrackNameCoverSvc(TrackLayout& trk_lay): MyParent(trk_lay) {}

    protected:

    virtual   void  Process() { FormContent(); }
    virtual   void  ProcessScale() {}
    virtual   void  ProcessContent() { FormTrack(); }
    virtual   void  ProcessTrack()   { FormTrackName(); }
    virtual   void  ProcessTrackName(RefPtr<Pango::Layout>, const Rect& lct) 
                    {
                        AddRect(DRect(lct));
                    }
};

void TLOnChangeNameVis::Visit(Project::VideoMD& obj)
{
    if( &obj == CurrVideo )
        TrackNameCoverSvc(layout).FormLayout();
}

void PackTrackLayout(Gtk::Container& contr, TrackLayout& layout)
{
    // нужно для получения верных значений ресурсов
    ASSERT( contr.has_screen() );
    Gtk::Table& tbl = *Gtk::manage(new Gtk::Table(2, 2, false));
    contr.add(tbl);

    tbl.attach( layout, 0, 1, 0, 1,
                Gtk::EXPAND | Gtk::FILL | Gtk::SHRINK,
                Gtk::EXPAND | Gtk::FILL | Gtk::SHRINK,
                0, 0 );
    Project::RegisterOnDelete(new TLOnDeleteVis(layout));
    Project::RegisterOnChangeName(new TLOnChangeNameVis(layout));

    {
        Gtk::HPaned& hp = layout.TrkHPaned();
        {
            // делаем отступ от бегунка
            Gtk::Alignment& algn = NewPaddingAlg(0, 0, 0, 2);
            Gtk::HBox& hbox = Add(algn, NewManaged<Gtk::HBox>());
            {
                Gtk::Button* m_btn = CreateScaleButton(layout, false);
                {
                    Gtk::Label* m_lbl = Gtk::manage(new CenteredLabel);
                    SetupScaleLabel(*m_lbl, "-");

                    m_btn->add(*m_lbl);
                }
                hbox.pack_start(*m_btn, false, true);

                Gtk::HScale* scl = Gtk::manage(new Gtk::HScale(layout.TrkScale()));
                SetScaleSecondary(*scl);
                hbox.pack_start(*scl, true, true);

                Gtk::Button* p_btn = CreateScaleButton(layout, true);
                {
                    Gtk::Label* p_lbl = Gtk::manage(new CenteredLabel);
                    SetupScaleLabel(*p_lbl, "+");

                    p_btn->add(*p_lbl);
                }
                hbox.pack_start(*p_btn, false, true);
            }
            hp.add1(algn);

            Gtk::HScrollbar* scr_bar = Gtk::manage(new Gtk::HScrollbar(layout.TrkHScroll()));
            hp.add2(*scr_bar);

        }
        tbl.attach( hp, 0, 1, 1, 2,
                    Gtk::EXPAND | Gtk::FILL | Gtk::SHRINK,
                    Gtk::FILL,
                    0, 0 );
    }

    {
        Gtk::VPaned& vp = layout.TrkVPaned();
        {
            Gtk::Label* lbl = Gtk::manage(new Gtk::Label); // как пустое место
            vp.add1(*lbl);

            Gtk::VScrollbar* scr_bar = Gtk::manage(new Gtk::VScrollbar(layout.TrkVScroll()));
            vp.add2(*scr_bar);
        }
        tbl.attach( vp, 1, 2, 0, 1,
                    Gtk::FILL,
                    Gtk::EXPAND | Gtk::FILL | Gtk::SHRINK,
                    0, 0 );
    }

    // кнопка добавления DVD-метки
    {
        Gtk::Button* dvd_btn = Gtk::manage(new Gtk::Button);
        SetNonActiveButton(*dvd_btn, "DVDLabelButton");
        SetTip(*dvd_btn, _("Add Chapter Point"));
        dvd_btn->signal_clicked().connect( boost::lambda::bind(&InsertDVDMark, boost::ref(layout)) );

        Gtk::Image* dvd_img = Gtk::manage(new Gtk::Image(GetFactoryImage("dvdmark.png")));
        dvd_btn->add(*dvd_img);
        dvd_btn->show_all();
        layout.put(*dvd_btn, 0, 0);

        // положение (после вставки!)
        Rect lct(Timeline::GetBigLabelLocation(layout));
        Gtk::Requisition req = dvd_btn->size_request();
        layout.move(*dvd_btn, lct.rgt-req.width, lct.btm-2);
    }

    // чтоб до минимума можно уменьшать размер таймлинии
    tbl.set_size_request(0, 0);
}

namespace Timeline 
{

time4_t FramesToTime(int cnt, double fps)
{
    time4_t res;

    int sec = int(cnt / fps);
    // показываем округленное значение кадров для нецелых fps
    // таким образом, базовой системой координат для нас является
    // позиция в кадрах (пока; возможно перейдем на другой вариант,-
    // округление fps, как например в Adobe Premiere и Sony Vegas)
    // см. ParsePointerPos()
    res.ff = int(cnt - sec * fps);
    //int ff = Round(cnt - sec * fps);

    int min = sec / 60;
    res.ss  = sec - min*60;
    res.hh  = min / 60;
    res.mm  = min - res.hh*60;

    return res;
}

void FramesToTime(std::string& str, int cnt, double fps)
{
    time4_t t4 = FramesToTime(cnt, fps);
    str = (str::stream() << Mpeg::set_hms() << t4.hh << ":" << Mpeg::set_hms() << t4.mm << ":"
                         << Mpeg::set_hms() << t4.ss << ";" << Mpeg::set_hms() << t4.ff).str();
}

} // namespace TimeLine

static void MakeFullUpdate(TrackLayout& layout, bool set_sensitive)
{
    DAMonitor* mon = dynamic_cast<DAMonitor*>(&layout.GetMonitor());
    if( mon )
        mon->Rebuild();

    layout.Rebuild();
    layout.set_sensitive(set_sensitive);
}

void CloseTrackLayout(TrackLayout& layout)
{
    using namespace Timeline;
    ClosedTL& stt = ClosedTL::Instance();
    stt.ChangeTo(layout);

    if( layout.is_sensitive() ) // так определяем что мы в нормальном состоянии
    {
        Monitor& mon       = layout.GetMonitor();
        Mpeg::Player& plyr = mon.GetPlayer();

        plyr.Close();
        layout.SetPos(-1);

        MakeFullUpdate(layout, false);
    }
}

Project::VideoItem Timeline::SetCurrentVideo(Project::VideoItem new_vd)
{
    Project::VideoItem old_vd = CurrVideo;
    CurrVideo = new_vd ? new_vd : Project::VideoItem(new Project::VideoMD) ;

    return old_vd;
}

bool OpenTrackLayout(TrackLayout& layout, Project::VideoItem vd, TLFunctor on_open_fnr)
{
    bool res = false;
    if( CurrVideo == vd )
        res = true;
    else
    {
        CloseTrackLayout(layout);

        using namespace Timeline;
        Monitor& mon       = layout.GetMonitor();
        Mpeg::Player& plyr = mon.GetPlayer();
        if( plyr.Open(GetFilename(*vd).c_str()) )
        {
            NormalTL& stt = NormalTL::Instance();
            stt.ChangeTo(layout);

            layout.SetPos(0);

            MakeFullUpdate(layout, true);
            res = true;
        }
    }

    SetCurrentVideo(res ? vd : Project::VideoItem());
    if( res && on_open_fnr )
        on_open_fnr(layout);
    return res;
}


