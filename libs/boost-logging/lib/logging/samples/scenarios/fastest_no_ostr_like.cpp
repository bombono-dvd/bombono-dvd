// fastest_no_ostr_like.cpp
//
// Fastest. Does not use Formatters/Destinations. As logging syntax, uses the () operator

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
@example fastest_no_ostr_like.cpp

@copydoc fastest_no_ostr_like

@page fastest_no_ostr_like fastest_no_ostr_like.cpp Example


This usage:
- Fastest. It does not use formatters and destinations (thus, 
  the flexibility that comes with formatters/destinations) is gone
- You have one filter, which can be turned on or off
- You have 2 loggers: app and err.
- The app writes to console, the err writes to "err.txt" file
- the output is "dump" - no usage of << operator

Here's what the output will be:

The console:
@code
this is so cool
hello, world
good to be back ;)
@endcode

The err.txt file:
@code
first error 
second error 
@endcode


*/



#define BOOST_LOG_COMPILE_FAST_OFF
#include <boost/logging/logging.hpp>
#include <boost/logging/format.hpp>

using namespace boost::logging;

struct no_gather {
    const char * m_msg;
    no_gather() : m_msg(0) {}
    const char * msg() const { return m_msg; }
    void out(const char* msg) { m_msg = msg; }
    void out(const std::string& msg) { m_msg = msg.c_str(); }
};

// Step 1 : Specify your logging class(es)
typedef logger< no_gather, destination::cout > app_log_type;
typedef logger< no_gather, destination::file > err_log_type;

// Step 2 : Set up a filter
BOOST_DEFINE_LOG_FILTER(g_log_filter, filter::no_ts ) 

// Step 3: declare which loggers you'll use
BOOST_DEFINE_LOG(g_log_app, app_log_type)
BOOST_DEFINE_LOG_WITH_ARGS( g_log_err, err_log_type, ("err.txt") )

// FIXME most likely I can use BOOST_LOG_USE_IF_FILTER

// Step 4: define the macros through which you'll log
#define LAPP_ BOOST_LOG_USE_SIMPLE_LOG_IF_FILTER(g_log_app, g_log_filter->is_enabled() ) 
#define LERR_ BOOST_LOG_USE_SIMPLE_LOG_IF_FILTER(g_log_err, g_log_filter->is_enabled() ) 

void fastest_no_ostr_like_example() {
    // Step 5: use it...
    LAPP_("this is so cool\n");
    LERR_("first error \n");

    std::string hello = "hello", world = "world";
    LAPP_(hello + ", " + world + "\n");

    g_log_filter->set_enabled(false);
    LAPP_("this will not be written to the log");
    LAPP_("this won't be written to the log");
    LERR_("this error is not logged ");

    g_log_filter->set_enabled(true);
    LAPP_("good to be back ;) \n" );
    LERR_("second error \n" );

    // Step 6 : Enjoy!
}



#ifdef SINGLE_TEST

int main() {
    fastest_no_ostr_like_example();
}

#endif

// End of file

