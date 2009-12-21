//
// mgui/timeline/service.cpp
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

#include <mgui/_pc_.h>

#include <cmath> // std::ceil(), std::floor()

#include <mgui/render/text.h>
#include <mgui/img-factory.h>
#include "service.h"

namespace Timeline
{

TLContext::TLContext(CR::RefPtr<CR::Context> cr_, TrackLayout& trk_lay)
    : trkLay(trk_lay), cont(cr_), winSz(WidgetSize(trk_lay)),
      shift(trkLay.GetShift())
{}

//
// Service
// 

void Service::FormLayout()
{
    cont->set_line_width(1.0);

    Process();
}

void Service::FormBigLabel()
{
    RefPtr<Pango::Layout> lay = Pango::Layout::create(cont);
    //std::string mark_str = "<span font_desc=\"FreeSans Bold 11\" color=\"darkblue\" underline=\"single\">" +
    std::string mark_str = "<span font_desc=\"Nimbus Sans L Bold Condensed 13\" color=\"#" +
        ColorToString(BLUE_CLR) + "\" underline=\"single\">" + CurPointerStr(trkLay) + "</span>";
    lay->set_markup(mark_str);

    int sh = (int)(HEADER_WDH - CalcTextSize(lay).x) / 2; // 0;
    ProcessBigLabel(lay, Point(sh, 8));
}

DPoint Service::CalcTextSize(RefPtr<Pango::Layout> lay)
{
    return CalcAbsSizes(lay, GetDPI(trkLay));
}

void Service::FormScale()
{
    CairoStateSave save(cont);
    // переносим координаты на начало шкалы
    int scl_wdh = winSz.x - HEADER_WDH;
    cont->translate(HEADER_WDH, 0.);

    // вычисляем положение шкалы
    Rect scl_rct(Point(-1, 0), Point(scl_wdh, HEADER_HGT));
    scl_rct.top += 5;

    // лишнее не рисовать
    if( !scl_rct.IsValid() )
        return;

    {
        // так удобней рисовать форму шкалы
        CairoStateSave save(cont);
        CR::HPTranslate(cont);

        cont->move_to(scl_rct.lft, scl_rct.btm-1);
        double radius = 5;
        cont->arc(scl_rct.lft+radius,   scl_rct.top+radius, radius, M_PI, 1.5*M_PI);
        cont->arc(scl_rct.rgt-radius-1, scl_rct.top+radius, radius, 1.5*M_PI, 0.0);
        cont->line_to(scl_rct.rgt-1, scl_rct.btm-1);
        cont->close_path();
    }

    sclRct = scl_rct;
    ProcessScale();
}

void Service::FormHandlers()
{
    FormDVDMarks();
    FormPointer();
}

static CR::RefPtr<CR::ImageSurface> GetDVDMark(bool is_top)
{
    static CR::RefPtr<CR::ImageSurface> top_dvd_sur;
    static CR::RefPtr<CR::ImageSurface> btm_dvd_sur;

    CR::RefPtr<CR::ImageSurface> dvd_sur = is_top ? top_dvd_sur : btm_dvd_sur ;
    if( !dvd_sur )
    {
        const char* img_name = is_top 
            ? 0 //"test.png"
            : "dvdmark.png" ;
        ASSERT( img_name );

        dvd_sur = ConvertToSurface(GetFactoryImage(img_name)->copy());
    }
    return dvd_sur;
}

static double GetDVDMarkXPos(Service& svc, DVDArrType::iterator cur)
{
    return svc.trkLay.FramesSz(GetMarkData(*cur).pos) - svc.shift.x;
}

void Service::FormDVDMarks()
{
    CairoStateSave save(cont);
    int line_hgt = GetSclLevel();

    for( DVDArrType::iterator cur = DVDMarks().begin(); cur != DVDMarks().end(); ++cur )
    {
        double pos = GetDVDMarkXPos(*this, cur);
        if( IsVisibleObj((int)pos, sclRct.Width()) )
        {
            double x = pos;
            double y = line_hgt;
            bool is_top = false;//cur->isTop;

            CR::RefPtr<CR::ImageSurface> sur = GetDVDMark(false);

            Point sz(sur->get_width(), sur->get_height());
            // sz.x должно быть нечетным!
            x -= sz.x/2;
            if( is_top )
                y -= sz.y;
            else
                y += 1; // на пиксель ниже от горизонт. линии
            ProcessDVDMark(cur-DVDMarks().begin(), Point(Round(x), Round(y)));
        }
    }
}

void Service::FormPointer()
{
    if( trkLay.CurPos() >= 0 )
    {
        int line_hgt = GetSclLevel();
        int pos = Round(trkLay.FramesSz(trkLay.CurPos()) - shift.x);
        if( IsVisibleObj(pos, sclRct.Width()) )
        {
            CairoStateSave save(cont);
            ProcessPointer(Point(pos, line_hgt));
        }
    }
}

int Service::GetSclLevel()
{
    return Timeline::GetSclLevel(sclRct.top, sclRct.btm);
}

void Service::FormContent()
{
    int gr_top = trkLay.GetGroundOff();
    CairoStateSave save(cont);
    CR::RectClip(cont, Rect(Point(0, gr_top), winSz));

    // переносимся на начало "земли"
    cont->translate(0, gr_top-shift.y);

    ProcessContent();
}

void Service::FormTrack()
{
    cont->translate(HEADER_WDH, 0);
    ProcessTrack();
}

Rect Service::GetTrackLocation(bool is_trough)
{
    Rect trk_rct(0, 0, winSz.x-HEADER_WDH-1, trkLay.GetGroundHeight());
    trk_rct = EnlargeRect(trk_rct, Point(0, -1));

    if( !is_trough )
    {
        double wdh  = trkLay.GetTrackSize();
        int win_wdh = Round(wdh-shift.x);
        if( shift.x > 0 )
            trk_rct.lft = -1; // невидимый
        trk_rct.rgt = (win_wdh > trk_rct.rgt) ? trk_rct.rgt + 1 : win_wdh ;
    }

    return trk_rct;
}

void Service::FormDVDThumbnails()
{
    Rect v_rct(GetTrackLocation(false));
    Point aspect = Mpeg::GetAspectRatio(trkLay.GetMonitor().GetPlayer());

    int hgt = v_rct.Height();
    int wdh = int((double)hgt/aspect.y*aspect.x);

    //CR::RefPtr<CR::ImageSurface> sur = GetDVDThumbnail(v_rct.Height());
    for( DVDArrType::iterator cur = DVDMarks().begin(); cur != DVDMarks().end(); ++cur )
    {
        int pos = Round(GetDVDMarkXPos(*this, cur)); //trkLay.FramesSz(cur->pos) - shift.x);
        //Rect lct( RectASz(Point(pos, 1), Point(sur->get_width(), sur->get_height())) );
        Rect lct( RectASz(Point(pos, 1), Point(wdh, hgt)) );

        if( lct.Intersects(v_rct) )
            ProcessDVDThumbnail(cur-DVDMarks().begin(), lct);
    }
}

void Service::FormTrackName()
{
    Rect v_rct(GetTrackLocation(false));
    const std::string& name = CurrVideo->mdName;
    if( !name.empty() )
    {
        RefPtr<Pango::Layout> lay = Pango::Layout::create(cont);
        Pango::FontDescription dsc("Sans Italic 12");
        lay->set_font_description(dsc);
        lay->set_text(name);

        Rect txt_rct( CeilRect(RectASz(DPoint(3 - shift.x, 3), CalcTextSize(lay))) );
        if( v_rct.Intersects(txt_rct) )
            ProcessTrackName(lay, txt_rct);
    }
}

void Service::PaintPointer(const Point& pos, bool form_only)
{
    Timeline::PaintPointer(cont, DPoint(pos.x+H_P, pos.y), form_only);
}

//
// HookSvc
// 

void LeftMouseHook::AtBigLabel()
{
    actStt = &EditBigLabelTL::Instance();
}

void LeftMouseHook::AtScale()
{
    PointerMoverTL& stt = PointerMoverTL::Instance();
    stt.SetCursorPos(trkLay, lct, CR::DeviceToUser(cont, lct));
    trkLay.MoverTL::Data::atPointer = false;

    actStt = &stt;
}

void LeftMouseHook::AtDVDMark(int idx)
{
    DVDLabelMoverTL& stt = DVDLabelMoverTL::Instance();
    stt.SetDVDIdx(trkLay, idx);

    actStt = &stt;
}

void LeftMouseHook::AtPointer()
{
    trkLay.MoverTL::Data::atPointer = true;
    actStt = &PointerMoverTL::Instance();
}

static bool IsInPath(CR::RefPtr<CR::Context> cr, DPoint pos)
{
    pos = CR::DeviceToUser(cr, pos);
    return cr->in_fill(pos.x, pos.y);
}

void HookSvc::Process()
{
    FormBigLabel();
    FormScale(); 
}

void HookSvc::ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos)
{
    DPoint user_pos(CR::DeviceToUser(cont, PhisPos()));
    DPoint sz(CalcTextSize(lay));

    DRect user_rct(RectASz(DPoint(pos), sz));
    if( user_rct.Contains(user_pos) )
    {
        pAction->AtBigLabel();
    }
}

void RenderSvc::ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos)
{
    // смещение надписи
    cont->move_to(pos.x, pos.y);
    show_in_cairo_context(lay, cont);
}

void HookSvc::ProcessScale()
{
    bool is_in = IsInPath(cont, PhisPos());
    cont->begin_new_path();

    if( is_in )
    {
        pAction->AtScale();
        FormHandlers();
    }
}

void RenderSvc::ProcessDVDMark(int /*idx*/, const Point& pos)
{
    cont->set_source(GetDVDMark(false), pos.x, pos.y);
    cont->paint();
}

static bool IsAtPicture(CR::RefPtr<CR::ImageSurface> pic, const Point& pos)
{
    ASSERT( pic->get_format() == Cairo::FORMAT_ARGB32 );
    Point sz(pic->get_width(), pic->get_height());
    ASSERT( pos.x < sz.x && pos.y < sz.y );

    uint8_t* dat = pic->get_data() + pic->get_stride() * pos.y + 4 * pos.x;
    return CairoGetAlpha(dat);
}

static DRect GetDVDLabelLocation(int /*idx*/, const Point& pos)
{
    CR::RefPtr<CR::ImageSurface> sur = GetDVDMark(false);
    DPoint user_pos(pos);
    return RectASz(user_pos, DPoint(sur->get_width(), sur->get_height()));
}

void HookSvc::ProcessDVDMark(int idx, const Point& pos)
{
    DRect user_lct(GetDVDLabelLocation(idx, pos));

    DPoint real_pos(PhisPos());
    cont->device_to_user(real_pos.x, real_pos.y);
    if( user_lct.Contains(real_pos) )
    {
        DPoint int_pos(real_pos - user_lct.A());
        if( IsAtPicture(GetDVDMark(false), Point((int)int_pos.x, (int)int_pos.y)) )
        {
            pAction->AtDVDMark(idx);
        }
    }
}

void HookSvc::ProcessPointer(const Point& pos)
{
    PaintPointer(pos, true);
    if( IsInPath(cont, DPoint(PhisPos())) )
    {
        pAction->AtPointer();
    }
    cont->begin_new_path();
}

// 
// *CoverSvc
// 

// static void PrintRect(const DRect& rct)
// {
//     io::cout.precision(8);
//     io::cout << rct.lft << " " << rct.top << " " << rct.rgt << " " << rct.btm << io::endl;
// }

void CoverSvc::AddRect(const DRect& user_rct)
{
    DRect drct(CR::UserToDevice(cont, user_rct));

    Rect rct( (int)std::floor(drct.lft), (int)std::floor(drct.top), 
              (int)std::ceil(drct.rgt),  (int)std::ceil(drct.btm)   );
    // на всякий случай увеличим
    rct = EnlargeRect(rct, Point(1, 1));

    // :TODO: переделать используя ReDivide()
    trkLay.queue_draw_area(rct.lft, rct.top, rct.Width(), rct.Height());
}

void PointerCoverSvc::Process()
{
    FormBigLabel();
    FormScale(); 

    if( DrawRedLine(cont, trkLay) )
    {
        DRect rct;
        cont->get_stroke_extents(rct.lft, rct.top, rct.rgt, rct.btm);
        AddRect(rct);

        cont->begin_new_path();
    }
}

void PointerCoverSvc::ProcessScale()
{
    cont->begin_new_path();
    FormPointer();
}

void PointerCoverSvc::ProcessPointer(const Point& pos)
{
    PaintPointer(pos, true);
    DRect rct;
    cont->get_stroke_extents(rct.lft, rct.top, rct.rgt, rct.btm);

    AddRect(rct);

    cont->begin_new_path();
}

void PointerCoverSvc::ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos)
{
    DPoint sz(CalcTextSize(lay));
    AddRect( RectASz(DPoint(pos), sz) );
}

void DVDLabelCoverSvc::Process()
{
    FormScale();
    FormContent();
}

void DVDLabelCoverSvc::ProcessScale()
{
    cont->begin_new_path();
    FormDVDMarks();
}

void DVDLabelCoverSvc::ProcessDVDMark(int idx, const Point& pos)
{
    if( idx == dvdIdx )
        AddRect(GetDVDLabelLocation(idx, pos));
}

void DVDLabelCoverSvc::ProcessDVDThumbnail(int idx, const Rect& lct)
{
    if( idx == dvdIdx )
    {
        DRect drct(lct);
        drct.lft -= 1; // разделительная черта
        AddRect(drct);
    }
}

//
// DrawRedLine()
// 

class RedLineCalcSvc: public CalcSvc
{
    typedef CalcSvc MyParent;
    public:

                    RedLineCalcSvc(TrackLayout& trk_lay)
                        : MyParent(trk_lay), crossLine(-1, 0) {}

              bool  Calc()
                    {
                        FormLayout();
                        return crossLine.x != -1;
                    }
             Point  GetPos() { return crossLine; }

    protected:

        Point  crossLine; // красная линия указателя

    virtual   void  Process();
    virtual   void  ProcessBigLabel(RefPtr<Pango::Layout>, const Point&) {}
    virtual   void  ProcessScale();
    virtual   void  ProcessDVDMark(int, const Point&) {}
    virtual   void  ProcessPointer(const Point& pos);
};

void RedLineCalcSvc::Process()
{
    FormScale();
}

void RedLineCalcSvc::ProcessScale()
{
    cont->begin_new_path();
    FormPointer();
}

void RedLineCalcSvc::ProcessPointer(const Point& pos)
{
    DPoint phis_pos( CR::UserToDevice(cont, DPoint(pos)) );
    crossLine.x = (int)phis_pos.x;
    crossLine.y = (int)phis_pos.y;
}

bool DrawRedLine(CR::RefPtr<CR::Context> cr, TrackLayout& trk_lay)
{
    bool res = false;

    RedLineCalcSvc rl_svc(trk_lay);
    if( rl_svc.Calc() )
    {
        Point pos(rl_svc.GetPos());
        res = true;
        cr->move_to(pos.x+H_P, pos.y);
        cr->line_to(pos.x+H_P, trk_lay.get_height());
    }
    return res;
}

} // namespace Timeline

