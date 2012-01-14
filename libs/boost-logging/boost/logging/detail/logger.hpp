// logger.hpp

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


#ifndef JT28092007_logger_HPP_DEFINED
#define JT28092007_logger_HPP_DEFINED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/logging/detail/fwd.hpp>
#include <boost/logging/detail/forward_constructor.hpp>
#include <boost/logging/detail/find_gather.hpp>

namespace boost { namespace logging {


    
    template<class holder, class gather_type> struct gather_holder {
        gather_holder(const holder & p_this) : m_this(p_this), m_use(true) {}
        
        gather_holder(const gather_holder & other) : m_this(other.m_this), m_use(true) {
            other.m_use = false;
        }

        ~gather_holder() { 
            // FIXME handle exiting from exceptions!!!
            if ( m_use)
                m_this.on_do_write(m_obj); 
        }
        gather_type & gather() { return m_obj; }
    private:
        const holder & m_this;
        mutable gather_type m_obj;
        mutable bool m_use;
    };

    namespace detail {
        template<class type> type& as_non_const(const type & t) { return const_cast<type&>(t); }
    }



    /** 
    @brief The logger class. Every log from your application is an instance of this (see @ref workflow_processing "workflow")

    As described in @ref workflow_processing "workflow", processing the message is composed of 2 things:
    - @ref workflow_2a "Gathering the message" 
    - @ref workflow_2b "Processing the message"

    The logger class has 2 template parameters:


    @param gather_msg A new gather instance is created each time a message is written. 
    The @c gather_msg class needs to be default-constructible.
    The @c gather_msg must have a function called @c .msg() which contains all information about the written message.
    It will be passed to the write_msg class.
    You can implement your own @c gather_msg class, or use any from the gather namespace.


    @param write_msg This is the object that does the @ref workflow_2b "second step" - the writing of the message.
    It can be a simple functor.
    Or, it can be a more complex object that contains logic of how the message is to be further formatted,
    and written to multiple destinations. 
    You can implement your own @c write_msg class, or it can be any of the classes defined in writer namespace.
    Check out writer::format_write - which allows you to use
    several formatters to further format the message, and then write it to destinations.

    \n\n
    You will seldom need to use the logger class directly. You can use @ref defining_your_logger "other wrapper classes".


    \n\n    
    The logger forwards 
    the gathering of the message to the @c gather_msg class. Once all message is gathered, it's passed on to the writer.
    This is usually done through a @ref macros_use "macro".

    @code
    typedef logger< ... > log_type;
    BOOST_DECLARE_LOG_FILTER(g_log_filter, filter::no_ts ) 
    BOOST_DECLARE_LOG(g_l, log_type) 

    #define L_ BOOST_LOG_USE_LOG_IF_FILTER(g_l, g_log_filter->is_enabled() ) 

    // usage
    L_ << "this is so cool " << i++;

    @endcode



    \n\n        
    To understand more on the workflow that involves %logging:
    - check out the gather namespace
    - check out the writer namespace
    
    */
    template<class gather_msg = default_, class write_msg = default_ > struct logger {
        typedef typename use_default<gather_msg, gather::ostream_like::return_str< std::basic_string<char_type>, std::basic_ostringstream<char_type> > > ::type gather_type;
        typedef write_msg write_type;

        typedef logger<gather_msg, write_msg> self;

        logger() {}
        BOOST_LOGGING_FORWARD_CONSTRUCTOR(logger,m_writer)

        // FIXME watch for copy-construction!
        /** 
            reads all data about a log message (gathers all the data about it)
            FIXME
        */
        gather_holder<self, gather_type> read_msg() const { return gather_holder<self, gather_type>(*this) ; }

        write_msg & writer()                    { return m_writer; }
        const write_msg & writer() const        { return m_writer; }

        // called after all data has been gathered
        void on_do_write(gather_type & gather) const {
            m_writer( detail::as_non_const(gather.msg()) );
        }

    private:
        write_msg m_writer;
    };

    // specialize for write_msg* pointer!
    template<class gather_msg, class write_msg> struct logger<gather_msg, write_msg* > {
        typedef gather_msg gather_type;
        typedef write_msg write_type;

        typedef logger<gather_msg, write_msg*> self;

        logger(write_msg * writer = 0) : m_writer(writer) {}

        void set_writer(write_msg* writer) {
            m_writer = writer;
        }

        // FIXME watch for copy-construction!
        /** 
            reads all data about a log message (gathers all the data about it)
            FIXME
        */
        gather_holder<self, gather_msg> read_msg() const { return gather_holder<self, gather_msg>(*this) ; }

        write_msg & writer()                    { return *m_writer; }
        const write_msg & writer() const        { return *m_writer; }

        // called after all data has been gathered
        void on_do_write(gather_msg & gather) const {
            (*m_writer)( detail::as_non_const(gather.msg()) );
        }

    private:
        write_msg *m_writer;
    };


    // specialize when write_msg is not set - in this case, you need to derive from this
    template<class gather_msg> struct logger<gather_msg, default_ > {
        typedef gather_msg gather_type;
        typedef void_ write_type;

        typedef logger<gather_msg, default_> self;
        typedef typename gather_msg::param param;

        logger() {}
        // we have virtual functions, lets have a virtual destructor as well - many thanks Martin Baeker!
        virtual ~logger() {}

        // FIXME watch for copy-construction!
        /** 
            reads all data about a log message (gathers all the data about it)
            FIXME
        */
        gather_holder<self, gather_msg> read_msg() const { return gather_holder<self, gather_msg>(*this) ; }

        write_type & writer()                    { return m_writer; }
        const write_type & writer() const        { return m_writer; }

        // called after all data has been gathered
        void on_do_write(gather_msg & gather) const {
            do_write( detail::as_non_const(gather.msg()) );
        }

        virtual void do_write(param) const = 0;
    private:
        // we don't know the writer
        void_ m_writer;
    };

    /** 
    
    @param write_msg the write message class. If a pointer, forwards to a pointer. If not a pointer, it holds it by value.
    */
    template<class gather_msg, class write_msg> struct implement_default_logger : logger<gather_msg, default_> {
        typedef typename gather_msg::param param;

        implement_default_logger() {}
        BOOST_LOGGING_FORWARD_CONSTRUCTOR(implement_default_logger,m_writer)

        virtual void do_write(param a) const {
            m_writer(a);
        }

    private:
        write_msg m_writer;
    };

    // specialization for pointers
    template<class gather_msg, class write_msg> struct implement_default_logger<gather_msg,write_msg*> : logger<gather_msg, default_> {
        typedef typename gather_msg::param param;

        implement_default_logger(write_msg * writer = 0) : m_writer(writer) {}

        void set_writer(write_msg* writer) {
            m_writer = writer;
        }

        virtual void do_write(param a) const {
            (*m_writer)(a);
        }

    private:
        write_msg * m_writer;
    };



}}

#endif

