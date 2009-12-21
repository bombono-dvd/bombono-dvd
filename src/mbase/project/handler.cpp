//
// mbase/project/handler.cpp
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

#include <mbase/_pc_.h>

#include <mlib/string.h>

#include "handler.h"
#include "table.h"

namespace Project
{

typedef std::vector<MHandler> HandlerArr;
typedef std::vector<HookHandler> HookArr;

struct LtStr
{
    bool operator()(const char* s1, const char* s2) const
    {
        return strcmp(s1, s2) < 0;
    }
};

struct HandlerSet
{
    typedef std::map<const char*, HandlerArr, LtStr> Map;
    typedef Map::iterator Itr;

        Map  map;
    HookArr  hookArr;
};

static HandlerSet& GetHandlerSet()
{
    return AData().GetData<HandlerSet>();
}

void RegisterHandler(MHandler hdlr, const char* action)
{
    HandlerArr& arr = GetHandlerSet().map[action];
    arr.push_back(hdlr);
}

void RegisterHook(HookHandler hook)
{
    GetHandlerSet().hookArr.push_back(hook);
}

void InvokeOn(MediaItem mi, const char* action)
{
    HandlerSet& hs = GetHandlerSet();
    for( HookArr::iterator itr = hs.hookArr.begin(), end = hs.hookArr.end(); itr != end; ++itr )
        (*itr)(mi, action);

    HandlerArr& arr = hs.map[action];
    for( HandlerArr::iterator itr = arr.begin(), end = arr.end(); itr != end; ++itr )
    {
        MHandler hdlr = *itr;
        mi->Accept(*hdlr);
    }
}

} // namespace Project

