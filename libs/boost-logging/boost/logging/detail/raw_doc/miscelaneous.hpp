namespace boost { namespace logging {

/** 
@page miscelaneous Miscelaneous things

@section misc_use_defaults Template parameters - Using defaults

This parameter is optional. This means you don't need to set it, unless you want to.
Just leave it as @c default_, and move on to the paramers you're interested in.

Example:

@code
typedef logger_format_write< default_, default_, writer::threading::on_dedicated_thread > log_type;
@endcode


*/

}}
