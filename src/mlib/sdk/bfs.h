#ifndef __MLIB_SDK_BFS_H__
#define __MLIB_SDK_BFS_H__

#define BOOST_FILESYSTEM_VERSION 3
// избавляемся от leaf() etc
// :TODO: не хотим пока без normalize() 
//#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp> // fs::create_directories()

#if BOOST_FILESYSTEM_VERSION == 3
#define BFS_VERSION_3
#endif

#endif // __MLIB_SDK_BFS_H__
