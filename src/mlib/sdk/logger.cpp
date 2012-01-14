//
// mlib/sdk/logger.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#include "logger.h"

#include <strings.h> // strcasecmp()

using namespace boost::logging;

BOOST_DEFINE_LOG_FILTER( LogFilter, Log::LevelFilter )
BOOST_DEFINE_LOG(AppLogger, log_type)

struct CerrDest: destination::class_<CerrDest, destination::implement_op_equal::no_context>
{
        void  operator()(param msg) const;
};

void CerrDest::operator()(param msg) const
{
    io::cerr << msg;
}

void InitLogFunc()
{
    // 1 форматирование лога
    //AppLogger->writer().add_formatter( formatter::idx() );
    //AppLogger->writer().add_formatter( formatter::append_newline_if_needed() );

    // 2 - установка фильтра и выводов
    const char* log_val = getenv("ATOM_LOG");
    if( !log_val )
        log_val = "Warn";
    std::stringstream strm(log_val);

    std::string str_val;
    strm >> str_val;

    Log::LogLevel lvl = Log::Off;
    if( strcasecmp(str_val.c_str(), "Debug2") == 0 )
        lvl = Log::Debug2;
    else if( strcasecmp(str_val.c_str(), "Debug") == 0 )
        lvl = Log::Debug;
    else if( strcasecmp(str_val.c_str(), "Info") == 0 )
        lvl = Log::Info;
    else if( strcasecmp(str_val.c_str(), "Warn") == 0 )
        lvl = Log::Warn;
    else if( strcasecmp(str_val.c_str(), "Error") == 0 )
        lvl = Log::Error;
    else if( strcasecmp(str_val.c_str(), "Fatal") == 0 )
        lvl = Log::Fatal;
    else
        std::cerr << "Bad ATOM_LOG environment variable!" << std::endl;
    LogFilter->SetLevel(lvl);

    bool set_cerr = lvl != Log::Off;
    while( strm >> str_val )
    {
        set_cerr = false;
        if( strcasecmp(str_val.c_str(), "cout") == 0 )
            AppLogger->writer().add_destination( destination::cout() );
        else if( strcasecmp(str_val.c_str(), "cerr") == 0 )
            AppLogger->writer().add_destination( CerrDest() );
        else
            AppLogger->writer().add_destination( 
                destination::file(str_val, destination::
                                  file_settings().initial_overwrite(true).do_append(false)) );
    }
    // по умолчанию пишем в cerr
    if( set_cerr )
        AppLogger->writer().add_destination( CerrDest() );
}

namespace 
{

struct InitLog
{
    InitLog();
};
// глобальный объект
InitLog InitLogInstance;

InitLog::InitLog()
{
    InitLogFunc();
}

} // namespace

