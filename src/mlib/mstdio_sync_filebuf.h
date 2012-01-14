// Iostreams wrapper for stdio FILE* -*- C++ -*-

// Copyright (C) 2003, 2004 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

// 
// :COPY_N_PASTE: c libstdc++v3 ext/stdio_sync_filebuf.h
//
// mstdio_sync_filebuf.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007,2009-2010 Ilya Murav'jov
// 
// Используется в проекте "Атом" как безопасная (в плане буферизации)
// обертка над FILE* и int (файловый дескриптор)
// 
// Да, этот вариант не самый быстрый,  и применяется в libstdc++v3 только
// для стандартных потоков cin, cout, cerr, но если использовать "большие"
// чтение/запись, т.е. get(buf, not_small_size) = fread(buf, not_small_size),
// то падение производительности не будет.
// 

#ifndef __MLIB_MSTDIO_SYNC_FILEBUF_H__
#define __MLIB_MSTDIO_SYNC_FILEBUF_H__

//#pragma GCC system_header

#include <streambuf>
#include <unistd.h>
#include <cstdio>

//#ifdef _GLIBCXX_USE_WCHAR_T
//#include <cwchar>
//#endif

namespace msys
{

template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
class filebuf : public std::basic_streambuf<_CharT, _Traits>
{
    public:
        typedef FILE FILE_type;

   virtual      bool  is_open() const = 0;
   virtual FILE_type* file() { return 0; }

   virtual      void  set_close(bool) {}
   virtual      bool  get_close()     { return true; }

   virtual FILE_type* swap_cstreams(FILE_type* p_file) { return p_file; }

   virtual      bool  open(FILE_type*)                           { return false; }
   virtual      bool  open(const char*, std::ios_base::openmode) { return false; }
   virtual      bool  open(int, std::ios_base::openmode)         { return false; }
   virtual      bool  close()                                    { return false; }
};

template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
class stdio_sync_filebuf : public filebuf<_CharT, _Traits>
{
    public:
    // Types:
    typedef _CharT                    char_type;
    typedef _Traits                   traits_type;
    typedef typename traits_type::int_type        int_type;
    typedef typename traits_type::pos_type        pos_type;
    typedef typename traits_type::off_type        off_type;

    typedef filebuf<_CharT, _Traits>  filebuf_type;
    typedef typename filebuf_type::FILE_type FILE_type;

    private:
    // Underlying stdio FILE
    FILE_type* _M_file;

    // Last character gotten. This is used when pbackfail is
    // called from basic_streambuf::sungetc()
    int_type _M_unget_buf;

    protected:
    int_type
    syncgetc();

    int_type
    syncungetc(int_type __c);

    int_type
    syncputc(int_type __c);

    virtual int_type
    underflow()
    {
        int_type __c = this->syncgetc();
        return this->syncungetc(__c);
    }

    virtual int_type
    uflow()
    {
        // Store the gotten character in case we need to unget it.
        _M_unget_buf = this->syncgetc();
        return _M_unget_buf;
    }

    virtual int_type
    pbackfail(int_type __c = traits_type::eof())
    {
        int_type __ret;
        const int_type __eof = traits_type::eof();

        // Check if the unget or putback was requested
        if(traits_type::eq_int_type(__c, __eof)) // unget
        {
            if(!traits_type::eq_int_type(_M_unget_buf, __eof))
                __ret = this->syncungetc(_M_unget_buf);
            else // buffer invalid, fail.
                __ret = __eof;
        }
        else // putback
            __ret = this->syncungetc(__c);

        // The buffered character is no longer valid, discard it.
        _M_unget_buf = __eof;
        return __ret;
    }

    virtual std::streamsize
    xsgetn(char_type* __s, std::streamsize __n);

    virtual int_type
    overflow(int_type __c = traits_type::eof())
    {
        int_type __ret;
        if(traits_type::eq_int_type(__c, traits_type::eof()))
        {
            if(std::fflush(_M_file))
                __ret = traits_type::eof();
            else
                __ret = traits_type::not_eof(__c);
        }
        else
            __ret = this->syncputc(__c);
        return __ret;
    }

    virtual std::streamsize
    xsputn(const char_type* __s, std::streamsize __n);

    virtual int
    sync()
    { return std::fflush(_M_file);}

    virtual std::streampos
    seekoff(std::streamoff __off, std::ios_base::seekdir __dir,
            std::ios_base::openmode = std::ios_base::in | std::ios_base::out)
    {
        std::streampos __ret(std::streamoff(-1));
        int __whence;
        if(__dir == std::ios_base::beg)
            __whence = SEEK_SET;
        else if(__dir == std::ios_base::cur)
            __whence = SEEK_CUR;
        else
            __whence = SEEK_END;
#ifdef _GLIBCXX_USE_LFS
        if(!fseeko64(_M_file, __off, __whence))
            __ret = std::streampos(ftello64(_M_file));
#else
        if(!fseek(_M_file, __off, __whence))
            __ret = std::streampos(std::ftell(_M_file));
#endif
        return __ret;
    }

    virtual std::streampos
    seekpos(std::streampos __pos,
            std::ios_base::openmode __mode =
            std::ios_base::in | std::ios_base::out)
    { return seekoff(std::streamoff(__pos), std::ios_base::beg, __mode);}

    /////////////////////////////////////////
    // Дополнительные методы
    
    public:
    typedef stdio_sync_filebuf<char_type, traits_type> streambuf_type;

          explicit  stdio_sync_filebuf()
                    : _M_file(NULL), _M_unget_buf(traits_type::eof()), is_to_close(true)
                    {}
                   ~stdio_sync_filebuf()
                    {
                        if( is_to_close ) close();
                    }

              bool  is_open() const
                    { return _M_file != NULL; }
         FILE_type* file() { return this->_M_file; }

                    //
              void  set_close(bool close) { is_to_close = close; }
              bool  get_close() { return is_to_close; }

                    // Основное применение - FILE* pfile = strm.swap_cstreams(NULL);
                    // для освобождения хэндла потока от нашего stream
         FILE_type* swap_cstreams(FILE_type* p_file) 
                    {
                        FILE_type* tmp = _M_file;
                        _M_file = p_file;
                        return tmp;
                    }

               bool open(FILE_type* p_file)
                    {
                        bool res = false;
                        if( !is_open() )
                        {
                            _M_file = p_file;
                            res = true;
                        }
                        return res;
                    }

              bool  open(const char* path_str, std::ios_base::openmode mode);
              bool  open(int fd, std::ios_base::openmode mode);
        
              bool  close();

    private:

    bool is_to_close;  // закрывать ли поток при закрытии

};

template<>
bool
stdio_sync_filebuf<char>::close();

template<>
inline stdio_sync_filebuf<char>::int_type
stdio_sync_filebuf<char>::syncgetc()
{ return std::getc(_M_file);}

template<>
inline stdio_sync_filebuf<char>::int_type
stdio_sync_filebuf<char>::syncungetc(int_type __c)
{ return std::ungetc(__c, _M_file);}

template<>
inline stdio_sync_filebuf<char>::int_type
stdio_sync_filebuf<char>::syncputc(int_type __c)
{ return std::putc(__c, _M_file);}

template<>
inline std::streamsize
stdio_sync_filebuf<char>::xsgetn(char* __s, std::streamsize __n)
{
    std::streamsize __ret = std::fread(__s, 1, __n, _M_file);
    if(__ret > 0)
        _M_unget_buf = traits_type::to_int_type(__s[__ret - 1]);
    else
        _M_unget_buf = traits_type::eof();
    return __ret;
}

template<>
inline std::streamsize
stdio_sync_filebuf<char>::xsputn(const char* __s, std::streamsize __n)
{ return std::fwrite(__s, 1, __n, _M_file);}

} // namespace msys


#endif // __MLIB_MSTDIO_SYNC_FILEBUF_H__
