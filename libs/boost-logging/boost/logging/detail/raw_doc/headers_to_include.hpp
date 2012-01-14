namespace boost { namespace logging {

/** 
@page headers_to_include Headers to #include

- when using Formatters and Destinations

@code
// when declaring logs
#include <boost/logging/format_fwd.hpp>

// when defining logs and you don't use thread-safety
#include <boost/logging/format.hpp>

// when defining logs and you use thread-safety
#include <boost/logging/format_ts.hpp>
@endcode


- when using tags

@code
#include <boost/logging/tags.hpp>
@endcode


- when using Logging, without Formatters/Destinations

@code
#include <boost/logging/logging.hpp>
@endcode



*/

}}
