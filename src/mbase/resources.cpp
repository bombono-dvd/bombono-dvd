
//#include <mbase/_pc_.h>
#include "resources.h"

#include <mlib/filesystem.h>

const char* GetInstallPrefix()
{
#ifdef INSTALL_PREFIX
    return INSTALL_PREFIX;
#else
    // работаем локально
    return 0;
#endif

}

#define DATADIR  "resources"

const std::string& GetDataDir()
{
    static std::string res_dir;
    if( res_dir.empty() )
    {
        const char* prefix = GetInstallPrefix();
        res_dir = prefix ? (fs::path(prefix) / "share" / "bombono" / DATADIR).string() : DATADIR ;
    }
    return res_dir;
}

