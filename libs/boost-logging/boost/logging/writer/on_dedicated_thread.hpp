// on_dedicated_thread.hpp

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


#ifndef JT28092007_on_dedicated_thread_HPP_DEFINED
#define JT28092007_on_dedicated_thread_HPP_DEFINED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/logging/detail/fwd.hpp>
#include <boost/logging/detail/forward_constructor.hpp>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/bind.hpp>
#include <boost/logging/detail/manipulator.hpp> // boost::logging::manipulator

namespace boost { namespace logging { namespace writer {

/** @file boost/logging/writer/on_dedidcated_thread.hpp
*/

namespace detail {
    template<class msg_type> struct dedicated_thread_context {
        dedicated_thread_context() : is_working(true), write_period_ms(100) {}

        bool is_working;
        int write_period_ms;

        boost::logging::threading::mutex cs;

        // the thread doing the write
        typedef boost::shared_ptr<boost::thread> thread_ptr;
        thread_ptr writer;

        // ... so that reallocations are fast
        typedef boost::shared_ptr<msg_type> ptr;
        typedef std::vector<ptr> array;
        array msgs;
    };
}

/** 
@brief Performs all writes on a dedicated thread  - very efficient and <b>thread-safe</b>. 

<tt>\#include <boost/logging/writer/on_dedicated_thread.hpp> </tt>

Keeps locks in the worker threads to a minimum:
whenever a message is logged, is put into a queue (this is how long the lock lasts).
Then, a dedicated thread reads the queue, and processes the messages (applying formatters and destinations if needed).

@section on_dedicated_thread_logger Transforming a logger into on-dedicated-thread writer

To transform a @b logger into on-dedicated-thread (thread-safe) writer, simply specify @c on_dedicated_thread as the thread safety:

@code
typedef logger_format_write< default_, default_, writer::threading::on_dedicated_thread > log_type;
@endcode

Of if you're using @ref boost::logging::scenario::usage scenarios, specify @c speed for the @c logger::favor_ :
@code
using namespace boost::logging::scenario::usage;
typedef use< ..., ..., ..., logger_::favor::speed> finder;
@endcode



\n\n
@section on_dedicated_thread_writer Transforming a writer into on-dedicated-thread writer

To transform a @b writer into on-dedicated-thread thread-safe writer, simply surround the writer with @c on_dedicated_thread:

Example:

@code
typedef gather::ostream_like::return_str<> string;

// not thread-safe
logger< string, write_to_cout> g_l;

// thread-safe, on dedicated thread
logger< string, on_dedicated_thread<string,write_to_cout> > g_l;
@endcode

You should note that a @b writer is not necessary a %logger. It can be a destination, for instance. For example, you might have a destination
where writing is time consuming, while writing to the rest of the destinations is very fast. 
You can choose to write to all but that destination on the current thread, and to that destination on a dedicated thread.
(If you want to write to all destinations on a different thread, we can go back to @ref on_dedicated_thread_logger "transforming a logger...")

*/
template<class msg_type, class base_type> 
struct on_dedicated_thread 
        : base_type, 
          boost::logging::manipulator::non_const_context<detail::dedicated_thread_context<msg_type> > {

    typedef on_dedicated_thread<msg_type,base_type> self_type;
    typedef typename detail::dedicated_thread_context<msg_type> context_type;
    typedef typename boost::logging::manipulator::non_const_context<detail::dedicated_thread_context<msg_type> > non_const_context_base;

    typedef boost::logging::threading::mutex::scoped_lock scoped_lock;

    on_dedicated_thread() {}
    BOOST_LOGGING_FORWARD_CONSTRUCTOR(on_dedicated_thread,base_type)

    /** 
        @brief Sets the write period : on the dedicated thread (in milliseconds)
    */
    void write_period_ms(int period_ms) {
        scoped_lock lk( non_const_context_base::context().cs);
        non_const_context_base::context().write_period_ms = period_ms;
    }

    ~on_dedicated_thread() {
        boost::shared_ptr<boost::thread> writer;
        { scoped_lock lk( non_const_context_base::context().cs);
          non_const_context_base::context().is_working = false;
          writer = non_const_context_base::context().writer;
        }

        if ( writer)
            writer->join();

        // write last messages, if any
        write_array();
    }

//    void operator()(const msg_type & msg) const {
    void operator()(msg_type & msg) const {
        typedef typename context_type::ptr ptr;
        typedef typename context_type::thread_ptr thread_ptr;
        //ptr new_msg(new msg_type(msg));
        ptr new_msg(new msg_type);
        std::swap(msg, *new_msg);

        scoped_lock lk( non_const_context_base::context().cs);
        if ( !non_const_context_base::context().writer) 
            non_const_context_base::context().writer = thread_ptr( new boost::thread( boost::bind(&self_type::do_write,this) ));

        non_const_context_base::context().msgs.push_back(new_msg);
    }
private:
    void do_write() const {
        const int NANOSECONDS_PER_SECOND = 1000 * 1000 * 1000;

        int sleep_ms = 0;
        while ( true) {
            { scoped_lock lk( non_const_context_base::context().cs);
              // refresh it - just in case it got changed...
              sleep_ms = non_const_context_base::context().write_period_ms;
              if ( !non_const_context_base::context().is_working)
                  break; // we've been destroyed
            }

            boost::xtime to_wait;
            xtime_get(&to_wait, boost::TIME_UTC);
            to_wait.sec += sleep_ms / 1000;
            to_wait.nsec += (sleep_ms % 1000) * (NANOSECONDS_PER_SECOND / 1000);
            to_wait.sec += to_wait.nsec / NANOSECONDS_PER_SECOND ;
            to_wait.nsec %= NANOSECONDS_PER_SECOND ;
            boost::thread::sleep( to_wait);

            write_array();
        }
    }

    void write_array() const {
        typedef typename context_type::array array;

        array msgs;
        { scoped_lock lk( non_const_context_base::context().cs);
          std::swap( non_const_context_base::context().msgs, msgs);
          // reserve elements - so that we don't get automatically resized often
          non_const_context_base::context().msgs.reserve( msgs.size() );
        }

        for ( typename array::iterator b = msgs.begin(), e = msgs.end(); b != e; ++b)
            base_type::operator()(*(b->get()));
    }
};


}}}

#endif

