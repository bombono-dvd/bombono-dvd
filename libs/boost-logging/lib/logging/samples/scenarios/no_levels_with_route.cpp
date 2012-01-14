// no_levels_with_route.cpp
//
// A test of the Logging library with no levels and one logging class writing to multiple destinations, using a custom route.

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
@example no_levels_with_route.cpp

@copydoc no_levels_with_route

@page no_levels_with_route no_levels_with_route.cpp Example

This usage:
- There are no levels
- There is only one logger
- The logger has multiple destinations
- We use a custom route

A custom route means you don't want to first run all formatters, and then write to all destinations.
Depending on the destination, you'll want a certain formatting of the message

In our example:
@code
to cout:        [idx] [time] message [enter]
to dbg_window:  [time] message [enter]
to file:        [idx] message [enter]
@endcode

We will use an @c apply_format_and_write class that caches the formatting, so that it'll format faster
(more specifically, the boost::logging::format_and_write::use_cache, together with boost::logging::optimize::cache_string_several_str).

The output will be similar to this:

The debug window
@code
12:15.12 this is so cool 1
12:15.12 hello, world
12:15.12 good to be back ;) 2
@endcode

The file:
@code
[1] this is so cool 1
[2] hello, world
[3] good to be back ;) 2
@endcode

The console:
@code
[1] 12:15.12 this is so cool 1
[2] 12:15.12 hello, world
[3] 12:15.12 good to be back ;) 2
@endcode

*/



#define BOOST_LOG_COMPILE_FAST_OFF
#include <boost/logging/format_fwd.hpp>

// Step 1: Optimize : use a cache string, to make formatting the message faster
BOOST_LOG_FORMAT_MSG( optimize::cache_string_several_str<> )

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

void no_levels_with_route_example() {
    // Step 7: add formatters and destinations
    //         That is, how the message is to be formatted...
    g_l->writer().add_formatter( formatter::idx() );
    g_l->writer().add_formatter( formatter::time("$hh:$mm.$ss ") );
    g_l->writer().add_formatter( formatter::append_newline() );

    //        ... and where should it be written to
    g_l->writer().add_destination( destination::cout() );
    g_l->writer().add_destination( destination::dbg_window() );
    g_l->writer().add_destination( destination::file("out.txt") );

    // Now, specify the route
    g_l->writer().router().set_route()
        .fmt( formatter::time("$hh:$mm.$ss ") ) 
        .fmt( formatter::append_newline() )
        .fmt( formatter::idx() )
        .clear()
        .fmt( formatter::time("$hh:$mm.$ss ") ) 
        .fmt( formatter::append_newline() )
        .dest( destination::dbg_window() )
        .clear()
        .fmt( formatter::idx() )
        .fmt( formatter::time("$hh:$mm.$ss ") ) 
        .fmt( formatter::append_newline() )
        .dest( destination::cout() )
        .clear()
        .fmt( formatter::idx() )
        .fmt( formatter::append_newline() )
        .dest( destination::file("out.txt") );

    // Step 8: use it...
    int i = 1;
    L_ << "this is so cool " << i++;

    std::string hello = "hello", world = "world";
    L_ << hello << ", " << world;

    g_log_filter->set_enabled(false);
    L_ << "this will not be written anywhere";
    L_ << "this won't be written anywhere either";

    g_log_filter->set_enabled(true);
    L_ << "good to be back ;) " << i++;

    // Step 9 : Enjoy!
}



#ifdef SINGLE_TEST

int main() {
    no_levels_with_route_example();
}

#endif

// End of file

