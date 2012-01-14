// mul_loggers_one_filter.cpp
//
// A test of the Logging library with multiple levels, multiple logging classes (each writing to multiple destinations) and one filter.

// Boost Logging library
//
// Author: John Torjo, www.torjo.com
//
// Copyright (C) 2007 John Torjo (see www.torjo.com for email)
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.
// See http://www.torjo.com/log2/ for more details


/**
@example mul_loggers_one_filter.cpp

@copydoc mul_loggers_one_filter 

@page mul_loggers_one_filter mul_loggers_one_filter.cpp Example


This usage:
- You have several loggers
- You have one filter, which can be turned on or off
- You want to format the message before it's written 
- Each logger has several log destinations

Optimizations:
- use a cache string (from optimize namespace), in order to make formatting the message faster

Logs:
- Error messages go into err.txt file
  - formatting - prefix each message by time, index, and append enter
- Info output goes to console, and a file called out.txt
  - formatting - prefix each message by time, "[app]", and append enter
- Debug messages go to the debug output window, and the console
  - formatting - prefix each message by "[dbg]", time, and append enter


Here's how the output will look like:

The debug output window:
@code
18:59.24 [dbg] this is so cool 1
18:59.24 [dbg] this is so cool again 2
@endcode


The console:
@code
18:59.24 [dbg] this is so cool 1
18:59.24 [dbg] this is so cool again 2
18:59.24 [app] hello, world
18:59.24 [app] good to be back ;) 4
@endcode


The out.txt file:
@code
18:59.24 [app] hello, world
18:59.24 [app] good to be back ;) 4
@endcode


The err.txt file
@code
18:59.24 [1] first error 3
18:59.24 [2] second error 5
@endcode
*/



#define BOOST_LOG_COMPILE_FAST_OFF
#include <boost/logging/format_fwd.hpp>

// Step 1: Optimize : use a cache string, to make formatting the message faster
BOOST_LOG_FORMAT_MSG( optimize::cache_string_one_str<> )

#include <boost/logging/format.hpp>
#include <boost/logging/writer/ts_write.hpp>
using namespace boost::logging;

// Step 3 : Specify your logging class(es)
typedef logger_format_write< > log_type;

// Step 4: declare which filters and loggers you'll use (usually in a header file)
BOOST_DECLARE_LOG_FILTER(g_log_filter, filter::no_ts ) 
BOOST_DECLARE_LOG(g_log_err, log_type) 
BOOST_DECLARE_LOG(g_log_app, log_type)
BOOST_DECLARE_LOG(g_log_dbg, log_type)

// Step 5: define the macros through which you'll log
#define LDBG_ BOOST_LOG_USE_LOG_IF_FILTER(g_log_dbg, g_log_filter->is_enabled() ) << "[dbg] "
#define LERR_ BOOST_LOG_USE_LOG_IF_FILTER(g_log_err, g_log_filter->is_enabled() )
#define LAPP_ BOOST_LOG_USE_LOG_IF_FILTER(g_log_app, g_log_filter->is_enabled() ) << "[app] "

// Step 6: Define the filters and loggers you'll use (usually in a source file)
BOOST_DEFINE_LOG_FILTER(g_log_filter, filter::no_ts ) 
BOOST_DEFINE_LOG(g_log_err, log_type)
BOOST_DEFINE_LOG(g_log_app, log_type)
BOOST_DEFINE_LOG(g_log_dbg, log_type)

void mul_logger_one_filter_example() {
    // Step 7: add formatters and destinations
    //         That is, how the message is to be formatted and where should it be written to

    // Err log
    g_log_err->writer().add_formatter( formatter::idx() );
    g_log_err->writer().add_formatter( formatter::time("$hh:$mm.$ss ") );
    g_log_err->writer().add_formatter( formatter::append_newline() );
    g_log_err->writer().add_destination( destination::file("err.txt") );

    // App log
    g_log_app->writer().add_formatter( formatter::time("$hh:$mm.$ss ") );
    g_log_app->writer().add_formatter( formatter::append_newline() );
    g_log_app->writer().add_destination( destination::file("out.txt") );
    g_log_app->writer().add_destination( destination::cout() );

    // Debug log
    g_log_dbg->writer().add_formatter( formatter::time("$hh:$mm.$ss ") );
    g_log_dbg->writer().add_formatter( formatter::append_newline() );
    g_log_dbg->writer().add_destination( destination::dbg_window() );
    g_log_dbg->writer().add_destination( destination::cout() );

    // Step 8: use it...
    int i = 1;
    LDBG_ << "this is so cool " << i++;
    LDBG_ << "this is so cool again " << i++;
    LERR_ << "first error " << i++;

    std::string hello = "hello", world = "world";
    LAPP_ << hello << ", " << world;

    g_log_filter->set_enabled(false);
    LDBG_ << "this will not be written anywhere";
    LAPP_ << "this won't be written anywhere either";
    LERR_ << "this error is not logged " << i++;

    g_log_filter->set_enabled(true);
    LAPP_ << "good to be back ;) " << i++;
    LERR_ << "second error " << i++;

    // Step 9 : Enjoy!
}



#ifdef SINGLE_TEST

int main() {
    mul_logger_one_filter_example();
}

#endif

// End of file

