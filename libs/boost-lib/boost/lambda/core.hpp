// -- core.hpp -- Boost Lambda Library -------------------------------------
//
// Copyright (C) 2000 Gary Powell (powellg@amazon.com)
// Copyright (C) 1999, 2000 Jaakko J�rvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// 
// Includes the core of LL, without any real features for client:
// 
// tuples, lambda functors, return type deduction templates,
// argument substitution mechanism (select functions)
// 
// Some functionality comes as well:
// Assignment and subscript operators, as well as function
// call operator for placeholder variables.
// -------------------------------------------------------------------------

#ifndef BOOST_LAMBDA_CORE_HPP
#define BOOST_LAMBDA_CORE_HPP

#include "boost/type_traits/transform_traits.hpp"
#include "boost/type_traits/cv_traits.hpp"

#include "boost/tuple/tuple.hpp"

// inject some of the tuple names into lambda 
namespace boost {
namespace lambda {

using ::boost::tuples::tuple;
using ::boost::tuples::null_type;

} // lambda
} // boost

#include "boost/lambda/detail/lambda_config.hpp"
#include "boost/lambda/detail/lambda_fwd.hpp"

#include "boost/lambda/detail/arity_code.hpp"
#include "boost/lambda/detail/actions.hpp"

#include "boost/lambda/detail/lambda_traits.hpp"

#include "boost/lambda/detail/function_adaptors.hpp"
#include "boost/lambda/detail/return_type_traits.hpp"

#include "boost/lambda/detail/select_functions.hpp"

#include "boost/lambda/detail/lambda_functor_base.hpp"

#include "boost/lambda/detail/lambda_functors.hpp"

#include "boost/lambda/detail/ret.hpp"

namespace boost {
namespace lambda {

namespace {

  // Muravj'ov - have to declare vars as static to compatilbe with gcc' precompiled
  // headers
  // These are constants types and need to be initialised
  static boost::lambda::placeholder1_type free1 = boost::lambda::placeholder1_type();
  static boost::lambda::placeholder2_type free2 = boost::lambda::placeholder2_type();
  static boost::lambda::placeholder3_type free3 = boost::lambda::placeholder3_type();

  static boost::lambda::placeholder1_type& _1 = free1;
  static boost::lambda::placeholder2_type& _2 = free2;
  static boost::lambda::placeholder3_type& _3 = free3;
  // _1, _2, ... naming scheme by Peter Dimov
} // unnamed
   
} // lambda
} // boost
   
   
#endif //BOOST_LAMBDA_CORE_HPP