//
// mgui/timer.h
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

#ifndef __MGUI_TIMER_H__
#define __MGUI_TIMER_H__

#include <boost/function.hpp> // для BoolFnr

typedef boost::function<bool()> BoolFnr;
//
// По функционалу очень похож на Glib::signal_timeout()/Glib::signal_idle(), 
// но есть разница,- в любой момент можно явно отключиться с помощью Disconnect(); 
// а со временем снова подключиться
// 
class EventSource
{
    public:
                EventSource(GMainContext* cnxt = 0): sourceId(0), mainCnxt(cnxt) {}

          void  ConnectTimer(const BoolFnr& fnr, int interval);
          void  ConnectIdle(const BoolFnr& fnr);

          void  Disconnect();

           int  Id() { return sourceId; }

    protected:

            int  sourceId;
   GMainContext* mainCnxt; 
};

// удобный интерфейс
class Timer: protected EventSource
{
    typedef EventSource MyParent;
    public:
            Timer(GMainContext* cnxt = 0): MyParent(cnxt) {}

      void  Connect(const BoolFnr& fnr, int interval) { ConnectTimer(fnr, interval); }
      void  Disconnect() { MyParent::Disconnect(); }
};

#endif // __MGUI_TIMER_H__

