//
// mlib/filesystem.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009 Ilya Murav'jov
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

#include "filesystem.h"
#include "tech.h"
#include "format.h"

#include <string.h> // strstr()

namespace boost { namespace filesystem {

bool is_empty_directory(const path & dir_path)
{
    static const directory_iterator end_itr;
    return directory_iterator(dir_path) == end_itr;
}

} } // namepspace filesystem, boost

std::string get_basename(const fs::path& pth)
{
    std::string name_s = pth.leaf();
    const char* name = name_s.c_str();

    if( const char* dot = strstr(name, ".") )
        return std::string(name, dot);
    return name_s;
}

std::string get_extension(const fs::path& pth)
{
    std::string name_s = pth.leaf();
    const char* name = name_s.c_str();

    if( const char* dot = strrchr(name, '.') )
        return std::string(dot+1);
    return std::string();
}

// глобальная установка проверки имен файлов
class tune_boost_filesystem
{
    public:
    tune_boost_filesystem()
    {
        // чтоб любые символы в именах файлов позволялись, для utf8
        fs::path::default_name_check(fs::native);
    }
} tune_boost_filesystem_obj;

namespace Project
{

fs::path MakeAbsolutePath(const fs::path& pth, const fs::path& cur_dir)
{
    fs::path res;

    if( pth.is_complete() )
        res = pth;
    else
    {
        fs::path dir = cur_dir.empty() ? fs::current_path() : cur_dir ;
        res = dir/pth;
    }
    return res.normalize();
}

// оба аргумента должны быть абсолютными путями
bool MakeRelativeToDir(fs::path& pth, fs::path dir)
{
    pth.normalize();
    dir.normalize();
    ASSERT( pth.is_complete() );
    ASSERT( dir.is_complete() );

    fs::path::iterator p_itr = pth.begin(), p_end = pth.end();
    fs::path::iterator d_itr = dir.begin(), d_end = dir.end();
    // только под Win
    if( *p_itr != *d_itr )
        return false;

    for( ; (p_itr != p_end) && (d_itr != d_end) && (*p_itr == *d_itr); 
         ++p_itr, ++d_itr )
        ;

    fs::path res;
    for( ; d_itr != d_end; ++d_itr )
        res /= "..";
    for( ; p_itr != p_end; ++p_itr )
        res /= *p_itr;

    pth = res;
    return true;
}

bool HaveFullAccess(const fs::path& path)
{
    ASSERT( fs::exists(path) );
    return 0 == access(path.string().c_str(), R_OK|W_OK|X_OK);
}

bool ClearAllFiles(const fs::path& dir_path, std::string& err_str)
{
    bool res = true;
    try
    {
        static const fs::directory_iterator end_itr;
        for( fs::directory_iterator itr(dir_path);
            itr != end_itr; ++itr )
            fs::remove_all(*itr);
    }
    catch( const fs::filesystem_error& fe )
    {
        err_str = FormatFSError(fe);
        res = false;
    }
    return res;
}

} // namespace Project

std::string AppendPath(const std::string& dir, const std::string& path)
{
    return (fs::path(dir)/path).string();
}

std::string FormatFSError(const fs::filesystem_error& fe)
{
    std::string what  = fe.code().message();
    // бывает и второй путь, path2(), но пока не требуется
    std::string fpath = fe.path1().string();
    if( !fpath.empty() )
        what = boost::format("%1%: \"%2%\"") % what % fpath % bf::stop;
    return what;
}

