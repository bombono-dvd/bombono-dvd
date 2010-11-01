//
// mlib/sdk/stream_utils.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007, 2010 Ilya Murav'jov
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

#include "stream_util.h"

#include <mlib/const.h>
#include <mlib/tech.h>

#include <sys/stat.h> // S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH
#include <fcntl.h>
#include <string.h> // strcmp()
#include <errno.h>

void CleanEof(io::stream& strm)
{
    if( strm.eof() )
        strm.clear();
}

StreamPosSaver::~StreamPosSaver()
{
    CleanEof(stream);
    stream.seekg(origPos); 
}

bool CanOpen(const char* fname, iof::openmode attr)
{
    io::stream strm(fname, attr);
    return strm.is_open();
}

// открыть файл: только для чтения (is_read) или только для записи
int OpenFileAsArg(const char* fpath, bool is_read)
{
    int fd =   is_read ? IN_HNDL  : OUT_HNDL ;
    int opts = is_read ? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC ;

    if( strcmp( fpath, "-" ) != 0 )
    {
        // обязательно выставляем режим mode с S_IRUSR|S_IWUSR,
        // иначе у нас не будет прав на изменение/удаление этого
        // же файла во второй раз (при перезаписи)!
        mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
        fd = open(fpath, opts, mode);
    }

    if( fd == NO_HNDL )
        Error("Cant open one of files!");

    return fd;
}

bool writeall(int fd, const void* buf, size_t nbyte)
{
    bool res = true;

    ssize_t cnt = 0;
    for( ssize_t n; cnt < (ssize_t)nbyte; )
    {
        n = write(fd, (const char*)buf+cnt, nbyte-cnt);
        if( n == -1 )
        {
            if( errno != EINTR )
            {
                // все плохо, и даже уже записанное не может
                // нас интересовать
                res = false;
                break;
            }
        }
        else
            cnt += n;
    }

    // не было ошибок и записали сколько требовалось
    return res && (cnt == (ssize_t)nbyte);
}

// writeall() + ASSERT_RTL()
void checked_writeall(int fd, const void* buf, size_t nbyte)
{
    bool res = writeall(fd, buf, nbyte);
    ASSERT_RTL( res );
}

