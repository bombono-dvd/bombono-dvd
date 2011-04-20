//
// mlib/stream.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#include "stream.h"
#include "sdk/stream_util.h" // msys::tell
#include "tech.h"
#include "string.h" // Utf8ToUcs16()

#include <stdio.h>
#include <errno.h>

namespace msys {

//
stream cin(stdin, false);
stream cout(stdout, false);
stream cerr(stderr, false);

//
//template class stdio_sync_filebuf<char>;


namespace impl {

// согласно стандарту С++ cin привязывается к cout в самом начале работы
class tie_cin_cout
{
    public:
    tie_cin_cout()
    {
        cin.tie(&cout);
    }
} tie_cin_cout_obj;

// :COPY_N_PASTE: from libstdc++v3

static const char* fopen_mode(std::ios_base::openmode mode)
{
    enum 
    {
        in     = std::ios_base::in,
        out    = std::ios_base::out,
        trunc  = std::ios_base::trunc,
        app    = std::ios_base::app,
        binary = std::ios_base::binary
    };

    switch(mode & (in|out|trunc|app|binary))
    {
    case (   out                 ): return "w";  
    case (   out      |app       ): return "a";  
    case (   out|trunc           ): return "w";  
    case (in                     ): return "r";  
    case (in|out                 ): return "r+"; 
    case (in|out|trunc           ): return "w+"; 

    case (   out          |binary): return "wb"; 
    case (   out      |app|binary): return "ab"; 
    case (   out|trunc    |binary): return "wb"; 
    case (in              |binary): return "rb"; 
    case (in|out          |binary): return "r+b";
    case (in|out|trunc    |binary): return "w+b";

    default: return 0; // invalid
    }
}

} // impl

template<>
bool stdio_sync_filebuf<char>::open(const char* path_str, std::ios_base::openmode mode)
{
    bool res = false;
    const char* c_mode = impl::fopen_mode(mode);
    if( c_mode && !is_open() )
    {
#ifdef _WIN32
        FILE_type* pfile = _wfopen(Utf8ToUcs16(path_str).c_str(), Utf8ToUcs16(c_mode).c_str());
#else
        FILE_type* pfile = fopen(path_str, c_mode);
#endif
        if( pfile )
        {
            _M_file = pfile;
            res = true;
        }
    }
    return res;
}

template<>
bool stdio_sync_filebuf<char>::open(int fd, std::ios_base::openmode mode)
{
    bool res = false;
    const char* c_mode = impl::fopen_mode(mode);
    if( c_mode && !is_open() )
    {
        FILE_type* pfile = fdopen(fd, c_mode);
        if( pfile )
        {
            _M_file = pfile;
            res = true;
        }
    }
    return res;
}

template<>
bool stdio_sync_filebuf<char>::close()
{
    bool res = false;
    if( is_open() )
    {
        int err = 0;
        // In general, no need to zero errno in advance if checking
        // for error first. However, C89/C99 (at variance with IEEE
        // 1003.1, f.i.) do not mandate that fclose must set errno
        // upon error.
        errno = 0;
        do
            err = fclose(_M_file);
        while (err && errno == EINTR);

        _M_file = 0;
        if (!err)
            res = true;
    }
    return res;
}

// :COPY_N_PASTE_END:

static int get_sync_fd(FILE* p_file)
{
    int fd = -1;
    if( p_file )
    {
        ::fflush(p_file);
        fd = ::fileno(p_file);
        //assert(fd != -1);
    }
    return fd;
}

fd_proxy::fd_proxy(construct_type of): my_parent(of), fd(-1) 
{
    fd = get_sync_fd(p_file);
}

static void sync_pfile(FILE* p_file, int fd)
{
    if( p_file )
    {
        long new_pos = tell(fd);
        if( new_pos != -1 )
            fseek(p_file, new_pos, SEEK_SET);

        new_pos = ftell(p_file);
    }
}

fd_proxy::~fd_proxy()
{
    sync_pfile(p_file, fd);
}

} // msys
