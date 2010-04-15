//
// mgui/editor/toolbar.cpp
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

#include "toolbar.h"
#include "actions.h"

#include <mgui/win_utils.h>
#include <mgui/img-factory.h>
#include <mgui/theme.h>
#include <mgui/text_obj.h>

#include <mgui/sdk/gnc-gtk-utils.h>
#include <mgui/sdk/entry.h>
#include <mgui/sdk/packing.h>

#include <mgui/render/menu.h>
#include <mgui/render/editor.h>
#include <mgui/project/thumbnail.h>
#include <mgui/gettext.h>

#include <mbase/project/theme.h>
#include <mlib/sdk/logger.h>

namespace Editor
{

void ForAllSelected(FTOFunctor fto_fnr, TextObjFunctor txt_fnr, MenuRegion& mn_rgn, const int_array& sel_arr)
{
    bool fto_continue = fto_fnr;
    bool txt_continue = txt_fnr;
    Comp::ListObj::ArrType& lst = mn_rgn.List();
    for( int i=0; (fto_continue || txt_continue) && i<(int)sel_arr.size(); ++i )
    {
        Comp::Object* obj = lst[sel_arr[i]];

        if( fto_continue )
            if( FrameThemeObj* fto_obj = dynamic_cast<FrameThemeObj*>(obj) )
            {
                if( !fto_fnr(fto_obj, mn_rgn) )
                    fto_continue = false;
            }

        if( txt_continue )
            if( TextObj* txt_obj = dynamic_cast<TextObj*>(obj) )
            {
                if( !txt_fnr(txt_obj, mn_rgn) )
                    txt_continue = false;
            }
    }
}

void ForAllSelectedFTO(FTOFunctor fnr, MenuRegion& mn_rgn, const int_array& sel_arr)
{
    ForAllSelected(fnr, TextObjFunctor(), mn_rgn, sel_arr);
}


Gtk::Image& GetFactoryGtkImage(const char* name)
{
    RefPtr<Gdk::Pixbuf> img = GetFactoryImage(name);

    ASSERT( img );
    return NewManaged<Gtk::Image>(img);
}

Gtk::Widget& MakeTextToolLabel()
{
//     Gtk::Label& label = NewManaged<Gtk::Label>("<span font_desc=\"FreeSerif 20\">T</span>");
//     label.set_use_markup(true);
//     //label.set_padding(0, 5);
//
//     return label;
    return GetFactoryGtkImage("tool-text.png");
}

Gtk::Widget& MakeSelectionToolImage()
{
    // непонятно, как получить курсор по умолчанию - поэтому переворачиваем
    // похожий на него
//     RefPtr<Gdk::Pixbuf> img = Gdk::Cursor(Gdk::ARROW).get_image();
//     img = img->flip(true);
//     img->save("../ubuntu_pointer.png", "png");
    return GetFactoryGtkImage("tool-pointer.png");
}

Gtk::TreeModelColumn<std::string> FrameTypeColumn;

static RefPtr<Gdk::Pixbuf> RenderToolbarFrame(const std::string& theme_str, uint fill_clr)
{
    const Editor::ThemeData& td   = Editor::GetThumbTheme(theme_str);
    Point thumb_sz(PixbufSize(td.vFrameImg));
    RefPtr<Gdk::Pixbuf> frame_pix = CreatePixbuf(thumb_sz);
    frame_pix->fill(fill_clr);
    frame_pix = Editor::CompositeWithFrame(frame_pix, td);

    Point sz(Round(thumb_sz.x*Editor::TOOL_IMAGE_SIZE/(double)thumb_sz.y), Editor::TOOL_IMAGE_SIZE);
    RefPtr<Gdk::Pixbuf> pix = CreatePixbuf(sz);
    RGBA::Scale(pix, frame_pix);
    return pix;
}

Toolbar::Toolbar(): selTool(MakeSelectionToolImage()), txtTool(MakeTextToolLabel())
    // внучную построим кнопки
    //, bldBtn(Gtk::Stock::BOLD), itaBtn(Gtk::Stock::ITALIC), undBtn(Gtk::Stock::UNDERLINE)
{
    // * инструменты
    Gtk::RadioToolButton::Group group;
    // Selection Tool
    selTool.set_tooltip(TooltipFactory(), boost::format("%1% (S)") % _("Selection Tool") % bf::stop);
    Gtk::set_group(selTool, group);
    // Text Tool
    txtTool.set_tooltip(TooltipFactory(), boost::format("%1% (T)") % _("Text Tool") % bf::stop);
    Gtk::set_group(txtTool, group);

    // * выбор рамки
    {
        // * создаем модель
        Gtk::TreeModelColumnRecord columns;
        Gtk::TreeModelColumn<RefPtr<Gdk::Pixbuf> > pix_cln;
        Gtk::TreeModelColumn<std::string>          str_cln;
        columns.add(pix_cln);
        columns.add(str_cln);
        // добавляем str_cln, чтобы можно было много раз создавать панель инструментов
        Editor::FrameTypeColumn = str_cln;
        RefPtr<Gtk::ListStore> f_store = Gtk::ListStore::create(columns);

        // * нужен цвет фона
        unsigned int fill_clr = 0;
        {
            Gtk::Window tmp_win;
            Gtk::ComboBox tmp_combo;
            tmp_win.add(tmp_combo);
            fill_clr = GetStyleColorBg(Gtk::STATE_NORMAL, tmp_combo).ToUint();
        }
        // * заполняем
        Gtk::ComboBox& combo = frame_combo;
        combo.set_model(f_store);
        // по фокусу определяем источник изменений!
        //combo.set_focus_on_click(false);
        // :TODO: надо поставить подсказку, особенности см. в заданиях про GtkTooltips->GtkToolTip
        //TooltipFactory().set_tip(*combo.get_child(), "Frame Type");
        Str::List lst;
        Project::GetThemeList(lst);
        //fill_clr = GetStyleColorBg(Gtk::STATE_NORMAL, combo).ToUint();
        for( Str::List::iterator itr = lst.begin(), end = lst.end(); itr != end; ++itr )
        {
            Gtk::TreeRow row = *f_store->append();
            const std::string& theme_str = *itr;
            // *
            row[pix_cln] = RenderToolbarFrame(theme_str, fill_clr);
            row[str_cln] = theme_str;
            if( theme_str == "rect" )
                combo.set_active(itr-lst.begin());
        }
        // * внешний вид
        combo.pack_start(pix_cln, false);
        //combo.pack_start(str_cln, true);
        Gtk::CellRendererText& txt_rndr = NewManaged<Gtk::CellRendererText>();
        //txt_rndr.property_size_points() = 12.0;
        txt_rndr.property_xpad() = 5;
        combo.pack_start(txt_rndr, true);
        combo.add_attribute(txt_rndr, "text", str_cln);
    }
}

static double GetFontSize(Toolbar& edt_tbar)
{
    std::string str_sz = edt_tbar.fontSzEnt.get_entry()->get_text();
    double sz;
    if( !Str::GetDouble(sz, str_sz.c_str()) )
        sz = Editor::DEF_FONT_SIZE;
    return sz;
}

TextStyle Toolbar::GetFontDesc()
{
    // шрифт
    Pango::FontDescription fnt_desc;
    fnt_desc.set_family(fontFmlEnt.get_entry()->get_text());
    // размер шрифта
    MEdt::SetFontSize(fnt_desc, GetFontSize(*this));
    // стиль
    fnt_desc.set_weight(bldBtn.get_active() ? Pango::WEIGHT_BOLD : Pango::WEIGHT_NORMAL);
    fnt_desc.set_style(itaBtn.get_active() ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL);

    RGBA::Pixel pxl(clrBtn.get_color());
    pxl.alpha = RGBA::FromGdkComponent(clrBtn.get_alpha());
    return TextStyle(fnt_desc, undBtn.get_active(), pxl);
}

std::string GetActiveTheme(Gtk::ComboBox& combo)
{
    Gtk::TreeIter itr = combo.get_active();
    return itr ? itr->get_value(FrameTypeColumn) : std::string() ;
}

static void AppendTSeparator(Gtk::Toolbar& tbar)
{
    tbar.append(NewManaged<Gtk::SeparatorToolItem>());
}

static void AppendToToolbar(Gtk::Toolbar& tbar, Gtk::Widget& wdg)
{
    Gtk::ToolItem& tool_item = NewManaged<Gtk::ToolItem>();
    tool_item.add(wdg);
    tbar.append(tool_item);
}

static void AppendToToolbarWithAC(Gtk::Toolbar& tbar, Gtk::ComboBoxEntryText& c_ent, 
				  bool is_strict = false)
{
    c_ent.set_focus_on_click(false);
    Gtk::Alignment& alg = NewManaged<Gtk::Alignment>(0.5, 0.5, 0.0, 0.0);
    alg.add(c_ent);
    AppendToToolbar(tbar, alg);
    // автодополнение
    gnc_cbe_require_list_item(c_ent.gobj(), is_strict);
}

Str::PList GetFamiliesList(RefPtr<Pango::Context> context)
{
    Str::PList lst = new Str::List;

    typedef Glib::ArrayHandle<RefPtr<Pango::FontFamily> > FFArray;
    FFArray arr = context->list_families();
    for( FFArray::iterator itr = arr.begin(), end = arr.end(); itr != end; ++itr )
        lst->push_back((*itr)->get_name());

    std::sort(lst->begin(), lst->end());
    return lst;
}

struct RectMatrix
{
    std::vector<Rect> rects;
               Point  dims; //

               RectMatrix(const Point& d): dims(d) {}

         Rect& At(int i, int j) { return rects[i + dims.x * j]; }
};

class AddCheckerVis: public GuiObjVisitor
{
    public:
        RectMatrix& rectMatr;
                       AddCheckerVis(RectMatrix& rm): rectMatr(rm) { }

        virtual  void  Visit(FrameThemeObj& obj) { CheckIntersection(obj.Placement()); }
        virtual  void  Visit(TextObj& obj)       { CheckIntersection(obj.Placement()); }

    protected:
                 void  CheckIntersection(const Rect& rct);
};

void AddCheckerVis::CheckIntersection(const Rect& obj_rct)
{
    for( int i=0; i<rectMatr.dims.x; i++ )
        for( int j=0; j<rectMatr.dims.y; j++ )
        {
            Rect& rct = rectMatr.At(i, j);
            if( obj_rct.Intersects(rct) )
                rct = Rect(); // не подходит
        }
}

const double REL_SAFE_SZ  = 0.1;  // размер границы безопасной области
const double REL_INTER_SZ = 0.01; // расстояние между объектами
const double REL_OBJ_SZ   = 0.18; // размеры добавляемого объекта

static Point CalcFromRelSz(const Point& sz, double rel_sz)
{
    return Point(Round(sz.x*rel_sz), Round(sz.y*rel_sz));
}

static Point CalcStandardObjectSize(const Point& mn_sz)
{
    return CalcFromRelSz(mn_sz, REL_OBJ_SZ);
}

Rect FindNewObjectLocation(RectMatrix& matr, MenuRegion& mr)
{
    AddCheckerVis vis(matr);
    mr.Accept(vis);

    Rect lct;
    for( int i=0; i<matr.dims.x; i++ )
    {
        for( int j=0; j<matr.dims.y; j++ )
        {
            const Rect& rct = matr.At(i, j);
            if( !rct.IsNull() )
            {
                lct = rct;
                break;
            }
        }
        if( !lct.IsNull() )
            break;
    }
    if( lct.IsNull() ) // не нашли свобоодного места
    {
        Point sz(mr.GetParams().Size());
        Point obj_sz = CalcStandardObjectSize(sz);
        Point a      = FindAForCenteredRect(obj_sz, Rect0Sz(sz));

        lct = RectASz(a, obj_sz);
    }

    ASSERT( !lct.IsNull() );
    return lct;
}

// создать матрицу заполнения объектами для кнопки "Добавить объект"
RectMatrix MakeAddObjectMatrix(const MenuParams& mp)
{
    RectMatrix matr(Point(4, 4));   // 4x4 строк и столбцов
    matr.rects.resize(matr.dims.x*matr.dims.y);

    Point sz(mp.Size());
    Point safe_sz(CalcFromRelSz(sz, REL_SAFE_SZ));
    Point inter_sz(CalcFromRelSz(sz, REL_INTER_SZ));
    Point obj_sz(CalcStandardObjectSize(sz));

    Point obj_shift = obj_sz + inter_sz;
    for( int i=0; i<matr.dims.x; i++ )
        for( int j=0; j<matr.dims.y; j++ )
        {
            Point a(safe_sz.x + i*obj_shift.x, safe_sz.y + j*obj_shift.y);
            matr.At(i, j) = RectASz(a, obj_sz);
        }
    return matr;
}

static void AddFTOItem(MEditorArea& editor, const Rect& lct, Project::MediaItem mi)
{
    MenuRegion& rgn = editor.CurMenuRegion();

    Project::AddFTOItem(rgn, Editor::GetActiveTheme(editor.Toolbar().frame_combo), lct, mi);
    RenderForRegion(editor, Planed::AbsToRel(rgn.Transition(), lct));
}

void AddFTOItem(MEditorArea& editor, const Point& center, Project::MediaItem mi)
{
    // :KLUDGE: не самый правильный (и точный) способ сосчитать пропорцию
    Point proportion = editor.Transition().RelToAbs(Project::CalcThumbSize(mi));
    Point sz(editor.CurMenuRegion().GetParams().Size());

    Point it_sz = Project::CalcProportionSize(proportion, sz.x*sz.y*REL_OBJ_SZ*REL_OBJ_SZ);
    Point a(center.x-it_sz.x/2, center.y-it_sz.y/2);
    Rect lct = RectASz(a, it_sz);

    AddFTOItem(editor, lct, mi);
}

static void AddObjectClicked(MEditorArea& editor)
{
    if( Project::Menu mn = editor.CurMenu() )
    {
        // * находим положение
        RectMatrix rm   = MakeAddObjectMatrix(mn->Params());
        MenuRegion& rgn = mn->GetData<Project::MenuPack>().thRgn;
        Rect lct = FindNewObjectLocation(rm, rgn);

        // * создаем
        AddFTOItem(editor, lct, Project::MediaItem());
    }
}

static bool ReframeFTO(RectListRgn& r_lst, FrameThemeObj* obj, 
                       const std::string& theme, MenuRegion& mn_rgn)
{
    if( theme != obj->Theme() )
    {
        obj->GetData<FTOInterPixData>().ClearPix();
        obj->Theme() = theme;
        r_lst.push_back( Planed::AbsToRel(mn_rgn.Transition(), obj->Placement()) );
    }
    return true;
}

static void FrameTypeChanged(MEditorArea& editor)
{
    using namespace boost;
    MenuRegion& mn_rgn = editor.CurMenuRegion();
    RectListRgn r_lst;
    ForAllSelectedFTO( lambda::bind(&ReframeFTO, boost::ref(r_lst), lambda::_1, 
                                    Editor::GetActiveTheme(editor.Toolbar().frame_combo), lambda::_2),
                       mn_rgn, editor.SelArr() );
    RenderForRegion(editor, r_lst);
}

static void EntryChanged(Gtk::Entry& ent, bool is_activate, ActionFunctor fnr)
{
    if( is_activate || !ent.has_focus() )
        fnr();
}

// вызов действия при изменении в поле ввода (улучшенный "activate")
void ConnectOnActivate(Gtk::Entry& ent, ActionFunctor fnr)
{
    ent.signal_activate().connect( boost::lambda::bind(&EntryChanged, boost::ref(ent), true, fnr) );
    ent.signal_changed().connect(  boost::lambda::bind(&EntryChanged, boost::ref(ent), false, fnr) );
}

static void FontNameChanged(MEditorArea& editor, bool only_clr)
{
    SetSelObjectsTStyle(editor, editor.Toolbar().GetFontDesc(), only_clr);
}

static void OnToolbarControlled(MEditorArea& editor, ActionFunctor fnr)
{
//     if( Gtk::Widget* wdg = GetFocusWidget(editor) )
//         PrintWidgetPath(*wdg);
//     else
//         io::cout << "Focus not on the window?" << io::endl;

    if( !editor.has_focus() && editor.CurMenu() )
    {
        fnr();
        // *
        GrabFocus(editor);
    }
}

static void SetupStyleBtn(Gtk::ToggleButton& btn, Gtk::Toolbar& tbar,
                          const char* tooltip, const ActionFunctor& fnr,
                          Gtk::BuiltinStockID stock_id)
{
    // вручную строим кнопки а-ля Gtk::ToggleToolButton, потому что
    // нам надо установить у них свойство focus-on-click для последующего
    // определения, откуда пошло действие (с кнопки или с редактора)

    //tbar.append(btn);
    //btn.set_tooltip(TooltipFactory(), tooltip);
    ASSERT( btn.get_focus_on_click() ); // должен быть по умолчанию
    AppendToToolbar(tbar, btn);
    btn.set_relief(Gtk::RELIEF_NONE);
    btn.add(NewManaged<Gtk::Image>(stock_id, tbar.get_icon_size()));
    SetTip(btn, tooltip);

    btn.signal_toggled().connect(fnr);
}

Gtk::Toolbar& PackToolbar(MEditorArea& editor, Gtk::VBox& lct_box)
{
    Editor::Toolbar& edt_tbar = editor.Toolbar();
    Gtk::Toolbar& tbar = NewManaged<Gtk::Toolbar>();
    tbar.set_toolbar_style(Gtk::TOOLBAR_ICONS);
    tbar.set_tooltips(true);
    lct_box.pack_start(tbar, Gtk::PACK_SHRINK);

    // * 2 инструмента
    Gtk::RadioToolButton& sel_btn = edt_tbar.selTool;
    tbar.append(sel_btn);
    tbar.append(edt_tbar.txtTool);
    AppendTSeparator(tbar);

    // хочется отрабатывать изменения в управляющих элементах только когда
    // фокус не на редакторе (т.е. когда не сам редактор изменился)
    typedef boost::function<void(ActionFunctor)> ActionActionFunctor;
    ActionActionFunctor otc = bl::bind(&OnToolbarControlled, boost::ref(editor), bl::_1);

    // * выбор темы объекта
    {
        Gtk::ComboBox& combo = edt_tbar.frame_combo;
        AppendToToolbar(tbar, combo);
        combo.signal_changed().connect(
            bl::bind(otc, ActionFunctor(bl::bind(&FrameTypeChanged, boost::ref(editor)))));

        // * кнопка
        Gtk::ToolButton& add_btn = NewManaged<Gtk::ToolButton>(Gtk::StockID(Gtk::Stock::ADD));
        add_btn.set_tooltip(TooltipFactory(), _("Add Item"));
        tbar.append(add_btn);
        add_btn.signal_clicked().connect( bl::bind(&AddObjectClicked, boost::ref(editor)) );
    }
    AppendTSeparator(tbar);

    // * управление шрифтами
    ActionFunctor on_font_change = bl::bind(otc, ActionFunctor(bl::bind(&FontNameChanged, boost::ref(editor), false)));
    {
        std::string def_font = tbar.get_style()->get_font().get_family().raw();
        // * семейства
        Gtk::ComboBoxEntryText& fonts_ent = edt_tbar.fontFmlEnt;
        Str::PList lst = GetFamiliesList(tbar.get_toplevel()->get_pango_context());
        for( Str::List::iterator itr = lst->begin(), end = lst->end(); itr != end; ++itr )
        {
            fonts_ent.append_text(*itr);
            if( *itr == def_font )
                fonts_ent.set_active(itr-lst->begin());
        }
        AppendToToolbarWithAC(tbar, fonts_ent, true);
        {
            Gtk::Entry& ent = *fonts_ent.get_entry();
            SetTip(ent, _("Font Name"));
            ConnectOnActivate(ent, on_font_change);
        }

        // * размер
        Gtk::ComboBoxEntryText& size_ent = edt_tbar.fontSzEnt;
        const char* size_list[] = { "4", "8", "9", "10", "11", "12", "14", "16", "18", "20",
            "22", "24", "26", "28", "36", "48", "72", 0 };
        for( const char** p_size = size_list; *p_size; ++p_size )
            size_ent.append_text(*p_size);
        size_ent.set_active(13); // = "28"
        AppendToToolbarWithAC(tbar, size_ent);
        {
            Gtk::Entry& ent = *size_ent.get_entry();
            LimitTextInput(ent, "0123456789.,");
            ent.set_width_chars(5);
            SetTip(ent, _("Font Size"));
            ConnectOnActivate(ent, on_font_change);
        }

        // * кнопки стилей шрифта
        SetupStyleBtn(edt_tbar.bldBtn, tbar, _("Bold"), on_font_change, Gtk::Stock::BOLD);
        SetupStyleBtn(edt_tbar.itaBtn, tbar, _("Italic"), on_font_change, Gtk::Stock::ITALIC);
        SetupStyleBtn(edt_tbar.undBtn, tbar, _("Underline"), on_font_change, Gtk::Stock::UNDERLINE);

        // * кнопка цвета текста
        Gtk::ColorButton& clr_btn = edt_tbar.clrBtn;
        clr_btn.set_use_alpha();
        clr_btn.set_relief(Gtk::RELIEF_NONE);
        clr_btn.set_color(RGBA::PixelToColor(GOLD_CLR));
        clr_btn.set_alpha(0xffff);

        clr_btn.set_title(_("Pick a Color for Text"));
        clr_btn.set_focus_on_click(false);
        SetTip(clr_btn, _("Text Color"));

        clr_btn.signal_color_set().connect(bl::bind(&FontNameChanged, boost::ref(editor), true));
        AppendToToolbar(tbar, clr_btn);

    }

    // * кнопка рамки
    Gtk::ToggleButton& frm_btn = edt_tbar.frmBtn;
    AppendToToolbar(tbar, frm_btn);
    frm_btn.set_relief(Gtk::RELIEF_NONE);
    frm_btn.add(GetFactoryGtkImage("copy-n-paste/lpetool_show_bbox.png"));
    SetTip(frm_btn, _("Show Safe Area"));
    // обвязка otc здесь используется для проверки, что в редакторе есть меню (и не валится при его отстутствии)
    frm_btn.signal_toggled().connect(bl::bind(otc, ActionFunctor(bl::bind(&ToggleSafeArea, boost::ref(editor)))));

    return tbar;
}

} // namespace Editor
