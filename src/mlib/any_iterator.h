#ifndef __MLIB_ANY_ITERATOR_H__
#define __MLIB_ANY_ITERATOR_H__

// any_iterator: Thomas Becker vs. Adobe implementation
// see results of compilation complexity, profile_any_iterator.cpp
#define ADOBE_AIT 

#ifdef ADOBE_AIT
#   include <mlib/adobe/any_iterator.hpp>
#else
#   include <mlib/any_iterator/any_iterator.hpp>
#endif

#endif // #ifndef __MLIB_ANY_ITERATOR_H__

