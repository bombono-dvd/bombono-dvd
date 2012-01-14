// ded_loger_one_filter.cpp
//
// A test of the Logging library with one logger and one filter. The logger is thread-safe, writing on a dedicated thread.

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
@example ded_loger_one_filter.cpp

@copydoc ded_loger_one_filter

@page ded_loger_one_filter ded_loger_one_filter.cpp Example


This usage:
- You have one @b thread-safe logger - the logging is done @ref boost::logging::writer::on_dedicated_thread "on a dedicated thread"
- You have one filter, which is always turned on
- You want to format the message before it's written 
- The logger has several log destinations
    - The output goes debug output window, and a file called out.txt
    - Formatting - prefix each message by time, its index, and append enter

Optimizations:
- use a cache string (from optimize namespace), in order to make formatting the message faster

In this example, all output will be written to the console, debug window, and "out.txt" file.
It will look similar to:

@code
...
30:33 [10] message 1
30:33 [11] message 2
30:33 [12] message 2
30:33 [13] message 2
30:33 [14] message 2
30:33 [15] message 3
30:33 [16] message 2
30:33 [17] message 3
30:33 [18] message 3
30:33 [19] message 4
30:33 [20] message 3
30:33 [21] message 3
30:33 [22] message 4
30:33 [23] message 4
30:33 [24] message 4
30:33 [25] message 4
30:33 [26] message 5
30:33 [27] message 5
30:33 [28] message 6
30:33 [29] message 6
30:33 [30] message 5
30:33 [31] message 5
30:33 [32] message 5
30:33 [33] message 6
30:33 [34] message 7
...
@endcode

*/



#define BOOST_LOG_COMPILE_FAST_OFF
#include <boost/logging/format_fwd.hpp>
// Step 1: Optimize : use a cache string, to make formatting the message faster
BOOST_LOG_FORMAT_MSG( optimize::cache_string_one_str<> )

#include <boost/logging/format_ts.hpp>
#include <boost/logging/format/formatter/thread_id.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

using namespace boost::logging;

// Step 3 : Specify your logging class(es)
//          In our case, we're also writing on a dedidcated thread
typedef logger_format_write< default_, default_, writer::threading::on_dedicated_thread > log_type;


// Step 4: declare which filters and loggers you'll use (usually in a header file)
BOOST_DECLARE_LOG_FILTER(g_log_filter, filter::no_ts ) 
BOOST_DECLARE_LOG(g_l, log_type) 

// Step 5: define the macros through which you'll log
#define L_ BOOST_LOG_USE_LOG_IF_FILTER(g_l, g_log_filter->is_enabled() ) 

// Step 6: Define the filters and loggers you'll use (usually in a source file)
BOOST_DEFINE_LOG_FILTER(g_log_filter, filter::no_ts ) 
BOOST_DEFINE_LOG(g_l, log_type)

void do_sleep(int ms) {
    using namespace boost;
    xtime next;
    xtime_get( &next, TIME_UTC);
    next.nsec += (ms % 1000) * 1000000;

    int nano_per_sec = 1000000000;
    next.sec += next.nsec / nano_per_sec;
    next.sec += ms / 1000;
    next.nsec %= nano_per_sec;
    thread::sleep( next);
}

void use_log_thread() {
    // Step 8: use it...
    for ( int i = 0; i < 20; ++i) {
        L_ << "message " << i ;
        do_sleep(1);
    }

    // Step 9 : Enjoy!
}

void ts_logger_one_filter_example() {
    // Step 7: add formatters and destinations
    //         That is, how the message is to be formatted and where should it be written to

    g_l->writer().add_formatter( formatter::idx() );
    g_l->writer().add_formatter( formatter::time("$mm:$ss ") );
    g_l->writer().add_formatter( formatter::append_newline() );
    g_l->writer().add_destination( destination::file("out.txt") );
    g_l->writer().add_destination( destination::dbg_window() );

    for ( int i = 0 ; i < 5; ++i)
        boost::thread t( &use_log_thread);

    // allow for all threads to finish
    do_sleep( 5000);
}



#ifdef SINGLE_TEST

int main() {
    ts_logger_one_filter_example();
}

#endif

// End of file

