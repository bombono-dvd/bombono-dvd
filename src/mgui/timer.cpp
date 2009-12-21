//
// mgui/timer.cpp
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

#include <mgui/_pc_.h>

#include "timer.h"

static gboolean EsCB(gpointer data)
{
    BoolFnr& fnr = *(BoolFnr*)data;
    return fnr() ? TRUE : FALSE ;
}

static void EsDestroyCB(gpointer data)
{
    BoolFnr* p_fnr = (BoolFnr*)data;
    delete p_fnr;
}

// сделано по типу g_timeout_add_full()/g_idle_add_full()
static guint ConnectSource(const BoolFnr& fnr, GSource* new_src,
                           GMainContext* main_cnxt)
{
    // разрешаем двойное соед. без разъединения.
    //ASSERT( !timerId ); 

    BoolFnr* fnr_copy = new BoolFnr(fnr);

    //if(priority != G_PRIORITY_DEFAULT)
    //    g_source_set_priority(source, priority);

    g_source_set_callback(new_src, &EsCB, fnr_copy, &EsDestroyCB);

    int src_id = g_source_attach(new_src, main_cnxt); // контекст по умолчанию
    g_source_unref(new_src);
    return src_id;
}

void EventSource::ConnectTimer(const BoolFnr& fnr, int interval)
{
    sourceId = ConnectSource(fnr, g_timeout_source_new(interval), mainCnxt);
}

void EventSource::ConnectIdle(const BoolFnr& fnr)
{
    sourceId = ConnectSource(fnr, g_idle_source_new(), mainCnxt);
}

void EventSource::Disconnect()
{
    if( sourceId )
    {
        GSource* src = g_main_context_find_source_by_id(mainCnxt, sourceId);
        if( src )
            g_source_destroy(src);
        sourceId = 0;
    }
}

