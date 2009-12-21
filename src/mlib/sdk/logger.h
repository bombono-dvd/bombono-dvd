//
// mlib/sdk/logger.h
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

#ifndef __MLIB_SDK_LOGGER_H__
#define __MLIB_SDK_LOGGER_H__

#define BOOST_LOG_COMPILE_FAST_ON
#include <boost/logging/format_fwd.hpp>
// более быстрый вывод + теги
//BOOST_LOG_FORMAT_MSG( optimize::cache_string_one_str<> )
#include <boost/logging/format.hpp>

#include "stream_util.h" // strm << Point, Rect

#include <limits.h>

// есть надежда, что, раскрыв шаблон логгера вручную, время компиляции уменьшится
#define MANUAL_LOG
#ifdef MANUAL_LOG

typedef boost::logging::formatter::base<std::string&> formatter_base;
typedef boost::logging::destination::base<const std::string &> destination_base;

struct nolock_resource {
    template<class lock_type> struct finder {
        typedef typename boost::logging::locker::ts_resource<lock_type, boost::logging::threading::no_mutex > type;
    };
};

typedef boost::logging::logger<boost::logging::default_, 
    boost::logging::writer::format_write<formatter_base, destination_base, nolock_resource> > log_type;

#else

#include <boost/logging/writer/ts_write.hpp>
typedef boost::logging::logger_format_write< > log_type;

#endif

namespace Log
{

// Фильтр
enum LogLevel
{
    All   = INT_MIN,
    Debug2= 5000,
    Debug = 10000,
    Info  = 20000,
    Warn  = 30000,
    Error = 40000,
    Fatal = 50000,
    Trace = 50000,
    Off   = INT_MAX
};

// 
// LevelFilter - в зависимости от переменной окружения ATOM_LOG устанавливается
// уровень фильтрации логируемых сообщений
// 
struct LevelFilter
{
                LevelFilter(): logLvl(Off) {}

                // проверка - пропускаются ли сообщения уровня request_lvl
          bool  IsEnabled(int request_lvl) { return request_lvl >= logLvl; }
          void  SetLevel(int lvl) { logLvl = (LogLevel)lvl; }

    protected:
        LogLevel  logLvl;
};

struct Prepender
{
    const char* text;

        Prepender(const char* txt): text(txt) {}
};

//template<typename stream_type> inline stream_type& 
template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits>& 
operator << (std::basic_ostream<CharT, Traits>& strm, Prepender pr)
{
    strm.precision(8);
    strm << pr.text;
    return strm;
}

} // namespace Log


BOOST_DECLARE_LOG_FILTER( LogFilter, Log::LevelFilter ) 
BOOST_DECLARE_LOG(AppLogger, log_type) 

#define LOG_DBG2 BOOST_LOG_USE_LOG_IF_FILTER( AppLogger, LogFilter->IsEnabled(::Log::Debug2)) << ::Log::Prepender("[dbg2] ")
#define LOG_DBG  BOOST_LOG_USE_LOG_IF_FILTER( AppLogger, LogFilter->IsEnabled(::Log::Debug) ) << ::Log::Prepender("[dbg] ")
#define LOG_INF  BOOST_LOG_USE_LOG_IF_FILTER( AppLogger, LogFilter->IsEnabled(::Log::Info) )  << ::Log::Prepender("[inf] ")
#define LOG_WRN  BOOST_LOG_USE_LOG_IF_FILTER( AppLogger, LogFilter->IsEnabled(::Log::Warn) )  << ::Log::Prepender("[wrn] ")
#define LOG_ERR  BOOST_LOG_USE_LOG_IF_FILTER( AppLogger, LogFilter->IsEnabled(::Log::Error) ) << ::Log::Prepender("[err] ")
#define LOG_FTL  BOOST_LOG_USE_LOG_IF_FILTER( AppLogger, LogFilter->IsEnabled(::Log::Fatal) ) << ::Log::Prepender("[ftl] ")
#define LOG_TRC  BOOST_LOG_USE_LOG_IF_FILTER( AppLogger, LogFilter->IsEnabled(::Log::Trace) ) << ::Log::Prepender("[trc] ")

#endif // #ifndef __MLIB_SDK_LOGGER_H__

