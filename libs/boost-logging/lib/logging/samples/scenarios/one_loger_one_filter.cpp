// one_loger_one_filter.cpp
//
// A test of the Logging library with one logger and one filter

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
@example one_loger_one_filter.cpp

@copydoc one_loger_one_filter

@page one_loger_one_filter one_loger_one_filter.cpp Example


This usage:
- You have one logger
- You have one filter, which can be turned on or off
- You want to format the message before it's written 
- The logger has several log destinations
    - The output goes to console, debug output window, and a file called out.txt
    - Formatting - prefix each message by its index, and append enter

Optimizations:
- use a cache string (from optimize namespace), in order to make formatting the message faster

In this example, all output will be written to the console, debug window, and "out.txt" file.
It will be:

@code
[1] this is so cool 1
[2] this is so cool again 2
[3] hello, world
[4] good to be back ;) 3
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
BOOST_DECLARE_LOG(g_l, log_type) 

// Step 5: define the macros through which you'll log
#define L_ BOOST_LOG_USE_LOG_IF_FILTER(g_l, g_log_filter->is_enabled() ) 

// Step 6: Define the filters and loggers you'll use (usually in a source file)
BOOST_DEFINE_LOG_FILTER(g_log_filter, filter::no_ts ) 
BOOST_DEFINE_LOG(g_l, log_type)


void one_logger_one_filter_example() {
    // Step 7: add formatters and destinations
    //         That is, how the message is to be formatted and where should it be written to

    g_l->writer().add_formatter( formatter::idx() );
    g_l->writer().add_formatter( formatter::append_newline_if_needed() );
    g_l->writer().add_destination( destination::file("out.txt") );
    g_l->writer().add_destination( destination::cout() );
    g_l->writer().add_destination( destination::dbg_window() );

    // Step 8: use it...
    int i = 1;
    L_ << "this is so cool " << i++;
    L_ << "this is so cool again " << i++;

    std::string hello = "hello", world = "world";
    L_ << hello << ", " << world;

    g_log_filter->set_enabled(false);
    L_ << "this will not be written to the log";
    L_ << "this won't be written to the log";

    g_log_filter->set_enabled(true);
    L_ << "good to be back ;) " << i++;

    // Step 9 : Enjoy!
}



#ifdef SINGLE_TEST

int main() {
    one_logger_one_filter_example();
}

#endif

// End of file

