//
// mgui/timeline/layout-draw.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008, 2010 Ilya Murav'jov
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
#include "service.h"

#include <mgui/sdk/clearlooks.h>
#include <mgui/render/text.h>
#include <mgui/gettext.h>

namespace Timeline
{

void RenderSvc::Process()
{
    CR::RectClip(cont, drwRct);

    // 1 текущее положение курсора
    FormBigLabel();

    // 2 шкала
    FormScale();

    // 3 дорожка
    FormContent();

    // 4 разделительные линии
    {
        CR::SetColor(cont, GetBorderColor(trkLay));
        // горизонтальная линия
        cont->move_to(0,       HEADER_HGT-H_P);
        cont->line_to(winSz.x, HEADER_HGT-H_P);
        cont->stroke();
        // вертикальная
        cont->move_to(HEADER_WDH-H_P, HEADER_HGT);
        cont->line_to(HEADER_WDH-H_P, winSz.y);
        cont->stroke();
    }

    // 5 красная линия указателя
    if( DrawRedLine(cont, trkLay) )
    {
        CR::SetColor(cont, RED_CROSS_LINE);
        cont->stroke();
    }
}


static void PaintReliefFrame(CR::RefPtr<CR::Context> cr, const Rect& rct, 
                             bool is_pushed, const CR::Color& shade_clr)
{
    CR::Color top_clr(TOP_LFT_CLR);
    CR::Color btm_clr(shade_clr);
    if( is_pushed )
    {
        btm_clr = top_clr;
        top_clr = shade_clr;
    }

    CR::SetColor(cr, top_clr);
    cr->move_to(rct.lft+H_P, rct.btm-1);
    cr->line_to(rct.lft+H_P, rct.top+H_P);
    cr->line_to(rct.rgt-1,   rct.top+H_P);
    cr->stroke();

    CR::SetColor(cr, btm_clr);
    cr->move_to(rct.lft,       rct.btm-H_P);
    cr->line_to(rct.rgt-H_P, rct.btm-H_P);
    cr->line_to(rct.rgt-H_P, rct.top);
    cr->stroke();
}

static void PaintRectangleForFill(CR::RefPtr<CR::Context> cr, const Rect& rct, int dec = 0)
{
    cr->rectangle(rct.lft+dec, rct.top+dec, rct.Width()-2*dec, rct.Height()-2*dec);
}

static void FillRectangle(CR::RefPtr<CR::Context> cr, const Rect& rct, 
                          bool is_clip, int dec = 0)
{
    PaintRectangleForFill(cr, rct, dec);
    if( is_clip ) 
    {
        cr->fill_preserve();
        cr->clip();
    }
    else
        cr->fill();
}

//
// CurrVideo
// 

Project::VideoItem CurrVideo(new Project::VideoMD);

DVDMark PushBackDVDMark(int frame_pos)
{
    ASSERT( CurrVideo );
    using namespace Project;
    ChapterItem ci = VideoChapterMD::CreateChapter(CurrVideo.get(), 0);
    GetMarkData(ci).pos = frame_pos;

    return ci;
}

// static void PrintColor(RGBA::Pixel pxl)
// {
//     io::cout << "clr " << std::hex << (int)pxl.red << " " << (int)pxl.green << " " << (int)pxl.blue << io::endl;
// }

// static void PrintPoint(CR::RefPtr<CR::Context> cr, double x, double y)
// {
//     io::cout.precision(8);
//     io::cout << "user coords: " << x << ", " << y << "; ";
//     cr->user_to_device(x, y);
//     io::cout << "dev  coords: " << x << ", " << y << "; ";
// }

void RenderSvc::ProcessContent()
{
    Rect trk_rct(0, 0, winSz.x, trkLay.GetGroundHeight());
    CR::Color sh_clr = GetBorderColor(trkLay);
    // 1 земля
    PaintReliefFrame(cont, trk_rct, true, sh_clr);

    // 2 заголовок дорожки
    Rect head_rct(Point(1, 1), Point(HEADER_WDH, trk_rct.btm-1));
    PaintReliefFrame(cont, head_rct, false, sh_clr);

    //CR::SetColor(cr, SCALE_CLR, 0.7);
    CR::SetColor(cont, GetBGColor(trkLay), 0.90);
    FillRectangle(cont, head_rct, false, 1);

    RefPtr<Pango::Layout> lay = Pango::Layout::create(cont);
    std::string mark_str = boost::format("<span font_desc=\"Sans 11\" color=\"black\">%1%</span>") % _("Video") % bf::stop;
    lay->set_markup(mark_str);

    DPoint txt_pos = FindAForCenteredRect(CalcTextSize(lay), DRect(head_rct));
    //cont->move_to(10, 5);
    cont->move_to(Round(txt_pos.x), Round(txt_pos.y));
    show_in_cairo_context(lay, cont);

    FormTrack();
}

void RenderSvc::ProcessTrack()
{
    Rect trk_rct(GetTrackLocation(true));
    CR::Color brd_clr = GetBorderColor(trkLay);

    // 3.1 "желоб"
    CR::SetColor(cont, GetBGColor(trkLay), 0.95);
    FillRectangle(cont, trk_rct, true, 0);

    // 3.1.1 "тень от шкалы"
    CR::SetColor(cont, brd_clr, 1.5);
    cont->move_to(trk_rct.lft, trk_rct.top+H_P);
    cont->line_to(trk_rct.rgt, trk_rct.top+H_P);
    cont->stroke();

    // 3.2 сама дорожка
    Rect v_rct(GetTrackLocation(false));
    PaintReliefFrame(cont, v_rct, false, brd_clr);

    CR::SetColor(cont, TRACK_CLR);
    FillRectangle(cont, v_rct, true, 1);

    // 3.3 кадры DVD-меток
    FormDVDThumbnails();

    // 4 название медиа
    FormTrackName();
}

void RenderSvc::ProcessDVDThumbnail(int idx, const Rect& lct)
{
    DVDMarkData& mrk = GetMarkData(idx);
    Point sz = lct.Size();
    if( !mrk.thumbPix )
    {
        RefPtr<Gdk::Pixbuf> pix = CreatePixbuf(sz);
        trkLay.GetMonitor().GetFrame(pix, mrk.pos);

        mrk.thumbPix = ConvertToSurface(pix);
    }
    else
        ASSERT( mrk.thumbPix->get_width() == sz.x && mrk.thumbPix->get_height() == sz.y );

    //cont->set_source(GetDVDThumbnail(lct.Height()), lct.lft, lct.top);
    cont->set_source(mrk.thumbPix, lct.lft, lct.top);
    //cr->rectangle(lct.lft, lct.top, lct.Width(), lct.Height());
    //cr->fill();
    cont->paint();

    // отделяем миниатюры (thumbnails) друг от друга линией контента
    CR::SetColor(cont, TRACK_CLR);
    cont->move_to(lct.lft-H_P, lct.top);
    cont->line_to(lct.lft-H_P, lct.btm);
    cont->stroke();
}

void RenderSvc::ProcessTrackName(RefPtr<Pango::Layout> lay, const Rect& txt_rct)
{
    CR::SetColor(cont, TOP_LFT_CLR);
    cont->move_to(txt_rct.lft, txt_rct.top);
    show_in_cairo_context(lay, cont);
}

// найти максимальное n_rnd=2^i, не большее n
const int MaxPower2 = 0x40000000;
static int GetLowPower2(int n)
{
    int i = MaxPower2;
    for( bool null=false; i ; i >>= 1 )
    {
        if( !null )
        {
            if( i & n )
                null = true;
        }
        else
            n &= ~i;
    }
    return n;
}

const char* MarkFontDsc = "Monospace 8";//"Serif 8";

static Point GetTimeMarkSize(double dpi)
{
    static Point txt_sz;
    if( txt_sz.IsNull() )
    {
        Pango::FontDescription dsc(MarkFontDsc);
        double wdh, hgt;
        CalcAbsSizes("00:00:00;00", dsc, wdh, hgt, dpi);

        txt_sz = Point(Round(wdh), Round(hgt));
    }
    return txt_sz;
}

void PaintMark(Cairo::RefPtr<Cairo::Context> cr, double from_x, double from_y,
               int sz)
{
    cr->move_to(Round(from_x)+H_P, from_y);
    cr->rel_line_to(0, sz);
    cr->stroke();
}

void RenderSvc::ProcessScale()
{
    Rect scl_rct(sclRct);
    if( !DRect(scl_rct).Intersects(CR::DeviceToUser(cont, drwRct)) )
        return;

    // 1 заполняем рамку градиентом
    //CR::Color clr(SCALE_CLR); // внутри шкалы
    CR::Color clr(GetBGColor(trkLay)); // внутри шкалы
    //cr->fill_preserve();
    FillScaleGradient(cont, scl_rct, clr);

    CR::SetColor(cont, GetBorderColor(trkLay));
    cont->stroke_preserve();
    cont->clip();

    scl_rct = EnlargeRect(scl_rct, Point(-1,-1)); // содержимое шкалы
    int scl_wdh = scl_rct.Width();

    // 2 Шкала
    // 
    // Обозначения:
    //  s - длина кадра в пикселах
    //  q - длина одного периода в кадрах
    //  t - -||- в пикселах
    //  r - зазор между временными отметками
    //  k - длина отметки ("00:00:00;00")
    // 
    // Требования:
    //  1) надписи времени над отметками не должны находить друг на друга, и не должны
    //   сильно расходится (q_l*k <= r <= q_h*k)
    //  2) при масштабировании q не должен меняться постоянно, чтобы показать "растягиваимость"
    //   шкалы;
    //  3) q должно быть целым, чтобы отметки совпадали с границами кадров.
    // 
    // Найдем подходящую функцию q(s):
    //  q_l*k <= t-k <= q_h*k
    //  t = s*q
    //  (q_l+1)*k <= s*q(s) <= (q_h+1)*k
    //  (q_l+1)*k/s <= q(s) <= (q_h+1)*k/s
    //
    // Следовательно, нужна "малопрерывистая" функция с поведением как у обратной пропорциональности :)
    // 
    // Легко заметить ("математическое (с)"), что подходящей функцией будет со значениями из геометрической
    // последовательности, причем с ко?ффициентом high_q/low_q, где  high_q = q_h+1, low_q = q_l+1. По остальным
    // требованиям получаем, что проходит последовательность степеней 2, и при q_l=1/2 => q_h=2
    //

    // 2.1 горизонтальная линия
    int line_hgt = GetSclLevel();
    cont->move_to(scl_rct.lft, line_hgt+H_P);
    cont->line_to(scl_rct.rgt, line_hgt+H_P);
    cont->stroke();

    // 2.2 отметки со временем
    // high больше ровно в 2 раза
    const double low_q = 3.0/2.0;
    const double high_q = low_q*2;
    Point txt_sz = GetTimeMarkSize(GetDPI(trkLay));

    double q = high_q*txt_sz.x/trkLay.FrameScale();
    int rnd_q = GetLowPower2((int)q);
    // для мелких значений будет всегда 1
    if( rnd_q<1 )
        rnd_q = 1;

    // длина периода с числовой меткой
    double t = trkLay.FramesSz(rnd_q);
    //double shift = trkLay.TrkHScroll().get_value();

    RefPtr<Pango::Layout> lay = Pango::Layout::create(cont);
    Pango::FontDescription dsc(MarkFontDsc);
    lay->set_font_description(dsc);
    std::string mark_str;
    

    int l_num = rnd_q > 8 ? 8 : rnd_q ; // кол-во отметок в одном периоде
    double l_t = t/l_num;

    int big_line = 17; // высота большой отметки
    int beg = int(shift.x/t);
    int end = int((scl_wdh+shift.x)/t)+1;
    for( int i=beg; i<=end; i++ )
    {
        double pos = i*t - shift.x;

        for( int j=0; j<l_num; j++, pos += l_t )
        {
            if( j == 0 )
            {
                int line_y = line_hgt-big_line/2;

                // отметка периода
                PaintMark(cont, pos, line_y, big_line);

                // текст
                int txt_gap = 2; // зазор между чертой и меткой
                cont->move_to(Round(pos-txt_sz.x/2.0), line_y-txt_sz.y-txt_gap);
                //lay->set_text("00:00:00;00");
                FramesToTime(mark_str, i*rnd_q, trkLay.FrameFPS());
                lay->set_text(mark_str);
                show_in_cairo_context(lay, cont);
            }
            else
            {
                int small_line = j%2 ? 5 : 11 ;
                PaintMark(cont, pos, line_hgt-small_line/2, small_line);
            }

        }
    }

    // 3 dvd-метки и указатель
    FormHandlers();
}

void RenderSvc::ProcessPointer(const Point& pos)
{
    //CairoStateSave save(cr);
    // отрисовываем курсор с прозрачностью
    CairoAlphaPaint cap(cont, 0.7);
    PaintPointer(pos, false);
}

namespace {
const double q_3 = 1.7321; // sqrt(3)

const double thin_pointer  = 0.3;
// const double thick_pointer = 0.0;
// const double ultra_pointer = 0.6;
}

static void AbsPaintPointer(CR::RefPtr<CR::Context> cr, bool form_only)
{
    double dcl = thin_pointer;

    cr->move_to(0.0, q_3);
    cr->curve_to(-0.5, q_3/2.0,
                 -1.0, q_3-dcl,
                 -0.5, q_3/2.0);

    cr->arc(0.0, q_3, 1.0, 4.0/3.0*M_PI, 10.0/6.0*M_PI);

    cr->curve_to(1.0, q_3-dcl,
                 0.5, q_3/2.0,
                 0.0, q_3);

    if( form_only )
      return;

    // заполняем градиентом
    CR::RefPtr<Cairo::LinearGradient> pat = Cairo::LinearGradient::create(0, 0.0, 0, q_3);
    // :TODO: высчитывать затененные цвета, а не выставлять явно,
    // используя ge_shade_color() 
    pat->add_color_stop_rgba(0, 0.1, 0.2, 0.3, 1);
    pat->add_color_stop_rgba(0.58, 0.9, 0.9, 1.0, 1);
    pat->add_color_stop_rgba(1, 0.1, 0.2, 0.3, 1);

    cr->set_source(pat);
    cr->fill_preserve();

    cr->set_source_rgb(0,0,0);
    cr->stroke();
}

void PaintPointer(CR::RefPtr<CR::Context> cr, DPoint pos, Point sz, bool form_only)
{
    cr->translate(pos.x, pos.y);

    cr->scale(sz.x/2.0, sz.y);
    cr->translate(0, -q_3);
    cr->set_line_width(0.02);

    AbsPaintPointer(cr, form_only);
}

void PaintPointer(CR::RefPtr<CR::Context> cr, DPoint pos, bool form_only)
{
    PaintPointer(cr, pos, Point(20, 17) /*Point(25, 22)*/, form_only);
}

} //namespace Timeline

