// format_msg_type.hpp

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


#ifndef JT28092007_format_msg_type_HPP_DEFINED
#define JT28092007_format_msg_type_HPP_DEFINED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/logging/detail/fwd.hpp>
#include <boost/logging/detail/find_gather.hpp>
#include <boost/type_traits/remove_reference.hpp>


namespace boost { namespace logging {

template<class gather_msg , class write_msg > struct logger ;

namespace formatter {
    /** 
    @brief what is the default type of your string, in formatter_base ? See BOOST_LOG_FORMAT_MSG
    */
    template<class T = override> struct msg_type {
        typedef hold_string_type& type;
        typedef hold_string_type raw_type;
    };
}

namespace destination {
    /** 
    @brief what is the default type of your string, in formatter_base ? See BOOST_LOG_DESTINATION_MSG
    */
    template<class T = override> struct msg_type {
        // by default  - the default string
        typedef const hold_string_type& type;
        typedef hold_string_type raw_type;
    };
}


/* 
    for when compiling fast, and:
    - I know the gather type OR
    - use a default gather
*/
namespace detail {

    template<class T = override> struct fast_compile_with_default_gather {
        typedef typename boost::logging::formatter::msg_type<T>::type msg_type_ref;
        typedef typename boost::remove_reference<msg_type_ref>::type msg_type;

        typedef typename find_gather< msg_type >::type gather_msg;
        typedef logger< gather_msg, default_ > log_type;
    };

}


}}

#endif

