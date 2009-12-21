//
// mlib/stream.h
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

#ifndef __MLIB_STREAM_H__
#define __MLIB_STREAM_H__

#include <istream>

#include "mstdio_sync_filebuf.h"
#include "ptr.h"

// флаги и перечисляемые функций io для удобства переместим в iof
namespace iof
{
    // нельзя, это класс :(
//    using std::ios;

    // флаги открытия файлов
    typedef std::ios::openmode openmode;

    const openmode app    =	std::ios::app;
    const openmode ate    =	std::ios::ate;
    const openmode binary = std::ios::binary;
    const openmode in     = std::ios::in;
    const openmode out    = std::ios::out;
    const openmode trunc  =	std::ios::trunc;

    const openmode in_out  = in|out;
    const openmode def     = in_out|binary; // по умолчанию

    // типы и константы для перемещения по файлу
    typedef std::ios::seekdir seekdir;

    const seekdir beg = std::ios::beg;
    const seekdir cur = std::ios::cur;
    const seekdir end = std::ios::end;

}

namespace msys {

// нужно, в том числе, и для использования констант вида msys::hex (io::hex)
using namespace std;

template<typename _CharT, typename _Traits>
class basic_fstream : public basic_iostream<_CharT, _Traits>
{
    public:
        // Types:
        typedef _CharT                    char_type;
        typedef _Traits                   traits_type;
        typedef typename traits_type::int_type        int_type;
        typedef typename traits_type::pos_type        pos_type;
        typedef typename traits_type::off_type        off_type;
    
        // Non-standard types:
        typedef filebuf<char_type, traits_type>       filebuf_type;
        typedef basic_ios<char_type, traits_type>     __ios_type;
        typedef basic_iostream<char_type, traits_type>    __iostream_type;

        typedef basic_fstream<char_type, traits_type>    basic_fstream_type;
    
    public:
                
                        // Конструкторы
                        // по умолчанию
                        basic_fstream( bool is_to_close = true ) : __iostream_type(NULL)
                        { 
                            init_defbuf();
                            _M_filebuf->set_close(is_to_close);
                        }
                        // по названию файла
              explicit  basic_fstream( const char* __s,
                                       ios_base::openmode mode = iof::def,
                                       bool is_to_close = true )
                        : __iostream_type(NULL), _M_filebuf()
                        {
                            init_defbuf();
                            this->open(__s, mode, is_to_close);
                        }
                        // по С-указателю на файл
              explicit  basic_fstream( FILE* c_stream,
                                       bool is_to_close = true ) : __iostream_type(NULL)
                        {
                            init_defbuf();
                            this->open(c_stream, is_to_close);
                        }
                        // по файловому дескриптору
              explicit  basic_fstream( int fd, 
                                       ios_base::openmode mode = iof::def,
                                       bool is_to_close = true ): __iostream_type()
                        {
                            init_defbuf();
                            this->open(fd, mode, is_to_close);
                        }
        
                       ~basic_fstream()
                        {}
            
          filebuf_type* rdbuf()
                        { return _M_filebuf.get();}
                    
                  bool  is_open()
                        { return _M_filebuf && _M_filebuf->is_open(); }
                  bool  is_open() const
                        { return _M_filebuf && _M_filebuf->is_open(); }
                    
                  void  open( const char* path_str,
                              ios_base::openmode mode = iof::def,
                              bool is_to_close = true )
                        {
                            if( !_M_filebuf->open(path_str, mode) )
                                this->setstate(ios_base::failbit);
                            else
                            {
                                _M_filebuf->set_close(is_to_close);
                                this->clear();
                            }
                        }
        
                  void  open( FILE* c_stream,
                              bool is_to_close = true )
                        {
                            if( !_M_filebuf->open(c_stream) )
                                this->setstate(ios_base::failbit);
                            else
                            {
                                _M_filebuf->set_close(is_to_close);
                                this->clear();
                            }
                        }
        
                  void  open( int fd,
                              ios_base::openmode mode = iof::def,
                              bool is_to_close = true )
                        {
                            if( !_M_filebuf->open(fd, mode) )
                                this->setstate(ios_base::failbit);
                            else
                            {
                                _M_filebuf->set_close(is_to_close);
                                this->clear();
                            }
                        }
                    
                  void  close()
                        {
                            if( !_M_filebuf->close() )
                                this->setstate(ios_base::failbit);
                        }


                        // С-указатель на этот файл
                  FILE* file()
                        { return _M_filebuf->file(); }

                        class opaque_file
                        {
                            public:
                                opaque_file(FILE* pf): p_file(pf) {}
                            protected:
                                FILE* p_file;
                                opaque_file();
                        };
                        // Также можно получить дескриптор файла:
                        //   io::fd_proxy fd = strm.file();
                        //   int real_fd = fd;
                        //   write(fd, "some text", 9);
                        //   ..
                        // Причины использования класса io::fd_proxy см. в его описании
           opaque_file  fileno() 
                        { return opaque_file(this->file()); }

                        // более короткий вариант для чтения в буфер (аналогичен 
                        // классическому варианту fread()), чем
                        // 
                        // for( ; strm.read(buf, cnt), strm.gcount() ; ) // "strm" нельзя вместо "strm.gcount()" ! 
                        // { ... }
                        // 
                        // причина в том, что stream::read(), если прочитал меньше чем cnt, ставит failbit;
                        // потому удобней иметь интерфейс(эту функцию), читающую "сколько возможно", при
                        // этом failbit ставится только если 0 байтов прочитали
            streamsize  raw_read(char_type* buf, streamsize cnt)
                        {
                            streamsize size = 0;
                            if( this->good() ) // в libstdc++ это делает sentry
                            {
                                if( this->tie() ) this->tie()->flush();
                                ios_base::iostate stt = ios_base::iostate(ios_base::goodbit);
                                try
                                {
                                    typedef std::basic_ios<char_type, traits_type> basic_ios;
                                    size = this->basic_ios::rdbuf()->sgetn(buf, cnt);
                                    if( !size && cnt )
                                        this->setstate( stt | ios_base::eofbit | ios_base::failbit );
                                }
                                catch(...)
                                {
                                    this->setstate(ios_base::badbit);
                                }
                            }
                            return size;
                        }
                        // = write(), просто для пары, к raw_write()
    basic_fstream_type& raw_write(const char_type* buf, streamsize cnt)
                        {
                            this->write(buf, cnt);
                            return const_cast<basic_fstream_type&>(*this);
                        }


                        // при закрытии потока закроем его C-поток
                  void  set_close(bool close) { _M_filebuf->set_close(close); }
                  bool  get_close() { return _M_filebuf->get_close(); }

                        // Основное применение - FILE* pfile = strm.swap_cstreams(NULL);
                        // для освобождения хэндла потока от нашего stream
                  FILE* swap_cstreams(FILE* p_file)
                        {
                            return _M_filebuf->swap_cstreams(p_file);
                        }

                        // установка специфичного потокового драйвера
                  void  init_buf(ptr::shared<filebuf_type> buf)
                        {
                            _M_filebuf = buf;
                            this->init(_M_filebuf.get());
                        }
    private:

        ptr::shared<filebuf_type> _M_filebuf;

                  void  init_defbuf()
                        {
                            _M_filebuf.reset(new stdio_sync_filebuf<char_type, traits_type>());
                            this->init(_M_filebuf.get());
                        }
};

////////////////////////////////////////////////////
// Весь пользовательский код использует имена io::stream и подобные

// для перемещения по файлу
// std::streampos слишком "жирный",- хранит еще и некий атрибут state
typedef std::streamoff  pos;

//using namespace std;
template<typename _CharT, typename _Traits = char_traits<_CharT> >
    class basic_fstream;

typedef basic_fstream<char> stream;
typedef filebuf<char> fbuf;

// сами обертки создаем для стандартных потоков
extern stream cin;
extern stream cout;
extern stream cerr;

// согласно стандарту POSIX при одновременном использовании int- и FILE*-хэндлов
// одного и того же файла требуется "синхронизация", 
// см. POSIX, Interaction of File Descriptors and Standard I/O Streams
class fd_proxy: public stream::opaque_file
{
    typedef stream::opaque_file my_parent;
    typedef const my_parent& construct_type;
    public:

            fd_proxy(construct_type);
           ~fd_proxy();

            operator int(){ return fd; }

    private:
//                       FILE* p_file;
                       int  fd;

            fd_proxy();
};

} // namespace msys

namespace io = msys;

#endif // #ifndef __MSTREAM_H__

