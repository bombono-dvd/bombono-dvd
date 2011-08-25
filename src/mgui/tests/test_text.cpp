//
// mgui/tests/test_text.cpp
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

#include <mgui/tests/_pc_.h>

#include <mgui/editor/text.h> 

#include <mgui/win_utils.h>
#include <mgui/design.h>
#include <mgui/sdk/window.h>

#include <pangomm/renderer.h>
#include <pango/pangocairo.h>
#include <math.h>

typedef bool (*DAExposeFunc)(Gtk::DrawingArea& da, GdkEventExpose* event);

class ExampleDA: public Gtk::DrawingArea
{
    public:
                 ExampleDA(DAExposeFunc ef): eF(ef) {}
   virtual bool  on_expose_event(GdkEventExpose* event);

    protected:
            DAExposeFunc eF;
};

bool ExampleDA::on_expose_event(GdkEventExpose* event)
{
    return eF(*this, event);
}

void TestExampleDA(DAExposeFunc ef)
{
    InitGtkmm();

    ExampleDA da(ef);

    Gtk::Window win;
    win.set_default_size(400, 400);
    win.add(da);

    RunWindow(win);
}

// Служебный класс для считывания агрументов из файла
class FileCmdLine
{
    public:
                    FileCmdLine(const char* fpath);
                   ~FileCmdLine();

               int  Count() { return arr.size(); }
             char** Words() { return words; }

    protected:
                  char** words;
      std::vector<char*> arr;
};

FileCmdLine::FileCmdLine(const char* fpath): words(0)
{
    io::stream strm(fpath, iof::in);
    std::string str;
    while( strm >> str )
    {
        char* word = new char[str.length() + 1];
        strcpy(word, str.c_str());
        arr.push_back(word);
    }

    if( Count() )
    {
        words = new char*[Count()];
        for( int i=0; i<Count(); i++ )
        {
            words[i] = arr[i];
        }
    }
}

FileCmdLine::~FileCmdLine()
{
    for( int i=0; i<Count(); i++ )
        delete [] arr[i];

    delete [] words;
}

Gtk::Widget* GetFocusWidget(Gtk::Widget& some_wdg)
{
    Gtk::Widget* res_wdg = 0;
    if( Gtk::Window* win = GetTopWindow(some_wdg) )
        res_wdg = win->get_focus();
    return res_wdg;
}

void PrintWidgetPath(Gtk::Widget& searched_wdg)
{
    io::cout << "Focus Widget Reverse Path: " << searched_wdg.get_name() << io::endl;
    for( Gtk::Widget* wdg = &searched_wdg; wdg = wdg->get_parent(), wdg; )
        io::cout << " -> " << wdg->get_name();
    io::cout << io::endl;
}

//////////////////////////////////////////////////////////

// для тестов
struct TestTextRenderer: public TextRenderer,
                         public TextObj
{
    RefPtr<Gdk::Pixbuf> canvPix;
    Planed::Transition  trans;

                        // TextRenderer
    virtual       void  DrawForRegion(RectListRgn&) { }
    virtual       void  RenderBegin(const Rect&) { }
    virtual       void  RenderEnd(const Rect&) { }
   virtual     TextObj& GetTextObj() { return *this; }
   virtual OwnerWidget* GetOwnerWidget() { return 0; }
   virtual const Planed::Transition& GetTransition() { return trans; }

//     virtual       void  SetPlacement(const Rect& rct)
//                         {
//                             mdPlc = rct;
//                         }

};

static char TestString[] = "Preved, Medved";

BOOST_AUTO_TEST_CASE( TestTextChange )
{
    InitGtkmm();
    //Gtk::Main main_instance(0, 0);

    TextObj tobj;
    tobj.SetText(TestString);

    Rect rct = tobj.Placement();
    rct.SetHeight( Round(rct.Height()*2.66) );
    tobj.SetPlacement(rct);

    tobj.SetText(tobj.Text());
    BOOST_CHECK_EQUAL( rct.Height(), tobj.Placement().Height() );
}

/////////////////////////////////////////////////////////////////////////
// TextWidget

class TextWidget: public Gtk::DrawingArea, 
                  public TextRenderer,
                  protected ImageCanvas
{
    typedef TextRenderer MyTextParent;
    public:

                      TextWidget();

        virtual bool  on_configure_event(GdkEventConfigure* event);
        virtual bool  on_expose_event(GdkEventExpose* event);
        virtual bool  on_focus_in_event(GdkEventFocus* event);
        virtual bool  on_focus_out_event(GdkEventFocus* event);
        virtual bool  on_key_press_event(GdkEventKey* event);
        virtual bool  on_button_press_event(GdkEventButton* event);

      virtual   void  DrawForRegion(RectListRgn& r_lst);

    protected:

               TextObj  txtObj;

    Planed::Transition  trans;

                 Point  absSz;   // геом. данные

      virtual   void  RenderBegin(const Rect& rct);
      virtual   void  RenderEnd(const Rect& rct);

                     void  PaintRegion(const Rect& rct);

          virtual TextObj& GetTextObj() { return txtObj; }
virtual const Planed::Transition& GetTransition() { return trans; }
      virtual OwnerWidget* GetOwnerWidget() { return this; }

            virtual  void  DoLayout();
            virtual  Rect  CalcTextPlc();
                           // ImageCanvas
             virtual void  ClearPixbuf() { TextRenderer::Clear(); }
             virtual void  InitPixbuf()  { TextRenderer::Init(CanvasPixbuf()); }
};


TextWidget::TextWidget(): absSz(300, 300), ImageCanvas(10, 5)
{
    txtObj.SetFontDesc("Sans Bold 18");

//     // яйцо
//     char rus_txt[19] =
//     {
//         0xD1, 0x8F, 'a', 0xD0, 0xB9, 0xD1, 0x86, 0xD0, 0xBE,
//         0x0A,
//         0xD1, 0x8F, 0xD0, 0xB9, 0xD1, 0x86, 0xD0, 0xBE,
//         0
//     };

    txtObj.SetText(TestString);//\nHello, comrades.");
    //txtObj.SetText(rus_txt);
    txtObj.SetLocation( Point(10, 130) );
    RGBA::Pixel clr(GOLD_CLR); // желтый
    txtObj.SetColor(clr);


    property_can_focus() = true;
    curPos = 1;
    ShowCursor(true);

    add_events(Gdk::EXPOSURE_MASK |Gdk::STRUCTURE_MASK |  // прорисовка и изменение размеров
               Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK );      // кнопки мыши


    // для проверки
    //get_window()->set_debug_updates();
}

void TextWidget::DoLayout()
{
    UpdatePixbuf(CalcTextPlc().Size(), false);
    MyTextParent::DoLayout();
}

Rect TextWidget::CalcTextPlc()
{
    Rect plc = MyTextParent::CalcTextPlc();
    return Planed::DevToRel(trans, plc);
}

bool TextWidget::on_configure_event(GdkEventConfigure* event)
{
    trans = Planed::Transition(Rect(0, 0, event->width, event->height), absSz);
    Rect plc = RelPos(GetTextObj(), trans);

    //trans = Planed::Transition(plc, txtPlc.Size());
    trans.SetShift(plc.A());

    DoLayout();
    return true;
}

bool TextWidget::on_focus_in_event(GdkEventFocus* /*event*/)
{
    OnFocusInEvent();
    return true;
}

bool TextWidget::on_focus_out_event(GdkEventFocus* /*event*/)
{
    OnFocusOutEvent();
    return true;
}

bool TextWidget::on_key_press_event(GdkEventKey* event)
{
//     io::cout << gdk_keyval_name(event->keyval) << io::endl;
//     io::cout << "state " << event->state << io::endl;
//     io::cout << "raw_key " << event->hardware_keycode << io::endl;

    OnKeyPressEvent(event);
    return true;
}

bool TextWidget::on_button_press_event(GdkEventButton* event)
{
    OnButtonPressEvent(event);
    return false;
}

static void ClearCanvas(Cairo::RefPtr<Cairo::Context>& cr)
{
    CairoStateSave save(cr);

    cr->set_operator(Cairo::OPERATOR_CLEAR);
    cr->paint();
}

// для TextWidget они только ограничивают область вывода,
// потому что отрисовка из памяти - общая для текста и курсора
void TextWidget::RenderBegin(const Rect& rct)
{
    CR::RectClip(caiCont, rct);
}

void TextWidget::RenderEnd(const Rect&)
{}


void TextWidget::PaintRegion(const Rect& rct)
{
    RefPtr<Gdk::Pixbuf> pix = CanvasPixbuf();
    Rect drw_rct = Intersection(rct, PixbufBounds(pix));
    if( drw_rct.IsNull() )
        return;

    AlignCairoVsPixbuf(pix, drw_rct);

    RefPtr<Gdk::Window> p_win = get_window();
    Rect dst_rct = Planed::RelToDev(trans, drw_rct);
    p_win->draw_pixbuf(get_style()->get_black_gc(), CanvasPixbuf(),
                       drw_rct.lft, drw_rct.top, 
                       dst_rct.lft, dst_rct.top, dst_rct.Width(), dst_rct.Height(), 
                       Gdk::RGB_DITHER_NORMAL, 0, 0);
}

bool TextWidget::on_expose_event(GdkEventExpose* event)
{
    Rect drw_rct = Planed::DevToRel(trans, MakeRect(event->area));
    CairoStateSave save(caiCont);

    CR::RectClip(caiCont, drw_rct);
    ClearCanvas(caiCont);

    // собственно отрисовка
    RenderByRegion(drw_rct);

    PaintRegion(drw_rct);
    return false;
}

void TextWidget::DrawForRegion(RectListRgn& rct_lst)
{
    Point sht = trans.GetShift();
    for( RLRIterType cur=rct_lst.begin(), end=rct_lst.end(); cur != end; ++cur )
    {
        Rect& r = *cur;
        queue_draw_area( sht.x+r.lft, sht.y+r.top, r.rgt-r.lft, r.btm-r.top );
    }
}

void
DoTextWidget()
{
    InitGtkmm();

    Gtk::Window win;
    TextWidget t_wdg;

    Gdk::Color white;
    white.set_rgb(0x0, 0x0, 0xffff);

    win.set_title("TextWidget");

    t_wdg.modify_bg(Gtk::STATE_NORMAL, white);

    win.set_default_size(300, 300);
    win.add(t_wdg);

    RunWindow(win);
}

BOOST_AUTO_TEST_CASE( _DoTextWidget_ )
{
    //DoTextWidget();
}

/////////////////////////////////////////////////////////////////////////
// ScaledText

#define RADIUS 150
#define N_WORDS 10
#define FONT "Sans Bold 27"

static void
draw_text (Cairo::RefPtr<Cairo::Context> cr, int wdh, int hgt)
{
    RefPtr<Pango::Layout> layout = Pango::Layout::create(cr);
    //layout->set_single_paragraph_mode(true);
    layout->set_text("MTextTextM\nAbc\nff");


    Pango::FontDescription dsc(FONT);
    layout->set_font_description(dsc);

    int t_wdh, t_hgt;
    layout->get_size(t_wdh, t_hgt);

    double t_sz = (double)dsc.get_size()/t_wdh;
    double new_sz = wdh * t_sz ;

    io::cout << "new_sz " << new_sz << io::endl;
    io::cout << "wdh " << wdh << io::endl;

    dsc.set_size( int(new_sz*PANGO_SCALE) );
    layout->set_font_description(dsc);

    layout->get_size(t_wdh, t_hgt);
    io::cout << "t_wdh " << t_wdh/(double)PANGO_SCALE << io::endl;

    // для наглядности
    cr->set_line_width(1.0);
    cr->rectangle(0, 0, wdh, hgt);
    cr->stroke();


    cr->save();

    cr->move_to(0, 0);
    cr->scale( 1.0, hgt / ((double)t_hgt/PANGO_SCALE) );
    //cr->scale( wdh / ((double)t_wdh/PANGO_SCALE), hgt / ((double)t_hgt/PANGO_SCALE) );

    layout->update_from_cairo_context(cr);

    pango_cairo_show_layout(cr->cobj(), layout->gobj());

    {
        Pango::Rectangle w_rct, s_rct;
        int cur_pos;

        cur_pos = 1;
        layout->get_cursor_pos(cur_pos, w_rct, s_rct);
        pango_extents_to_pixels(0, w_rct.gobj());

        io::cout << "curs - x, y, hgt " << w_rct.get_x() << " " << w_rct.get_y() << " " << w_rct.get_height() << io::endl;
        cr->move_to(w_rct.get_x()+5, w_rct.get_y());
        cr->line_to(w_rct.get_x()+5, w_rct.get_y()+w_rct.get_height());
        cr->stroke();


        cur_pos = 11;
        layout->get_cursor_pos(cur_pos, w_rct, s_rct);
        pango_extents_to_pixels(0, w_rct.gobj());

        io::cout << "curs - x, y, hgt " << w_rct.get_x() << " " << w_rct.get_y() << " " << w_rct.get_height() << io::endl;
        cr->move_to(w_rct.get_x()+5, w_rct.get_y());
        cr->line_to(w_rct.get_x()+5, w_rct.get_y()+w_rct.get_height());
        cr->stroke();

    }


    cr->restore();
}


class ScaledText: public Gtk::DrawingArea
{
    public:

                      //ScaledText() {}

        virtual bool  on_expose_event(GdkEventExpose* event);

};

bool ScaledText::on_expose_event(GdkEventExpose* /*event*/)
{
    RefPtr<Gdk::Window> p_win = get_window();
    Cairo::RefPtr<Cairo::Context> cr = p_win->create_cairo_context();

    int wdh = get_width();
    int hgt = get_height();

    draw_text(cr, wdh, hgt);

    return true;
}

void
DoScaledText()
{
    InitGtkmm();

    Gtk::Window win;
    ScaledText t_wdg;

    win.set_title("Scaled Text");
    win.set_border_width(0);

    win.set_default_size(300, 100);
    win.add(t_wdg);
    RunWindow(win);
}

BOOST_AUTO_TEST_CASE( test_scaled_text )
{
    //DoScaledText();
}

/////////////////////////////////////////////////////////////////////////

#if 0
#include <mcomposite/tests/test_common.h>
void SaveAsPPM(RefPtr<Gdk::Pixbuf> pix, const char* name_prefix)
{
    const char* fnam = TmpNam(0, name_prefix);
    io::stream strm(fnam, iof::def|iof::trunc);

    int wdh  = pix->get_width();
    int hgt  = pix->get_height();
    int rowstride = pix->get_rowstride();
    RGBA::Pixel* dat = (RGBA::Pixel*)pix->get_pixels(), *cur;
    BOOST_CHECK( pix->get_has_alpha() );

    strm << "P6\n" << wdh << " " << hgt << "\n255\n";
    for( int y=0; cur=dat, y<hgt; y++, dat = (RGBA::Pixel*)((char*)dat + rowstride) )
        for( int x=0; x<wdh; x++, cur++ )
        {
            strm << cur->red << cur->green << cur->blue;
        }
}

BOOST_AUTO_TEST_CASE( test_colors )
{
    InitGtkmm();

    std::string path = GetTestFileName("colors.png");
    {
        Magick::Image img(path.c_str());
        RefPtr<Gdk::Pixbuf> pix = GetAsPixbuf(img);

        SaveAsPPM(pix, "GM_");
    }

    {
        RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create_from_file(path);

        SaveAsPPM(pix, "Pixbuf_");
    }

    {
        Cairo::RefPtr<Cairo::ImageSurface> sur = Cairo::ImageSurface::create_from_png(path);
        RefPtr<Gdk::Pixbuf> pix = GetAsPixbuf(sur);
        
        SaveAsPPM(pix, "Cairo_");
    }
}
#endif // #if 0

