// log.cpp
#include "log.h"
#include <boost/logging/format.hpp>
#include <boost/logging/writer/ts_write.hpp>
#include <boost/logging/format/formatter/tags.hpp>

// uncomment if you want to use do logging on a dedicated thread
// #include <boost/logging/writer/on_dedicated_thread.hpp>

using namespace boost::logging;

// Step 6: Define the filters and loggers you'll use
BOOST_DEFINE_LOG_FILTER(g_log_filter, finder::filter ) 
BOOST_DEFINE_LOG(g_l, finder::logger) 


void init_logs() {
    // Add formatters and destinations
    // That is, how the message is to be formatted...
    g_l->writer().add_formatter( formatter::tag::thread_id() );
    g_l->writer().add_formatter( formatter::tag::time("$hh:$mm.$ss ") );
    g_l->writer().add_formatter( formatter::idx() );
    g_l->writer().add_formatter( formatter::append_newline() );

    //        ... and where should it be written to
    g_l->writer().add_destination( destination::cout() );
    g_l->writer().add_destination( destination::dbg_window() );
    g_l->writer().add_destination( destination::file("out.txt") );
}
