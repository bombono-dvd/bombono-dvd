
//#include <mbase/_pc_.h>
#include "resources.h"

#include <mlib/filesystem.h>

#define DATADIR  "resources"

std::string GetDataDir()
{
#ifdef DATA_PREFIX
    static std::string res_dir;
    if( res_dir.empty() )
        res_dir = (fs::path(DATA_PREFIX) / DATADIR).string();
    return res_dir;
#else
    // работаем локально
    return DATADIR;
#endif
}

