//
// mgui/execution.cpp
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

#include "execution.h"
#include "dialog.h"
#include "gettext.h"

#include <mgui/author/execute.h> // ::Spawn()

#include <mlib/tech.h>
#include <signal.h> // SIGTERM


namespace Execution
{

void Stop(GPid& pid)
{
    ASSERT( pid != NO_HNDL );
    kill(pid, SIGTERM);
}

void Data::StopExecution(const std::string& what)
{
    // COPY_N_PASTE - тупо сделал содержимое сообщений как у "TSNAMI-MPEG DVD Author"
    // А что делать - нафига свои придумывать, если смысл один и тот же
    if( Gtk::RESPONSE_YES == MessageBox(BF_("You are about to cancel %1%. Are you sure?") % what 
                                            % bf::stop, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO) )
    {
        userAbort = true;
        if( pid != NO_HNDL ) // во время выполнения внешней команды
            Stop(pid);
    }
}

static bool PulseProgress(Gtk::ProgressBar& prg_bar)
{
    prg_bar.pulse();
    return true;
}

Pulse::Pulse(Gtk::ProgressBar& prg_bar) 
{ 
    tm.Connect(bl::bind(&PulseProgress, boost::ref(prg_bar)), 500); 
}

Pulse::~Pulse() 
{
    tm.Disconnect(); 
}

void SimpleSpawn(const char *commandline, const char* dir)
{
    GPid p = Spawn(dir, commandline);
    ASSERT_RTL( p > 0 );
    g_spawn_close_pid(p);
}

} // namespace Execution

