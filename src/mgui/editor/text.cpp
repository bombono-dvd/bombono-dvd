//
// mgui/editor/text.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#include "text.h"
#include "kit.h"
#include "actions.h"

#include <mgui/win_utils.h>
#include <mgui/key.h>

#include <mlib/sdk/logger.h>

namespace MEdt
{

double FontSize(const Pango::FontDescription& dsc)
{
    int sz = dsc.get_size();

    if( dsc.get_size_is_absolute() )
        return sz * (72./GetDpi());
    else
        return pango_units_to_double(sz);
}

void SetFontSize(Pango::FontDescription& dsc, double sz)
{
    dsc.set_size(pango_units_from_double(sz));
}

void CheckDescNonNull(Editor::TextStyle& ts)
{
    Pango::FontDescription& dsc = ts.fntDsc;
    if( dsc.get_family().empty() )
    {
        Pango::FontDescription def_dsc("Sans Bold 27");
        dsc.swap(def_dsc);
    }
}

} // namespace MEdt

TextObj::TextObj(): hgtMult(1.)
{
    MEdt::CheckDescNonNull(tStyle);
}

void TextObj::SetFontDesc(const std::string& fnt)
{ 
    Pango::FontDescription dsc(fnt.c_str());
    SetFontDesc(dsc);
}

double TextObj::FontSize()
{
    return MEdt::FontSize(FontDesc());
}

void TextObj::SetFontSize(double sz)
{
    MEdt::SetFontSize(tStyle.fntDsc, sz);
}

static void _CalcAbsSizes(RefPtr<Pango::Layout> lay, double& wdh, double& hgt)
{
    int t_wdh, t_hgt;
    lay->get_size(t_wdh, t_hgt);

    wdh = pango_units_to_double(t_wdh);
    hgt = pango_units_to_double(t_hgt);
}

static void SetDPI(RefPtr<Pango::Layout> lay, double dpi)
{
    if( dpi == 0. )
        dpi = MEdt::GetDpi();
    lay->get_context()->set_resolution(dpi);
}

void CalcAbsSizes(const std::string& txt, const Pango::FontDescription& dsc, 
                      double& wdh, double& hgt, double dpi)
{
    RefPtr<Gdk::Pixbuf> aux_pix = CreatePixbuf(1, 1, true);
    TextContext t_cnvs;
    t_cnvs.Init(aux_pix, dpi);

    RefPtr<Pango::Layout> lay = t_cnvs.PanLay();
    lay->set_font_description(dsc);
    lay->set_text(txt);
    _CalcAbsSizes(lay, wdh, hgt);
}

DPoint CalcAbsSizes(RefPtr<Pango::Layout> lay, double dpi)
{
    SetDPI(lay, dpi);

    DPoint res;
    _CalcAbsSizes(lay, res.x, res.y);
    return res;
}

static Point CalcSizesByText(const std::string& txt, const Pango::FontDescription& dsc,
                             double hgt_scale)
{
    double wdh, hgt;
    CalcAbsSizes(txt, dsc, wdh, hgt);

    return Point( Round(wdh), Round(hgt*hgt_scale) );
}

static double CalcFontSizeFromWidth(const std::string& txt, double wdh, double& t_hgt,
                              const Pango::FontDescription& dsc)
{
    double t_wdh;
    CalcAbsSizes(txt, dsc, t_wdh, t_hgt);

    double sz = MEdt::FontSize(dsc);
    if( t_wdh != 0 ) 
        sz *= wdh/t_wdh;

    return sz;
}

void TextObj::CalcDimensions(bool by_text)
{
    if( by_text )
    {
        Point new_sz = CalcSizesByText(text, FontDesc(), hgtMult);
        mdPlc.SetWidth(new_sz.x);
        mdPlc.SetHeight(new_sz.y);
    }
    else
        CalcBySize(mdPlc.Size());
}

void TextObj::CalcBySize(const Point& sz)
{
    double t_hgt;
    double fnt_sz = CalcFontSizeFromWidth(text, sz.x, t_hgt, FontDesc());

    hgtMult = sz.y/t_hgt;
    // :TODO: при сохранении (на диск) надо использовать абсолютные размеры шрифта
    LOG_INF  << "TextObj::CalcBySize: sz " << sz << "; hgtMult " << hgtMult 
             << "; text " << text << "; fnt_sz " << fnt_sz 
             << "; t_hgt " << t_hgt
             << "; font " << FontDesc().to_string() << io::endl;

    SetFontSize(fnt_sz);
}

void TextObj::Accept(Comp::ObjVisitor& vis)
{
    if( GuiObjVisitor* gvis = dynamic_cast<GuiObjVisitor*>(&vis) )
        gvis->Visit(*this);
}

/////////////////////////////////////////////////////////////

void TextContext::Clear()
{
    ClearRefPtr(panLay);
    ClearRefPtr(caiCont);
    ClearRefPtr(caiSur);
}

void TextContext::Init(RefPtr<Gdk::Pixbuf> canv_pix, double dpi)
{
    ASSERT( canv_pix );

    caiSur  = GetAsImageSurface(canv_pix);
    caiCont = Cairo::Context::create(caiSur);
    panLay  = Pango::Layout::create(caiCont);

    SetDPI(panLay, dpi);
}

TextRenderer::~TextRenderer()
{
    //Clear();
    if( curStt )
        curStt->OnChange(*this, false);
}

void TextRenderer::ChangeState(CursorState* new_stt)
{
    if( curStt )
        curStt->OnChange(*this, false);

    curStt = new_stt;

    if( curStt )
        curStt->OnChange(*this, true);
}

bool TextRenderer::IsShowCursor()
{
    return curPos>=0 && curShow;
}

void TextRenderer::ShowCursor(bool is_show, bool is_init)
{ 
    curShow = is_show;
    if( is_init )
        curPos = 0;
    curStt->OnShow(*this);
}

void TextRenderer::RenderCursor()
{
    if( IsShowCursor() && curStt->IsToShow(*this) )
    {
        ApplyTextTrans();

        Rect plc = RawCursorPos();
        caiCont->rel_move_to(plc.lft, plc.top);
        double cur_x, cur_y;
        caiCont->get_current_point(cur_x, cur_y);

        caiCont->rectangle(cur_x, cur_y, plc.Width(), plc.Height());
        // см. "cairo_fill_preserve bug" - использование
        // cairo_fill_preserve() приводит к рисованию там, где не надо;
        // поскольку cairo_fill_preserve не обращает внимания на ограничения,
        // сделанные нами выше, в RenderBegin()
        //caiCont->fill_preserve();
        caiCont->fill();
    }
}

int TextRenderer::ByteCursorPos()
{
    if( curPos<0 )
        return curPos;

    return utf8::trans(GetText()).from_offset(curPos);
}

TextRenderer::TextRenderer(): curPos(-1), curStt(0), curShow(false)
{
    ChangeState(&HideCursor::Instance());
}

void TextRenderer::CalcTextSize(Pango::FontDescription& dsc, double font_mult,
                                double& wdh, double& hgt)
{
    double new_sz = font_mult * MEdt::FontSize(dsc);

    MEdt::SetFontSize(dsc, new_sz);
    panLay->set_font_description(dsc);

    _CalcAbsSizes(panLay, wdh, hgt);
}

void TextRenderer::DoLayout()
{
    const Point new_sz = CalcTextPlc().Size();

    TextObj& t_obj = GetTextObj();
    panLay->update_from_cairo_context(caiCont);
    panLay->set_text(t_obj.Text().c_str());

    const Editor::TextStyle& ts = t_obj.Style();
    Pango::AttrList attr_list;
    if( ts.isUnderlined )
    {
        Pango::AttrInt attr = Pango::Attribute::create_attr_underline(Pango::UNDERLINE_SINGLE);
        attr.set_start_index(0);
        attr.set_end_index(G_MAXUINT);

        attr_list.insert(attr);
    }
    panLay->set_attributes(attr_list);
    Pango::FontDescription dsc(ts.fntDsc);

    Point abs_sz = t_obj.Placement().Size();
    double t_wdh, t_hgt;
    if( abs_sz.x != 0 )
    {
        // размер рассчитываем по ширине, так как она уязвимей к скалированию
        CalcTextSize(dsc, (double)new_sz.x/abs_sz.x, t_wdh, t_hgt);

        //CalcTextSize(dsc, (double)new_sz.x/t_wdh, t_wdh, t_hgt);

        // погрешность - на практике оказалось, что лучше вообще не использовать
        // скалирование по горизонтали перед отображением, потому что отрисовка
        // текста реально может отличаться (по размеру) от того, что говорит
        // Pango::Layout::get_size() (!)
        txtRat.first  = (double)new_sz.x/t_wdh;
    }
    else
    {
        // пусто
        ASSERT( abs_sz.y != 0 ); // размер шрифта ненулевой

        CalcTextSize(dsc, (double)new_sz.y/abs_sz.y, t_wdh, t_hgt);

        txtRat.first  = 1.0;
        //txtSht = 0;
    }
    txtRat.second = (double)new_sz.y/t_hgt;
    txtSht = int( (new_sz.x - t_wdh)/2 );
}

void TextRenderer::ApplyTextTrans()
{
    Point lct = CalcTextPlc().A();
    // передвигаем координаты на смещение и сами перемещаемся
//     caiCont->move_to(lct.x, lct.y);
//     caiCont->rel_move_to(txtSht, 0);
//     caiCont->translate(txtSht, 0);
    lct.x += txtSht;
    caiCont->move_to(lct.x, lct.y);
    caiCont->translate(lct.x, lct.y);

    // см. TextRenderer::DoLayout()
    caiCont->scale(1.0, txtRat.second);

    RGBA::Pixel clr = GetTextObj().Color();
    CR::SetColor(caiCont, clr);
}

Rect TextRenderer::RawCursorPos()
{
    Pango::Rectangle w_rct, s_rct;
    panLay->get_cursor_pos(ByteCursorPos(), w_rct, s_rct);
    pango_extents_to_pixels(0, w_rct.gobj());

    double curs_rat = CursAspectRatio();
    int curs_wdh = int(w_rct.get_height()*curs_rat + 1);

    int curs_lft = w_rct.get_x() - curs_wdh/2;
    return Rect(curs_lft,            w_rct.get_y(),
                curs_lft + curs_wdh, w_rct.get_y() + w_rct.get_height());
}

static void UserToDevice(Cairo::RefPtr<Cairo::Context> cr, int& x, int& y)
{
    double d_x = x, d_y = y;
    cr->user_to_device(d_x, d_y);

    x = Round(d_x), y = Round(d_y);
}

Rect TextRenderer::CalcCursorPlc()
{
    if( curPos<0 ) // пусто
        return Rect();

    CairoStateSave save(caiCont);
    ApplyTextTrans();

    Rect plc = RawCursorPos();
    UserToDevice(caiCont, plc.lft, plc.top);
    UserToDevice(caiCont, plc.rgt, plc.btm);
    // из-за округления увеличиваем
    return EnlargeRect(plc, Point(1, 1));
}

void TextRenderer::RenderText()
{
//     // рамка для наглядности (отладки)
//     Rect txt_rct = CalcTextPlc();
//     caiCont->set_line_width(1.0);
//     caiCont->rectangle(0, 0, txt_rct.Width(), txt_rct.Height());
//     caiCont->stroke();
    //caiCont->set_source_rgb(1., 0, 0);

    ApplyTextTrans();
    // собственно отрисовка
    panLay->update_from_cairo_context(caiCont);
    show_in_cairo_context(panLay, caiCont);
}

void TextRenderer::ClearDelayedDrawing()
{
    delayedRgn.clear();
}

void TextRenderer::CommitDelayedDrawing()
{
    DrawForRegion(delayedRgn);
    ClearDelayedDrawing();
}

void TextRenderer::DrawForRegionDelayed(RectListRgn& r_lst)
{
    delayedRgn.insert(delayedRgn.end(), r_lst.begin(), r_lst.end());
}

static void RedrawTextRenderer(TextRenderer& txt_rndr, RectListRgn& r_lst, bool delayed)
{
    delayed ? 
        txt_rndr.DrawForRegionDelayed(r_lst) 
    :
        txt_rndr.DrawForRegion(r_lst);
}

void RedrawCursor(TextRenderer& txt_rndr, bool delayed)
{
    Rect rct = txt_rndr.CalcCursorPlc();

    RectListRgn r_lst;
    r_lst.push_back(rct);
    //txt_rndr.DrawForRegion(r_lst);
    RedrawTextRenderer(txt_rndr, r_lst, delayed);
}

void RedrawText(TextRenderer& txt_rndr, bool delayed)
{
    RectListRgn r_lst;

    r_lst.push_back(txt_rndr.CalcCursorPlc());
    r_lst.push_back(txt_rndr.CalcTextPlc());

    //txt_rndr.DrawForRegion(r_lst);
    RedrawTextRenderer(txt_rndr, r_lst, delayed);
}

class TxtDrawTrans
{
    public:
            TxtDrawTrans(TextRenderer& rndr) : txtRndr(rndr)
            { }
           ~TxtDrawTrans()
            { txtRndr.CommitDelayedDrawing(); }

    protected:
            TextRenderer& txtRndr;
};

void TextRenderer::MoveCursor(int new_pos)
{
    TxtDrawTrans tr(*this);    

    RedrawCursor(*this, true);
    curPos = new_pos;
    RedrawCursor(*this, true);
}

void TextRenderer::MoveCursorToLine(bool is_up)
{
    int lin_pos, x_pos;
    panLay->index_to_line_x(ByteCursorPos(), false, lin_pos, x_pos);
    lin_pos--;

    lin_pos += is_up ? -1 : 1 ;
    if( (lin_pos >= 0) && lin_pos < panLay->get_line_count() )
    {
        // найдем на новой строке наиболее близкий текущему символ
        // для этого используем функцию pango_layout_line_x_to_index с последующим
        // подбором - левее или правее символа перейдем
        // Замечание: пытался воспользоваться вроде сходной функцией
        // pango_layout_line_get_x_ranges(), но та почему всегда выдает диапазон размером
        // 1, т.е. все положение строки, а не ее символов (ерунда!)
        RefPtr<const Pango::LayoutLine> line = panLay->get_line(lin_pos);

        int new_pos;
        utf8::trans tr(GetText());

        int idx, trail;
        if( line->x_to_index(x_pos, idx, trail) )
        {
            new_pos = tr.to_offset(idx);

            int l_x_pos = line->index_to_x(idx, false);
            int r_x_pos = line->index_to_x(idx, true);
            ASSERT( l_x_pos<=x_pos && x_pos<=r_x_pos );
            if( x_pos - l_x_pos > r_x_pos - x_pos )
                new_pos++;

            MoveCursor( new_pos );
        }
        else
            MoveCursor( tr.to_offset(line->get_start_index() + line->get_length()) );
    }
}

const char* TextRenderer::GetText()
{
    return pango_layout_get_text(panLay->gobj());
}

void TextRenderer::ChangeText(const std::string& new_txt, int new_cur_pos)
{
    TxtDrawTrans tr(*this);

    RedrawText(*this, true);

    GetTextObj().SetText(new_txt);
    curPos = new_cur_pos;

    DoLayout();

    RedrawText(*this, true);
}

Rect TextRenderer::CalcTextPlc()
{
    Planed::Transition tr = GetTransition();
    return Planed::AbsToRel(tr, GetTextObj().Placement());
}

void TextRenderer::RenderWithFunctor(DrwFunctor drw_fnr, const Rect& drw_rct, 
                                   const Rect& obj_rct)
{
    Rect rct = Intersection(drw_rct, obj_rct);
    if( !rct.IsNull() )
    {
        CairoStateSave save(caiCont);
        RenderBegin(rct);

        drw_fnr();

        RenderEnd(rct);
    }
}

void TextRenderer::RenderByRegion(const Rect& drw_rct)
{
    // 1 отрисовка текста
    RenderWithFunctor( bb::bind(&TextRenderer::RenderText, this), drw_rct,
                       CalcTextPlc() );

    // 2 отрисовка курсора
    RenderWithFunctor( bb::bind(&TextRenderer::RenderCursor, this), drw_rct,
                       CalcCursorPlc() );
}

void TextRenderer::OnFocusInEvent()
{
    curStt->OnGetFocus(*this);
}

void TextRenderer::OnFocusOutEvent()
{
    curStt->OnLeaveFocus(*this);
}

void TextRenderer::InsertText(const std::string& insert_str)
{
    if( !insert_str.empty() )
    {
        std::string old_str = GetText();
        int idx = utf8::trans(old_str.c_str()).from_offset(curPos);
    
        std::string new_str(old_str, 0, idx);
        new_str.append(insert_str);
        new_str.append(old_str, idx, old_str.length() - idx);
    
        ChangeText(new_str, curPos+utf8::trans(insert_str.c_str()).length());
    }
}

static void InsertUText(TextRenderer* rndr, const Glib::ustring& insert_str)
{
    rndr->InsertText(insert_str.raw());
}

RefPtr<Gtk::Clipboard> GetCb(TextRenderer& rndr)
{
    return rndr.GetOwnerWidget()->get_clipboard("CLIPBOARD");
}

static void CopyClipboard(TextRenderer* rndr, bool with_cut)
{
    GetCb(*rndr)->set_text(rndr->GetText());
    if( with_cut )
        rndr->ChangeText("", 0);
}

static void PasteClipboard(TextRenderer* rndr)
{
    GetCb(*rndr)->request_text(bb::bind(InsertUText, rndr, _1));
}

void FillHotKeyMap(HK<TextRenderer>::Map& map, GdkKeymap* keymap)
{
    typedef HK<TextRenderer>::Functor Functor;

    // Copy Clipboard
    Functor copy_cb_fnr = bb::bind(&CopyClipboard, _1, false);
    AppendHK(map, GDK_c,      GDK_CONTROL_MASK, copy_cb_fnr, keymap);
    AppendHK(map, GDK_Insert, GDK_CONTROL_MASK, copy_cb_fnr, keymap);
    // Cut Clipboard
    Functor cut_cb_fnr = bb::bind(&CopyClipboard, _1, true);
    AppendHK(map, GDK_x,      GDK_CONTROL_MASK, cut_cb_fnr, keymap);
    AppendHK(map, GDK_Delete, GDK_SHIFT_MASK,   cut_cb_fnr, keymap);
    // Paste Clipboard
    Functor paste_cb_fnr = bb::bind(&PasteClipboard, _1);
    AppendHK(map, GDK_v,      GDK_CONTROL_MASK, paste_cb_fnr, keymap);
    AppendHK(map, GDK_Insert, GDK_SHIFT_MASK,   paste_cb_fnr, keymap);
}

void TextRenderer::OnKeyPressEvent(GdkEventKey* event)
{
    if( curPos<0 )
        return;

    // :TODO: "key_binding" - осталось полностью перевести на CallHotKey()
    if( CanShiftOnly(event->state) )
    {
        switch( event->keyval )
        {
        case GDK_Left:  case GDK_KP_Left:
            if( curPos>0 )
                MoveCursor(curPos - 1);
            break;
        case GDK_Right: case GDK_KP_Right:
            {
                int len = utf8::trans(GetText()).length();
                if( curPos<len )
                    MoveCursor(curPos + 1);
            }
            break;
        case GDK_Up:    case GDK_KP_Up:
            MoveCursorToLine(true);
            break;
        case GDK_Down:  case GDK_KP_Down:
            MoveCursorToLine(false);
            break;
        case GDK_Home:  case GDK_KP_Home:
            MoveCursor(0);
            break;
        case GDK_End:  case GDK_KP_End:
            MoveCursor( utf8::trans(GetText()).length() );
            break;
        case GDK_Delete:  case GDK_KP_Delete:
            {
                std::string old_str = GetText();
                utf8::trans tr(old_str.c_str());
                if( curPos<tr.length() )
                {
                    std::string new_str(old_str, 0, tr.from_offset(curPos));

                    int after_idx = tr.from_offset(curPos+1);
                    new_str.append(old_str, after_idx, old_str.length() - after_idx);

                    ChangeText(new_str, curPos);
                }
            }
            break;
        case GDK_BackSpace:
            {
                std::string old_str = GetText();
                if( curPos > 0 )
                {
                    utf8::trans tr(old_str.c_str());
                    std::string new_str(old_str, 0, tr.from_offset(curPos-1));

                    int idx = tr.from_offset(curPos);
                    new_str.append(old_str, idx, old_str.length() - idx);

                    ChangeText(new_str, curPos-1);
                }
            }
            break;
        default:
            {
                int u_c = gdk_keyval_to_unicode(event->keyval);
                // вставка символов
                if( u_c )
                {
                    if( !g_unichar_isprint(u_c) )
                        u_c = 0;
                }
                else
                {
                    if( event->keyval == GDK_Return )
                        u_c = '\n';
                    else if( event->keyval == GDK_Tab )
                        u_c = '\t';
                }

                if( u_c )
                {
                    char utf8_c[7];
                    utf8_c[ g_unichar_to_utf8(u_c, utf8_c) ] = 0;

                    //std::string old_str = GetText();
                    //int idx = utf8::trans(old_str.c_str()).from_offset(curPos);
                    //
                    //std::string new_str(old_str, 0, idx);
                    //new_str.append(utf8_c);
                    //new_str.append(old_str, idx, old_str.length() - idx);
                    //
                    //ChangeText(new_str, curPos+1);
                    InsertText(utf8_c);
                }
                break;
            }
        }
    }
    // классические горячие клавишы
    CallHotKey(*this, event);

    curStt->OnInput(*this);
}

void TextRenderer::OnButtonPressEvent(GdkEventButton* event)
{
    if( event->type != GDK_BUTTON_PRESS || !IsLeftButton(event) )
        return;

    Point lct((int)event->x, (int)event->y);
    lct = GetTransition().DevToRel(lct);
    if( CalcTextPlc().Contains(lct) )
    {
        double x = lct.x, y = lct.y;
        {
            CairoStateSave save(caiCont);
            ApplyTextTrans();

            caiCont->device_to_user(x, y);
        }

        int idx, trail;
        utf8::trans tr(GetText());
        if( panLay->xy_to_index(pango_units_from_double(x), pango_units_from_double(y), idx, trail) )
        {
            int new_pos = tr.to_offset(idx);
            // видимо в случае "сложных" букв trail может принимать и отличные от
            // {0, 1} значения (восточные иероглифы?), но нам требуется точность до целого символа
            if( trail )
                new_pos++;

            MoveCursor( new_pos );
        }
        else
            MoveCursor( tr.length() );
    }
}

bool TextRenderer::HasFocus()
{
    return GetOwnerWidget() && GetOwnerWidget()->has_focus();
}

double TextRenderer::CursAspectRatio()
{
    gfloat res;
    GetOwnerWidget()->get_style_property("cursor-aspect-ratio", res);
    return res;
}

int TextRenderer::CursBlinkTime()
{
    // "gtk-cursor-blink-time"
    return GetOwnerWidget()->get_settings()->property_gtk_cursor_blink_time();
}

/////////////////////////////////////////////////////////////////////////
// CursorState

void CursorState::ChangeState(TextRenderer& txt_rndr, CursorState& stt)
{
    txt_rndr.ChangeState(&stt);
}

static bool CanShowCursor(TextRenderer& txt_rndr)
{
    return txt_rndr.IsShowCursor() && txt_rndr.HasFocus();
}

void HideCursor::OnGetFocus(TextRenderer& txt_rndr)
{
    if( CanShowCursor(txt_rndr) )
    {
        ChangeState(txt_rndr, PendingCursor::Instance());
        RedrawCursor(txt_rndr);
    }
}

void HideCursor::OnShow(TextRenderer& txt_rndr)
{
    OnGetFocus(txt_rndr);
}

void ShowCursor::OnShow(TextRenderer& txt_rndr)
{
    OnLeaveFocus(txt_rndr);
}

void ShowCursor::OnLeaveFocus(TextRenderer& txt_rndr)
{
    if( !CanShowCursor(txt_rndr) )
    {
        ChangeState(txt_rndr, HideCursor::Instance());
        RedrawCursor(txt_rndr);
    }
}

bool BlinkCursor::IsToShow(TextRenderer& txt_rndr)
{ 
    return ((Data&)txt_rndr).toShow; 
}

const double CURSOR_ON_PART  = 0.66;  // соотношение видимости-невидимости курсора
const double CURSOR_OFF_PART = 0.34;
const double CURSOR_PEND_PART = 0.1;//1.0;  // на время печатания курсор должен перестать мигать

static int GetBlinkTime(bool to_show, int time_len)
{
    return int( time_len * ( to_show ? CURSOR_ON_PART : CURSOR_OFF_PART ) );
}

void BlinkCursor::SetTimer(TextRenderer& txt_rndr)
{
    Data& dat = (Data&)txt_rndr;
//     sigc::slot<bool> to_slot = sigc::bind(sigc::mem_fun(*this, &BlinkCursor::OnTimeout), &txt_rndr);
//     dat.timer =
//         Glib::signal_timeout().connect(to_slot, GetBlinkTime(dat.toShow, txt_rndr.CursBlinkTime()) );

    BoolFnr fnr = bb::bind(&BlinkCursor::OnTimeout, this, &txt_rndr);
    dat.timer.Connect( fnr, GetBlinkTime(dat.toShow, txt_rndr.CursBlinkTime()) );
}

bool BlinkCursor::OnTimeout(TextRenderer* txt_rndr)
{
    Data& dat = *(Data*)txt_rndr;
    dat.toShow = !dat.toShow;
    RedrawCursor(*txt_rndr);

    SetTimer(*txt_rndr);
    return false;
}

void BlinkCursor::OnChange(TextRenderer& txt_rndr, bool is_in)
{
    Data& dat = (Data&)txt_rndr;
    if( is_in )
    {
        dat.toShow = true;

        SetTimer(txt_rndr);
    }
    else
        dat.timer.Disconnect();
}

void BlinkCursor::OnInput(TextRenderer& txt_rndr)
{
    ChangeState(txt_rndr, PendingCursor::Instance());
    RedrawCursor(txt_rndr);
}

void PendingCursor::SetTimer(TextRenderer& txt_rndr)
{
    Data& dat = (Data&)txt_rndr;

    BoolFnr fnr = bb::bind(&PendingCursor::OnTimeout, this, &txt_rndr);
    dat.timer.Connect( fnr, int(CURSOR_PEND_PART*txt_rndr.CursBlinkTime()) );
}

bool PendingCursor::OnTimeout(TextRenderer* txt_rndr)
{
    ChangeState(*txt_rndr, BlinkCursor::Instance());
    return false;
}

void PendingCursor::OnChange(TextRenderer& txt_rndr, bool is_in)
{
    Data& dat = (Data&)txt_rndr;
    if( is_in )
        SetTimer(txt_rndr);
    else
        dat.timer.Disconnect();
}

void PendingCursor::OnInput(TextRenderer& txt_rndr)
{
    Data& dat = (Data&)txt_rndr;
    dat.timer.Disconnect();

    SetTimer(txt_rndr);
}

//////////////////////////////////////////////////////////////////////

EdtTextRenderer::EdtTextRenderer(DataWare& dw): edtOwner(0), rLst(0), cnvPrecisely(false)
{
    owner = dynamic_cast<TextObj*>(&dw);
    ASSERT( owner );
}

void EdtTextRenderer::DrawForRegion(RectListRgn& r_lst)
{
    if( rLst )
        rLst->insert(rLst->end(), r_lst.begin(), r_lst.end());
    else
        ::RenderForRegion(*edtOwner, r_lst);
}

static void ConvertCP(RefPtr<Gdk::Pixbuf> pix, const Rect& rct, bool precisely, bool from_cairo)
{
    precisely ? ConvertCairoVsPixbuf(pix, rct, from_cairo) : AlignCairoVsPixbuf(pix, rct) ;
}

void EdtTextRenderer::RenderBegin(const Rect& rct)
{
    //AlignCairoVsPixbuf(cnvBuf->Canvas(), rct);
    ConvertCP(cnvBuf->Canvas(), rct, cnvPrecisely, false);
    CR::RectClip(caiCont, rct);
}

void EdtTextRenderer::RenderEnd(const Rect& rct)
{
    //AlignCairoVsPixbuf(cnvBuf->Canvas(), rct);
    ConvertCP(cnvBuf->Canvas(), rct, cnvPrecisely, true);
}

const Planed::Transition& EdtTextRenderer::GetTransition()
{
    return cnvBuf->Transition();
}

TextRenderer::OwnerWidget* EdtTextRenderer::GetOwnerWidget()
{ 
    return edtOwner; 
}

// когда в редакторе => можем редактировать
void EdtTextRenderer::SetEditor(EditorType* edt) 
{ 
    edtOwner = edt;
    SetCanvasBuf(edtOwner);
}

// нужна только отрисовка
void EdtTextRenderer::SetCanvasBuf(CanvasBuf* cnv_buf)
{ 
    cnvBuf = cnv_buf; 
}

