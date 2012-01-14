namespace boost { namespace logging {

/** 
@page common_scenarios Usage Scenarios (together with code)

- @ref common_scenarios_1
- @ref common_scenarios_2
- @ref common_scenarios_3
- @ref common_scenarios_4
- @ref common_scenarios_5
- @ref common_scenarios_6
- @ref common_scenarios_7
- @ref common_scenarios_8
- @ref common_scenarios_9
- @ref common_scenarios_10

- @ref common_your_scenario

- @ref scenario_multiple_files
    - @ref scenario_multiple_files_program 
    - @ref scenario_multiple_files_log_h 
    - @ref scenario_multiple_files_log_cpp 
    - @ref scenario_multiple_files_main 

\n\n\n
@copydoc common_usage_steps_fd 



\n\n\n
@section common_scenarios_1 Scenario 1, Common usage: Multiple levels, One logging class, Multiple destinations.

Scenario 1 should be the most common.

@copydoc mul_levels_one_logger 

@ref scenarios_code_1 "Click to see the code"
\n\n\n



@section common_scenarios_2 Scenario 2: Multiple levels, Multiple logging classes, Multiple destinations.

@copydoc mul_levels_mul_logers 

@ref scenarios_code_2 "Click to see the code"
\n\n\n



@section common_scenarios_3 Scenario 3: No levels, One Logger, Multiple destinations, Custom route

@copydoc no_levels_with_route

@ref scenarios_code_3 "Click to see the code"
\n\n\n



@section common_scenarios_4 Scenario 4: No levels, Multiple Loggers, One Filter

@copydoc mul_loggers_one_filter

@ref scenarios_code_4 "Click to see the code"
\n\n\n



@section common_scenarios_5 Scenario 5: No levels, One Logger, One Filter

@copydoc one_loger_one_filter

@ref scenarios_code_5 "Click to see the code"
\n\n\n



@section common_scenarios_6 Scenario 6: Fastest: Multiple Loggers, One Filter, Not using Formatters/Destinations, Not using <<

@copydoc fastest_no_ostr_like

@ref scenarios_code_6 "Click to see the code"
\n\n\n



@section common_scenarios_7 Scenario 7: Fast: Multiple Loggers, One Filter, Not using Formatters/Destinations, Using <<

@copydoc fastest_use_ostr_like

@ref scenarios_code_7 "Click to see the code"
\n\n\n



@section common_scenarios_8 Scenario 8: Using custom formatters/destinations together with the existing formatters/destinations

@copydoc custom_fmt_dest

@ref scenarios_code_8 "Click to see the code"
\n\n\n



@section common_scenarios_9 Scenario 9: One Thread-safe Logger, One Filter

@copydoc ts_loger_one_filter

@ref scenarios_code_9 "Click to see the code"
\n\n\n


@section common_scenarios_10 Scenario 10: One Logger on a dedicated thread, One Filter

@copydoc ded_loger_one_filter

@ref scenarios_code_10 "Click to see the code"
\n\n\n


@section common_your_scenario Your Scenario : Find out logger and filter, based on your application's needs

@copydoc your_scenario

@ref common_your_scenario_code "Click to see the code"
\n\n\n



@section common_scenario_using_tags Using tags : file/line, thread id and time

@copydoc using_tags

@ref scenarios_code_using_tags "Click to see the code"

*/

}}
