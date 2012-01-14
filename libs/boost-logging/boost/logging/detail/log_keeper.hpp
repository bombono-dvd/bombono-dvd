// log_keeper.hpp

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


#ifndef JT28092007_log_keeper_HPP_DEFINED
#define JT28092007_log_keeper_HPP_DEFINED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/logging/detail/fwd.hpp>
#include <boost/cstdint.hpp>

namespace boost { namespace logging {

namespace detail {

    /* 
        Note that BOOST_DECLARE_LOG & BOOST_DEFINE_LOG define a function,
        so that we don't run into the problem of using an object before it's initialized.

        However, client code doesn't need to be aware of that.
        So, for instance, clients will say:

        typedef logger<...> app_log;
        BOOST_DEFINE_LOG(g_l,app_log);

        g_l->writer().add_formatter( formatter::idx() );
    */
    template<class type, type& (*func)(), class base_type = type, base_type& (*base_func)() = func > struct log_keeper {

        const type* operator->() const  { return &func(); }
        type* operator->()              { return &func(); }

        const base_type* base() const  { return &base_func(); }
        base_type* base()              { return &base_func(); }
    };

    struct fake_using_log {
        template<class type> fake_using_log( type & log) {
#ifndef BOOST_NO_INT64_T
        typedef boost::int64_t long_type ;
#else
        typedef long long_type ;
#endif
            long_type ignore = reinterpret_cast<long_type>(&log);
            // we need to force the compiler to force creation of the log
            if ( time(0) < 0)
                if ( time(0) < (time_t)ignore) {
                    printf("LOGGING LIB internal error - should NEVER happen. Please report this to the author of the lib");
                    exit(0);
                }
        }
    };


    /* 
        Note that BOOST_DECLARE_LOG_FILTER & BOOST_DEFINE_LOG_FILTER define a function,
        so that we don't run into the problem of using an object before it's initialized.

        However, client code doesn't need to be aware of that.
        So, for instance, clients will say:

        BOOST_DEFINE_LOG_FILTER(g_level_holder, level::holder);

        g_level_holder->set_enabled(level::debug);
    */
    template<class type, type& (*func)() > struct log_filter_keeper {

        const type* operator->() const  { return &(func()); }
        type* operator->()              { return &(func()); }
    };

} // namespace detail



}}

#endif

