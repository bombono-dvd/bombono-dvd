//
// mgui/sdk/textview.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#include "textview.h"
#include "widget.h"

static void InitStderrTag(RefPtr<Gtk::TextTag> tag)
{
    Glib::PropertyProxy<Pango::FontDescription> prop = tag->property_font_desc();

    Pango::FontDescription dsc = prop.get_value();
    dsc.set_style(Pango::STYLE_ITALIC);
    prop.set_value(dsc);
}

TextIterRange AppendNewText(Gtk::TextView& txt_view, const std::string& line, bool is_out)
{
    TextIterRange tir = AppendText(txt_view, line);
    //ApplyStripyOutput(txt_view, tir);

    if( !is_out )
        ApplyTag(txt_view, tir, "stderr", InitStderrTag);
    return tir;
}

static void InitCommandTag(RefPtr<Gtk::TextTag> tag)
{
    tag->property_foreground() = "darkred";
}

void AppendCommandText(Gtk::TextView& txt_view, const std::string& title)
{
    ApplyTag(txt_view, AppendText(txt_view, title + "\n"), 
             "Command", InitCommandTag);
}

//static void InitRedTag(RefPtr<Gtk::TextTag> tag, bool is_red)
//{
//    tag->property_foreground() = is_red ? "darkred" : "darkblue" ;
//}
//
//static bool RedTag = true;
//InitTagFunctor RedFnr  = boost::lambda::bind(&InitRedTag, boost::lambda::_1, true);
//InitTagFunctor BlueFnr = boost::lambda::bind(&InitRedTag, boost::lambda::_1, false);
//
//static void ApplyStripyOutput(Gtk::TextView& txt_view, const TextIterRange& tir)
//{
//    RedTag ? ApplyTag(txt_view, tir, "RedTag", RedFnr) : ApplyTag(txt_view, tir, "BlueTag", BlueFnr) ;
//    RedTag = !RedTag;
//}

TextIterRange AppendText(Gtk::TextView& txt_view, const std::string& text)
{
    RefPtr<Gtk::TextBuffer> buf = txt_view.get_buffer();
    Gtk::TextIter itr = buf->get_iter_at_offset(-1);
    
    // внизу ли находимся
    Gdk::Rectangle disp_rct;
    txt_view.get_visible_rect(disp_rct);
    Gdk::Rectangle itr_rct;
    txt_view.get_iter_location(itr, itr_rct);
    // нас интересует пересечение только по высоте, тем более что
    // ширина всегда нулевой ширины
    itr_rct.set_x(disp_rct.get_x());
    itr_rct.set_width(disp_rct.get_width());
    // при интенсивном заполнении даже при таком поднятии могут происходить
    // одиночные непересечения, но "по инерции" все равно будет скроллироваться
    itr_rct.set_y(itr_rct.get_y()-itr_rct.get_height());

    bool at_end = !itr_rct.intersect(disp_rct).has_zero_area();

    int itr_off = itr.get_offset(); // после вставки не действителен
    Gtk::TextIter end_itr = buf->insert(itr, text);
    itr = buf->get_iter_at_offset(itr_off);

    if( at_end ) // скроллируем
    {
        RefPtr<Gtk::TextMark> end_mark = buf->create_mark("end of text", end_itr, false);
        txt_view.scroll_to(end_mark);
    }
    return TextIterRange(itr, end_itr);
}

static RefPtr<Gtk::TextTag> GetBufTag(RefPtr<Gtk::TextBuffer> buf, const std::string& tag_name,
                                      InitTagFunctor fnr)
{
    RefPtr<Gtk::TextTag> tag = buf->get_tag_table()->lookup(tag_name);
    if( !tag )
    {
        tag = buf->create_tag(tag_name);
        fnr(tag);
    }
    return tag;
}

void ApplyTag(Gtk::TextView& txt_view, const TextIterRange& tir, 
              const std::string& tag_name, InitTagFunctor fnr)
{
    RefPtr<Gtk::TextBuffer> buf = txt_view.get_buffer();
    RefPtr<Gtk::TextTag> tag = GetBufTag(buf, tag_name, fnr);
    buf->apply_tag(tag, tir.first, tir.second);
}

Gtk::Widget& PackDetails(Gtk::TextView& txt_view)
{
    return PackWidgetInFrame(PackInScrolledWindow(txt_view, true), Gtk::SHADOW_ETCHED_IN);
}
