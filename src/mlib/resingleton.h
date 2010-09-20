#ifndef __MLIB_RESINGLETON_H__
#define __MLIB_RESINGLETON_H__

#include "ptr.h"

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
        SingletonStack(Si* si) { Si::InitInstance(si); }
       ~SingletonStack()       { Si::InitInstance();   }
};

#endif // #ifndef __MLIB_RESINGLETON_H__

