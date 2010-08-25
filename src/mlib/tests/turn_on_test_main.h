#ifndef __MLIB_TESTS_TEST_TOTM_H__
#define __MLIB_TESTS_TEST_TOTM_H__

#include <boost/version.hpp>

//
// for Boost > 1.33 dynamic version begin to be built
// 
#if BOOST_VERSION / 100 % 1000 > 33
#   ifndef STILL_HAVE_STATIC_BOOST_WITH_MAIN
#       define BOOST_TEST_DYN_LINK 
#   endif
#endif

#define BOOST_AUTO_TEST_MAIN


#endif // #ifndef __MLIB_TESTS_TEST_TOTM_H__

