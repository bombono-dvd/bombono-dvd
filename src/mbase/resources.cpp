
//#include <mbase/_pc_.h>
#include "resources.h"

#include <mlib/filesystem.h>
#include <mlib/gettext.h>
#include <mlib/sdk/logger.h>

#include <glibmm/miscutils.h> // get_user_config_dir()

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

bool CreateDirsQuiet(const fs::path& dir)
{
    bool res = true;
    std::string err_str;
    if( !CreateDirs(dir, err_str) )
    {
        res = false;
        LOG_ERR << "CreateDirsQuiet(): " << err_str << io::endl;
    }
    return res;
}

const std::string& GetConfigDir()
{
    static std::string cfg_dir;
    if( cfg_dir.empty() )
    {
        fs::path dir = fs::path(Glib::get_user_config_dir()) / "bombono-dvd";
        CreateDirsQuiet(dir);

        cfg_dir = dir.string();
    }
    return cfg_dir;
}
