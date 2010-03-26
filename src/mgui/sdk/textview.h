#ifndef __MGUI_SDK_TEXTVIEW_H__
#define __MGUI_SDK_TEXTVIEW_H__

#include <mgui/mguiconst.h>

#include <mlib/function.h>

#include <string>
#include <utility>

Gtk::Widget& PackDetails(Gtk::TextView& txt_view);

typedef std::pair<Gtk::TextIter, Gtk::TextIter> TextIterRange;
typedef boost::function<void(RefPtr<Gtk::TextTag>)> InitTagFunctor;

TextIterRange AppendText(Gtk::TextView& txt_view, const std::string& text);
void ApplyTag(Gtk::TextView& txt_view, const TextIterRange& tir, 
              const std::string& tag_name, InitTagFunctor fnr);

TextIterRange AppendNewText(Gtk::TextView& txt_view, const std::string& line, bool is_out);
void AppendCommandText(Gtk::TextView& txt_view, const std::string& title);

#endif // #ifndef __MGUI_SDK_TEXTVIEW_H__

