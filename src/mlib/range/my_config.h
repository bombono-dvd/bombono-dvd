#ifndef __MLIB_RANGE_MY_CONFIG_H__
#define __MLIB_RANGE_MY_CONFIG_H__

// отключаем ради старых версий буста
#define BOOST_RANGE_ENABLE_CONCEPT_ASSERT 0

#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
    #define BOOST_RANGE_CONCEPT_ASSERT( x ) BOOST_CONCEPT_ASSERT( x )
#else
    #define BOOST_RANGE_CONCEPT_ASSERT( x )
#endif

#endif // #ifndef __MLIB_RANGE_MY_CONFIG_H__

