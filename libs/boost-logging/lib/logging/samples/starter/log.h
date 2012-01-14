// log.h
#ifndef LOG_H_header
#define LOG_H_header

#include <boost/logging/format_fwd.hpp>
// If you want to use tags...
#include <boost/logging/tags.hpp>

// Step 1: Specify the class to hold the message
namespace b_l = boost::logging;
typedef b_l::tag::holder<
    // string class
    b_l::optimize::cache_string_one_str<>,
    // tags
    b_l::tag::thread_id, b_l::tag::time> log_string_type;
// note: if you don't use tags, you can simply use a string class:
// typedef b_l::optimize::cache_string_one_str<> log_string_type;
BOOST_LOG_FORMAT_MSG( log_string_type )

// if not compiling fast...
#ifndef BOOST_LOG_COMPILE_FAST
#include <boost/logging/format.hpp>
#include <boost/logging/writer/ts_write.hpp>

// If you use tags...
#include <boost/logging/format/formatter/tags.hpp>

// uncomment if you want to use do logging on a dedicated thread
// #include <boost/logging/writer/on_dedicated_thread.hpp>
#endif


// Step 3 : Specify your logging class(es)
using namespace boost::logging::scenario::usage;
typedef use<
        //  how often do you manipulate (change) the filter?
        filter_::change::often<10>,
        //  does the filter use levels?
        filter_::level::no_levels,
        // how often do you manipulate (change) the logger?
        logger_::change::often<10>,
        // for the logger: do you favor speed or correctness?
        logger_::favor::correctness> finder;

// Step 4: declare which filters and loggers you'll use
BOOST_DECLARE_LOG_FILTER(g_l_filter, finder::filter)
BOOST_DECLARE_LOG(g_l, finder::logger)

// Step 5: define the macros through which you'll log
#define L_ BOOST_LOG_USE_LOG_IF_FILTER(g_l, g_log_filter->is_enabled() ) 

// initialize thy logs..
void init_logs();

#endif
