//
// mlib/resingleton.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

#ifndef __MLIB_RESINGLETON_H__
#define __MLIB_RESINGLETON_H__

#include "ptr.h"
#include "tech.h"

// перезаряжаемый Singleton
template<typename Si>
class ReSingleton
{
    typedef ptr::one<Si> SiPtr;
    public:

    static        Si& Instance()
                      {
                          Si* p = Ptr().get();
                          ASSERT( p );
                          return *p;
                      }

    static      void  InitInstance(Si* p = 0)
                      {
                          Ptr().reset(p);
                      }

    private:
    static     SiPtr& Ptr()
                      {
                          static SiPtr ptr;
                          return ptr;
                      }
};

// ограничить время жизни объекта кадром стека
template<typename Si>
class SingletonStack
{
    public:
        SingletonStack(Si* p = 0) 
        {
            if( !p )
                p = new Si();
            Si::InitInstance(p); 
        }
       ~SingletonStack() { Si::InitInstance();   }
};

#endif // #ifndef __MLIB_RESINGLETON_H__

