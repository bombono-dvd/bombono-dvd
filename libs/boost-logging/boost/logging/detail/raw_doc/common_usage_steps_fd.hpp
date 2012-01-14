namespace boost { namespace logging {

/** 
@page common_usage_steps_fd Common steps when using Formatters and destinations

The usual steps when using the Boost Logging Lib are:
- Step 1: (optional) Specify your @ref BOOST_LOG_FORMAT_MSG "format message class" and/or @ref BOOST_LOG_DESTINATION_MSG "destination message class". By default, it's <tt>std::(w)string</tt>.
  You'll use this when you want a @ref optimize "optimize string class".
- Step 2: (optional) Specify your @ref boost::logging::manipulator "formatter & destination base classes"
- Step 3: @ref defining_your_logger "Specify your logger class(es)"
- Step 4: Declare the @ref defining_your_filter "filters" and @ref defining_your_logger "loggers" you'll use (in a header file)
- Step 5: Define the @ref macros_use "macros through which you'll do logging"
- Step 6: Define the @ref defining_your_logger "loggers" and the @ref defining_your_filter "filters" you'll use (in a source file). We need this separation
  (into declaring and defining the logs/filters), in order to @ref macros_compile_time "make compilation times fast".
- Step 7: Add @ref boost::logging::manipulator "formatters and destinations". That is, how the message is to be formatted...
- Step 8: @ref manipulator_use_it "Use it"
- Step 9: Enjoy the results!

*/

}}
