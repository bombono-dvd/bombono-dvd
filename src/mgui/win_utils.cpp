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
#include <mgui/prefs.h> // fcMap
#include <mgui/gettext.h>
#include <mgui/timeline/mviewer.h> // FileFilterList
#include <mgui/sdk/win32filesel.h> // CallWinFileDialog

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
    str::stream ss;
    ss << std::hex << (rgba >> 8);
    return ss.str();
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
std::string QuoteForGMarkupParser(const std::string& str)
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
    mdlg.set_default_response(Gtk::RESPONSE_YES);
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
    Gtk::Image& img  = NewManaged<Gtk::Image>(stock_id, icon_sz);
    Gtk::Button* btn = 0;
    if( label && *label )
    {
        btn = &NewManaged<Gtk::Button>(label);
        btn->set_image(img);
        // убираем влияние настройки gtk-button-images после включения в кнопку
        //img.set_visible();
        img.property_visible() = true;
    }
    else
    {
        btn = &NewManaged<Gtk::Button>();
        btn->add(img);
    }

    SetTip(*btn, tooltip);
    return btn;
}

void AddCancelDoButtons(Gtk::Dialog& dialog, Gtk::BuiltinStockID do_id)
{
    // в стандартных диалогах вроде Открыть/Сохранить порядок
    // кнопок определяется настройкой gtk-alternative-button-order = [1|0]
#ifdef _WIN32
    dialog.add_button(do_id, Gtk::RESPONSE_OK);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
#else
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(do_id, Gtk::RESPONSE_OK);
#endif
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

bool RunFileDialog(const char* title, bool open_file, Str::List& chosen_paths, 
                   Gtk::Widget& for_wdg, const FileFilterList& pat_lst,
                   bool multiple_choice, const FCDFunctor& fnr)
{
    bool res;
#ifdef _WIN32
    if( !fnr )
        // используем нативный диалог, по возможности
        // :TODO: соответственно, кастомизировать диалоги пока невозможно
        res = CallWinFileDialog(title, open_file, chosen_paths, &for_wdg, pat_lst, multiple_choice);
    else
#endif
    {
        Gtk::FileChooserDialog dialog(title, open_file ? Gtk::FILE_CHOOSER_ACTION_OPEN : Gtk::FILE_CHOOSER_ACTION_SAVE);
        
        FCMap& fc_map = UnnamedPrefs().fcMap;
        // :TODO: (субъективно) чтобы локализация title не мешала, необходимо
        // gettext() делать здесь, а при вызове,- C_()
        FCMap::iterator itr = fc_map.find(title);
        int last_flt = 0;
        if( itr != fc_map.end() )
        {
            FCState& fcs = itr->second;
            dialog.set_current_folder(fcs.lastDir);
            last_flt = fcs.lastFilter;
        }

        if( chosen_paths.size() )
            dialog.set_current_name(chosen_paths[0]);

        BuildChooserDialog(dialog, open_file, for_wdg);

        int idx = 0;
        boost_foreach( const FileFilter& ff, pat_lst )
        {
            // :TRICKY: если на стеке создать, то повторного доступа к С++-оболочкам
            // потом не будет
            Gtk::FileFilter& gff = NewManaged<Gtk::FileFilter>();
            gff.set_name(ff.title);
            boost_foreach( const std::string& glob_pat, ff.extPatLst )
            {
                //gff.add_pattern(ext);
                AddGlobFilter(gff, glob_pat);
            }

            dialog.add_filter(gff);
            if( last_flt && (idx == last_flt) )
                dialog.set_filter(gff);
            idx++;
        }

        if( fnr )
            fnr(dialog);
        dialog.set_select_multiple(multiple_choice);

        std::string fname;
        for( ; res = Gtk::RESPONSE_OK == dialog.run(), res; )
        {
            fname = dialog.get_filename();
            if( !open_file && CheckKeepOrigin(fname) )
                continue;

            chosen_paths = GetFilenames(dialog);
            break;
        }
        
        // сохраняем последние настройки
        if( res )
        {
            // приходиться перебором находить
            last_flt = 0;
            Gtk::FileFilter* cur_ff = dialog.get_filter();
            boost_foreach( const Gtk::FileFilter* ff, dialog.list_filters() )
            {
                if( cur_ff == ff )
                    break;
                last_flt++;
            }
            fc_map[title] = FCState(dialog.get_current_folder(), last_flt);
        }
    }

    return res;
}

bool ChooseFileSaveTo(std::string& fname, const std::string& title, Gtk::Widget& for_wdg,
                      const FCDFunctor& fnr)
{
    Str::List paths;
    paths.push_back(fname);
    bool res = RunFileDialog(title.c_str(), false, paths, for_wdg, FileFilterList(), false, fnr);
    if( res )
        fname = paths[0];
    return res;
}

bool CheckKeepOrigin(const std::string& fname)
{
    bool res = false;
    if( fs::exists(fname) && 
        (Gtk::RESPONSE_OK != MessageBox(BF_("A file named \"%1%\" already exists. Do you want to replace it?")
                                        % fs::name_str(fname) % bf::stop,
                                        Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL, 
                                        _("Replacing the file overwrite its contents."),
                                        true)) )
        res = true;
    return res;
}

static void PresentGtkmmedException(const std::string& reason)
{
    const char* title = "Unhandled Exception within GTK Signal";
    std::string text = reason +
        "\n\n"
        // :TODO: refactor
        "To help us diagnose and fix the problem please send this text "
        "to support@bombono.org (use Ctrl+A, Ctrl+C to copy; Ctrl+V to paste)."
        "\n\n"
        "The program's operation may be unstable until restart.";

    io::cout << title << io::endl;
    io::cout << text << io::endl;

    // :TODO: очень опасно запускать нативный диалог, ведь события
    // перерисовки скомпрометированного приложения, например, все равно 
    // будут проходить во время его отображения; потому надо:
    // - лучше всего запускать отдельный процесс с диалогом, а приложение пусть ждет
    // - под Linux есть Zenity для простых диалогов
    // - для Win можно создать отдельный поток с ожиданием и WinMsgBox(), как для OnMinidumpCallback()
#ifdef _WIN32
    WinMsgBox(text, title);
#else
    ErrorBox(title, text);
#endif
}

std::string GlibError2Str(const Glib::Error& err)
{
    const GError* error = err.gobj();
    ASSERT_RTL( error );

    std::string err_text = boost::format(
        "Type: Glib::Error\n"
        "Domain: %s\n"
        "Code: %d\n"
        "What: %s")
        % g_quark_to_string(error->domain) % error->code
        % ((error->message) ? error->message : "(null)") % bf::stop;
    return err_text;
}

// по мотиву glibmm_unexpected_exception()
static void OnGtkmmedException()
{
    // :TODO: хотелось бы не только ловить С++-исключения, но
    // и место их возникновения; однако есть проблемы:
    // - без замены всех первичных catch(...) в gtkmm на
    //   __except( DumpException(GetExceptionInformation()) )
    //   нельзя получить доступ к контексту возникновения (а это кучи мест;
    //   и хоть все они затем вызывают наш OnGtkmmedException(), время уже упущено)
    // - другая фундаментальная проблема - "либо C++, либо SEH (но не вместе)",
    //   см. test_breakpad()
    try
    {
        throw; // заново бросаем для конкретизации типа исключения
    }
    catch(const Glib::Error& err)
    {
        PresentGtkmmedException(GlibError2Str(err));
    }
    catch(const std::exception& except)
    {
        PresentGtkmmedException(std::string(
            "Type: std::exception\n"
            "What: ") + except.what());
    }
    catch(...)
    {
        PresentGtkmmedException("Type: unknown");
    }
}

void InitGtkmm(int argc, char** argv)
{
    static ptr::one<Gtk::Main> si;
    if( !si )
    {
        si = new Gtk::Main(argc, argv);

        Glib::add_exception_handler(&OnGtkmmedException);

        // стили виджетов программы
        gtk_rc_parse(DataDirPath("gtkrc").c_str());
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
        AddCancelOKButtons(dlg);
    dlg.get_vbox()->show_all();
}

Gtk::Label& NewMarkupLabel(const std::string& label, bool use_underline)
{
    Gtk::Label& lbl = NewManaged<Gtk::Label>(label);
    lbl.set_use_markup(true);
    lbl.set_use_underline(use_underline);
    return lbl;
}

std::string BoldItalicText(const std::string& txt)
{
    return "<span weight=\"bold\" style=\"italic\">" + txt + "</span>";
}

Gtk::Label& NewBoldItalicLabel(const std::string& label, bool use_underline)
{
    return NewMarkupLabel(BoldItalicText(label), use_underline);
}

