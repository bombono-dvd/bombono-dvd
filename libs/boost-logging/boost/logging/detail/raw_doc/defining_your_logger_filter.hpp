namespace boost { namespace logging {

/** 
@page defining_your_logger_filter Declaring/Defining your logger/filter class(es)

When using the Boost Logging Lib, you need 2 things (see @ref workflow "Workflow"):
- a filter : which tells you if a logger is enabled or not. Note that you can use the same filter for multiple loggers - if you want. 
- a logger : which does the actual logging, once the filter is enabled


- @ref defining_your_filter 
    - @ref defining_your_filter_scenario 
    - @ref defining_your_filter_manually 
- @ref defining_your_logger 
    - @ref defining_your_logger_scenario 
    - @ref defining_your_logger_format_write 
    - @ref defining_your_logger_use_logger 





@section defining_your_filter Declaring/Defining your filter class

@subsection defining_your_filter_scenario Declare/Define your filter using scenarios (the easy way)

You can declare/define both your logger and filter based on how you'll use them (scenario::usage).
Thus, you'll deal with the filter like this:

@code
#include <boost/logging/format_fwd.hpp>
using namespace boost::logging::scenario::usage;
typedef use<
        // how often does the filter change?
        filter_::change::often<10>, 
        // does the filter use levels?
        filter_::level::no_levels, 
        // logger info
        ...
        > finder;

// declare filter
BOOST_DECLARE_LOG_FILTER(g_log_filter, finder::filter ) 

// define filter
BOOST_DEFINE_LOG_FILTER(g_log_filter, finder::filter ) 
@endcode


@subsection defining_your_filter_manually Declare/Define your filter manually

This is where you manually specify the filter class you want. There are multiple filter implementations:
- not using levels - the classes from the filter namespace
- using levels - the classes from the level namespace

Choose any you wish:

@code
#include <boost/logging/format_fwd.hpp>

// declare filter
BOOST_DECLARE_LOG_FILTER(g_log_filter, filter::no_ts ) 

BOOST_DEFINE_LOG_FILTER(g_log_filter, filter::no_ts ) 
@endcode



@section defining_your_logger Declaring/defining your logger class(es)

@subsection defining_your_logger_scenario Declare/Define your logger using scenarios (the very easy way)

When you use formatters and destinations, you can declare/define both your logger and filter based on how you'll use them (scenario::usage).
Thus, you'll deal with the logger like this:

@code
#include <boost/logging/format_fwd.hpp>

using namespace boost::logging::scenario::usage;
typedef use<
        // filter info
        ...,
        // how often does the logger change?
        logger_::change::often<10>, 
        // what does the logger favor?
        logger_::favor::speed> finder;

// declare
BOOST_DECLARE_LOG(g_log_err, finder::logger ) 

// define
BOOST_DEFINE_LOG(g_log_err, finder::logger ) 

@endcode




@subsection defining_your_logger_format_write Declare/Define your logger using logger_format_write (the easy way)

When you use formatters and destinations, you can use the logger_format_write class. The template params you don't want to set,
just leave them @c default_.

@code
#include <boost/logging/format_fwd.hpp>

namespace b_l = boost::logging;
typedef b_l::logger_format_write< b_l::default_, b_l::default_, b_l::writer::threading::on_dedicated_thread > log_type;

// declare
BOOST_DECLARE_LOG(g_l, log_type) 

// define
BOOST_DEFINE_LOG(g_l, log_type)
@endcode



@subsection defining_your_logger_use_logger Declare/Define your logger using the logger class

In case you don't use formatters and destinations, or have custom needs that the above methods can't satisfy, or
just like to do things very manually, you can use the logger class directly:

@code
#include <boost/logging/logging.hpp>

typedef logger< gather::ostream_like::return_str<>, destination::cout> log_type;

// declare
BOOST_DECLARE_LOG(g_l, log_type) 

// define
BOOST_DEFINE_LOG(g_l, log_type)
@endcode


*/

}}
