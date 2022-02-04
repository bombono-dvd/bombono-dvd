//
// mlib/filesystem.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009-2010 Ilya Murav'jov
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

// :TRICKY: со временем может измениться (в бусте), но создавать
// свою хлопотнее пока, см. boost/detail/utf8_codecvt_facet.hpp
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
// в 1.51 v2 удалена полностью (svn diff -r 77555:HEAD -- boost/version.hpp | less)
#if BOOST_MINOR_VERSION >= 51
#define BOOST_FS_3 boost::filesystem
#include <boost/filesystem/path_traits.hpp> // boost::filesystem::convert()
#include <boost/filesystem/directory.hpp>
#else
#define BOOST_FS_3 boost::filesystem3
#include <boost/filesystem/v3/path_traits.hpp>
#endif 


#include <string.h> // strstr()

namespace boost { namespace filesystem {

bool is_empty_directory(const path & dir_path)
{
    static const directory_iterator end_itr;
    return directory_iterator(dir_path) == end_itr;
}

std::string operator / (const path& f, to_string_enum /*to_str*/)
{
    return f.string();
}

#ifdef BFS_VERSION_3

std::string name_str(const path& pth)
{
    return pth.filename().string();
}

#else

std::string name_str(const path& pth)
{
    return pth.leaf();
}

#endif // BFS_VERSION_3

std::string name_str(const std::string& pth)
{
    return name_str(path(pth));
}

} } // namepspace filesystem, boost

const char* FindExtDot(const char* name)
{
    return strrchr(name, '.');
}

#ifdef BFS_VERSION_3

std::string get_basename(const fs::path& pth)
{
    return pth.stem().string();
}

std::string get_extension(const fs::path& pth)
{
    // path("/foo/bar.txt").extension() => ".txt"
    // path("foo.bar.baz.tar").extension() => ".tar"
    // а нам нужно без точки
    std::string ext = pth.extension().string();
    if( !ext.empty() )
    {
        const char* ext_c = ext.c_str();
        ASSERT_RTL( ext_c[0] == '.' );
        std::string(ext_c + 1).swap(ext);
    }
    return ext;
}

#else

std::string get_basename(const fs::path& pth)
{
    std::string name_s = pth.leaf();
    const char* name = name_s.c_str();

    //if( const char* dot = strstr(name, ".") )
    if( const char* dot = FindExtDot(name) )
        return std::string(name, dot);
    return name_s;
}

std::string get_extension(const fs::path& pth)
{
    std::string name_s = pth.leaf();

    if( const char* dot = FindExtDot(name_s.c_str()) )
        return std::string(dot+1);
    return std::string();
}

#endif // BFS_VERSION_3

class tune_boost_filesystem
{
    public:
    tune_boost_filesystem()
    {
        // B.FS: отказались от проверок в ver>=2
        //// глобальная установка проверки имен файлов
        //// чтоб любые символы в именах файлов позволялись, для utf8
        //fs::path::default_name_check(fs::native);

#if defined(_WIN32) && defined(BFS_VERSION_3)
        // внутри используем utf-8 => меняем конвертор
        std::locale utf8_loc(std::locale(), new fs::detail::utf8_codecvt_facet);
        fs::path::imbue(utf8_loc);
#endif
    }
} tune_boost_filesystem_obj;

std::wstring Utf8ToUcs16(const char* utf8_str)
{
    std::wstring res;
#ifdef _WIN32
    // можно напрямую использовать utf8_codecvt_facet, но так проще
    // (будет работать при fs::path::imbue(utf8_loc);)
    BOOST_FS_3::path_traits::convert(utf8_str, 0, res, fs::path::codecvt());
#else
    ASSERT(0);
#endif
    return res;
}

namespace Project
{

fs::path MakeAbsolutePath(const fs::path& pth, const fs::path& cur_dir)
{
    return absolute(pth, cur_dir);
}

// оба аргумента должны быть абсолютными путями
bool MakeRelativeToDir(fs::path& pth, fs::path dir)
{
    pth = canonical(pth);
    dir = canonical(dir);
    ASSERT( pth.is_absolute() );
    ASSERT( dir.is_absolute() );

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

