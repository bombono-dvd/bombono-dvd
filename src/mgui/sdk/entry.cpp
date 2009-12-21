//
// mgui/sdk/entry.cpp
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

#include "entry.h"

const char DigitChars[]   = "0123456789";

class SignalHandlersBlocker
{
    public: 
            SignalHandlersBlocker(GObject* obj, void* p_func, void* d)
                : gObj(obj), pFunc(p_func), data(d)
            { g_signal_handlers_block_by_func(gObj, pFunc, data); }

           ~SignalHandlersBlocker()
            { g_signal_handlers_unblock_by_func(gObj, pFunc, data); }

    protected:
        GObject* gObj;
           void* pFunc;
           void* data;
};

static void OnInsertText(GtkEntry* ent, const gchar* text, gint length,
                         gint* position, gpointer data)
{
    const char* allowed_chars = (const char*)data;
    std::string ins_str;
    for( int i=0; i<length; i++ )
        if( strchr(allowed_chars, text[i]) )
            ins_str += text[i];

    if( ins_str.size() )
    {
        SignalHandlersBlocker shb(G_OBJECT(ent), (void*)OnInsertText, data);

        gtk_editable_insert_text(GTK_EDITABLE(ent), ins_str.c_str(), ins_str.size(), position);
    }
    g_signal_stop_emission_by_name(G_OBJECT(ent), "insert_text");
}

void LimitTextInput(Gtk::Entry& ent, const char* allowed_chars)
{
    // :TODO: стоит попытаться по-другому блокировать сигнал, не через SignalHandlersBlocker
    g_signal_connect(G_OBJECT(ent.gobj()), "insert_text", G_CALLBACK(OnInsertText), 
                     (void*)allowed_chars);
}

