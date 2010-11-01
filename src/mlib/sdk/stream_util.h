//
// mlib/sdk/stream_util.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2010 Ilya Murav'jov
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

#ifndef __MLIB_SDK_STREAM_UTIL_H__
#define __MLIB_SDK_STREAM_UTIL_H__

#include <mlib/stream.h>
#include <mlib/geom2d.h>

void CleanEof(io::stream& strm);

class StreamPosSaver
{
    public:
            StreamPosSaver(io::stream& strm): stream(strm), origPos(strm.tellg()) { }
           ~StreamPosSaver();
    private:
        io::stream& stream;
           io::pos  origPos;
};

inline io::pos StreamSize(io::stream& strm)
{
    StreamPosSaver sav(strm);

    strm.seekg(0, iof::end);
    return strm.tellg();
}

// можно ли открыть файл в режиме attr
bool CanOpen(const char* fname, iof::openmode attr = iof::def);

//////////////////////////////////////////////////////////////////////////////

// Вывод в поток (обычно в тестовых целях) различных структур
template<typename CharT, typename Traits, typename T>
inline std::basic_ostream<CharT, Traits>&
operator << (std::basic_ostream<CharT, Traits>& os, const PointT<T>& pnt)
{
    os << "(" << pnt.x << ", " << pnt.y << ")";
    return os;
}

template<typename CharT, typename Traits, typename T>
inline std::basic_ostream<CharT, Traits>&
operator << (std::basic_ostream<CharT, Traits>& os, const RectT<T>& rct)
{
    os << "(" << rct.lft << ", " << rct.top << ", " << rct.rgt << ", " << rct.btm << ")";
    return os;
}

//////////////////////////////////////////////////////////////////////////////
namespace msys
{

inline off_t tell(int fd) { return lseek(fd, 0, SEEK_CUR); }

} //namespace msys

// открыть файл: только для чтения (is_read) или только для записи
int OpenFileAsArg(const char* fpath, bool is_read);
// вызов write(int) не такой простой как кажется (см. "Advanced Unix Programming"),
// особенно при запись в канал (возможно прерывание EINTR); данная функция призвана
// надежно обернуть write() в этом плане и вернет false в случае действительной ошибки
// Замечание: только для блокируемых fd, для неблокируемых может быть еще EAGAIN
bool writeall(int fd, const void* buf, size_t nbyte);
// writeall() + ASSERT_RTL()
void checked_writeall(int fd, const void* buf, size_t nbyte);

#endif // #ifndef __MLIB_SDK_STREAM_UTIL_H__

