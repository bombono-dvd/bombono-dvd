//
// mgui/win_utils.cpp
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

#include "win_utils.h"
#include "dialog.h"

#include <mgui/sdk/packing.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/widget.h> // ForAllWidgets()
#include <mgui/gettext.h>

#include <mbase/project/table.h> // Project::ConvertPathToUtf8()
#include <mbase/resources.h>
#include <mlib/filesystem.h>     // fs::exists()

#include <gtk/gtklabel.h> // GTK_IS_LABEL()


#define GetStyleColor_Impl(ClrType, ClrName)    \
RGBA::Pixel GetStyleColor ## ClrName(Gtk::StateType typ, Gtk::Widget& wdg)  \
{ \
    GtkStyle* style = wdg.get_style()->gobj();  \
    GdkColor* clr_pack = style->ClrType;        \
  \
    GdkColor& clr = clr_pack[typ];              \
    return RGBA::Pixel( GColorToCType(clr.red), GColorToCType(clr.green), GColorToCType(clr.blue) );    \
} \
/**/

GetStyleColor_Impl(fg,    Fg)
GetStyleColor_Impl(bg,    Bg)
GetStyleColor_Impl(light, Light)
GetStyleColor_Impl(dark,  Dark)
GetStyleColor_Impl(mid,   Mid)
GetStyleColor_Impl(text,  Text)
GetStyleColor_Impl(base,  Base)
GetStyleColor_Impl(text_aa, Text_aa)

void ge_shade_color(const CR::Color *base, gdouble shade_ratio, CR::Color *composite);

namespace CR
{

const double Color::MinClr = 0.0;
const double Color::MaxClr = 1.0;

Color::Color(const unsigned int rgba)
{
    RGBA::Pixel pxl(rgba);
    FromPixel(pxl);
}

Color::Color(const RGBA::Pixel& pxl)
{
    FromPixel(pxl);
}

Color& Color::FromPixel(const RGBA::Pixel& pxl)
{
    using namespace RGBA;
    r = Pixel::FromQuant(pxl.red); 
    g = Pixel::FromQuant(pxl.green); 
    b = Pixel::FromQuant(pxl.blue); 
    a = Pixel::FromQuant(pxl.alpha); 

    return *this;
}

Color ShadeColor(const Color& clr, double shade_rat)
{
    Color res;
    ge_shade_color(&clr, shade_rat, &res);

    return res;
}

// SetColor
void SetColor(RefPtr<Context>& cr, const RGBA::Pixel& pxl)
{
    cr->set_source_rgba( pxl.FromQuant(pxl.red),  pxl.FromQuant(pxl.green),
                         pxl.FromQuant(pxl.blue), pxl.FromQuant(pxl.alpha) );
}

void SetColor(RefPtr<Context>& cr, const unsigned int rgba)
{
    RGBA::Pixel pxl(rgba);
    SetColor(cr, pxl);
}

void SetColor(RefPtr<Context>& cr, const Color& clr)
{
    cr->set_source_rgba( clr.r, clr.g, clr.b, clr.a );
}

void Scale(RefPtr<Context> cr, RefPtr<ImageSurface> src,
           const Rect& plc)
{
    CairoStateSave save(cr);
    Point sz = plc.Size();

    cr->translate(plc.lft, plc.top);
    cr->scale((double)sz.x/src->get_width(), (double)sz.y/src->get_height());

    cr->set_source(src, 0, 0);
    cr->paint();
}

} // namespace CR

std::string ColorToString(const unsigned int rgba)
{
    return (str::stream() << std::hex << (rgba >> 8)).str();
}

CR::Color GetBGColor(Gtk::Widget& wdg)
{
    return CR::Color( GetStyleColorBg(Gtk::STATE_NORMAL, wdg) );
}

CR::Color GetBorderColor(Gtk::Widget& wdg)
{
    CR::Color clr = GetBGColor(wdg);
    // в теме Clearlooks цвет границы берется как CairoColor::shade[6],
    // который в свою очередь есть затемненный до 0.475 фон
    return CR::ShadeColor(clr, 0.475);
}

void SetCursorForWdg(Gtk::Widget& wdg, Gdk::Cursor* curs)
{
    RefPtr<Gdk::Window> p_win = wdg.get_window();
    if( p_win )
    {
        if( curs )
            p_win->set_cursor(*curs);
        else
            p_win->set_cursor();
    }
}

void SetDefaultButton(Gtk::Button& btn)
{
    btn.property_can_default() = true;
    // иначе не работает
    ASSERT( btn.has_screen() );
    btn.grab_default();
}

Gtk::Tooltips& TooltipFactory()
{
    // :LEAK: возможно утечка (valgrind) - не такой виджет, как все?
    // проверить "большим кол-вом"
    static Gtk::Tooltips* tips = 0;
    if( !tips )
        tips = Gtk::manage(new Gtk::Tooltips);
    return *tips;
}

Gtk::Frame& NewManagedFrame(Gtk::ShadowType st, const std::string& label)
{
    Gtk::Frame& fram = NewManaged<Gtk::Frame>();
    fram.set_shadow_type(st);
    if( label.size() )
        fram.set_label(label);
    return fram;
}

Gtk::Frame& PackWidgetInFrame(Gtk::Widget& wdg, Gtk::ShadowType st, const std::string& label)
{
    Gtk::Frame& fram = NewManagedFrame(st, label);

    fram.add(wdg);
    return fram;
}

// символы вроде "&<>#" перед передачей GMarkupParser должны быть заменены
// на escape-последовательности
static std::string QuoteForGMarkupParser(const std::string& str)
{
    return Glib::Markup::escape_text(str).raw();
}

std::string MakeMessageBoxTitle(const std::string& title)
{
    return "<span weight=\"bold\">" + title + "</span>";
}

Gtk::ResponseType MessageBoxEx(const std::string& msg_str, Gtk::MessageType typ,
                               Gtk::ButtonsType b_typ, const std::string& desc_str, const MDFunctor& fnr)
{
    std::string markup_msg_str = MakeMessageBoxTitle(QuoteForGMarkupParser(msg_str));

    Gtk::MessageDialog mdlg(markup_msg_str, true, typ, b_typ);
    if( !desc_str.empty() )
        mdlg.set_secondary_text(desc_str, true);

    if( fnr )
        fnr(mdlg);

    return (Gtk::ResponseType)mdlg.run();
}

static void SetOKDefault(Gtk::MessageDialog& mdlg)
{
    mdlg.set_default_response(Gtk::RESPONSE_OK);
}

Gtk::ResponseType MessageBox(const std::string& msg_str, Gtk::MessageType typ,
                             Gtk::ButtonsType b_typ, const std::string& desc_str, bool def_ok)
{
    //std::string markup_msg_str = MakeMessageBoxTitle(QuoteForGMarkupParser(msg_str));
    //
    //Gtk::MessageDialog mdlg(markup_msg_str, true, typ, b_typ);
    //if( !desc_str.empty() )
    //    mdlg.set_secondary_text(desc_str, true);
    //if( def_ok )
    //    mdlg.set_default_response(Gtk::RESPONSE_OK);
    //
    //if( fnr )
    //    fnr(mdlg);
    //
    //return (Gtk::ResponseType)mdlg.run();

    return MessageBoxEx(msg_str, typ, b_typ, desc_str, def_ok ? SetOKDefault : MDFunctor());
}

void SetTip(Gtk::Widget& wdg, const char* tooltip)
{
    if( tooltip && *tooltip )
        TooltipFactory().set_tip(wdg, tooltip);
}

Gtk::Button* CreateButtonWithIcon(const char* label, const Gtk::BuiltinStockID& stock_id,
                                  const char* tooltip, Gtk::BuiltinIconSize icon_sz)
{
    Gtk::Button& btn = NewManaged<Gtk::Button>(label);
    Gtk::Image& img  = NewManaged<Gtk::Image>(stock_id, icon_sz);
    btn.set_image(img);
    // убираем влияние настройки gtk-button-images после включения в кнопку
    //img.set_visible();
    img.property_visible() = true;

    SetTip(btn, tooltip);
    return &btn;
}

void AddCancelDoButtons(Gtk::Dialog& dialog, Gtk::BuiltinStockID do_id)
{
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(do_id, Gtk::RESPONSE_OK);
    dialog.set_default_response(Gtk::RESPONSE_OK);
}

static void OnMenuSelectionDone(Gtk::Menu* mn)
{
    delete mn;
}

void SetDeleteOnDone(Gtk::Menu& menu)
{
    menu.signal_selection_done().connect(bb::bind(OnMenuSelectionDone, &menu));
}

Gtk::Menu& NewPopupMenu()
{
    Gtk::Menu& menu = NewManaged<Gtk::Menu>();
    SetDeleteOnDone(menu);
    return menu;
}

void BuildChooserDialog(Gtk::FileChooserDialog& dialog, bool is_open, Gtk::Widget& for_wdg)
{
    Gtk::Window* for_win = GetTopWindow(for_wdg);
    ASSERT( for_win );

    dialog.set_transient_for(*for_win);
    // кнопки
    AddCancelDoButtons(dialog, is_open ? Gtk::Stock::OPEN : Gtk::Stock::SAVE );
}

bool ChooseFileSaveTo(std::string& fname, const std::string& title, Gtk::Widget& for_wdg,
                      const FCDFunctor& fnr)
{
    Gtk::FileChooserDialog dialog(title, Gtk::FILE_CHOOSER_ACTION_SAVE);
    BuildChooserDialog(dialog, false, for_wdg);

    dialog.set_current_name(fname);
    if( fnr )
        fnr(dialog);

    bool res;
    for( ; res = Gtk::RESPONSE_OK == dialog.run(), res; )
    {
        fname = dialog.get_filename();
        if( CheckKeepOrigin(fname) )
            continue;

        break;
    }

    return res;
}

bool CheckKeepOrigin(const std::string& fname)
{
    bool res = false;
    if( fs::exists(fname) && 
        (Gtk::RESPONSE_OK != MessageBox(BF_("A file named \"%1%\" already exists. Do you want to replace it?")
                                        % fs::path(fname).leaf() % bf::stop,
                                        Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL, 
                                        _("Replacing the file overwrite its contents."),
                                        true)) )
        res = true;
    return res;
}

void InitGtkmm(int argc, char** argv)
{
    static ptr::one<Gtk::Main> si;
    if( !si )
    {
        si = new Gtk::Main(argc, argv);
        // стили виджетов программы
        fs::path rc_fpath = fs::path(GetDataDir())/"gtkrc";
        gtk_rc_parse(rc_fpath.string().c_str());
    }
}

static gboolean ActivateLink(GtkWidget* /*label*/, const gchar* uri, gpointer)
{
    gboolean ret = FALSE;
    if( strncmp(uri, "http://", ARR_SIZE("http://")-1) == 0 )
    {
        void GoUrl(const gchar* url);
        GoUrl(uri);

        ret = TRUE;
    }

    return ret;
}

static void ActivateLinksFor2Label(GtkWidget* wdg, int& label_num)
{
    if( GTK_IS_LABEL(wdg) )
    {
        //io::cout << "Found Label!" << gtk_label_get_text(GTK_LABEL(wdg)) << io::endl;

        label_num++;
        if( label_num == 2 ) // 2-я по счету
        {
            // :KLUDGE: при открытии диалога, в вызове gtk_dialog_map() производится установка
            // фокуса на первый виджет, за исключением меток. Все бы ничего, только "код избегания"
            // меток фатален (бесконечный цикл), если метка содержит ссылки (с тегом <a>). Нашел 
            // наиболее простое решение - убрать фокус со злополучной метки
            //gtk_label_set_selectable(GTK_LABEL(wdg), FALSE);
            //gtk_widget_set_can_focus(wdg, FALSE);
            g_object_set(wdg, "can-focus", FALSE, NULL);

            g_signal_connect(wdg, "activate-link", G_CALLBACK (ActivateLink), NULL);
        }
    }
}

void SetWeblinkCallback(Gtk::MessageDialog& mdlg)
{
    // устанавливаем действие при нажатии на ссылку во второй метке
    // для этого его сначала нужно найти
    // 
    // :KLUDGE: идейно правильный вариант заключается в реализации нового MessageDialog
    // на основе Gtk::Dialog; при этом придется копировать стандартный функционал MessageDialog
    //
    int label_num = 0;
    ForAllWidgets(static_cast<Gtk::Widget&>(mdlg).gobj(), bb::bind(&ActivateLinksFor2Label, _1, boost::ref(label_num)));
}

Gtk::Alignment& NewPaddingAlg(int top, int btm, int lft, int rgt)
{
    Gtk::Alignment& alg = NewManaged<Gtk::Alignment>();
    alg.set_padding(top, btm, lft, rgt);
    return alg;
}

void CompleteDialog(Gtk::Dialog& dlg, bool close_style)
{
    if( close_style )
        dlg.add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);
    else
        AddCancelDoButtons(dlg, Gtk::Stock::OK);
    dlg.get_vbox()->show_all();
}

Gtk::Label& NewMarkupLabel(const std::string& label, bool use_underline)
{
    Gtk::Label& lbl = NewManaged<Gtk::Label>(label);
    lbl.set_use_markup(true);
    lbl.set_use_underline(use_underline);
    return lbl;
}

