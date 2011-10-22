//
// mdemux/dvdread.h
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

#ifndef __MDEMUX_DVDREAD_H__
#define __MDEMUX_DVDREAD_H__

// must be before ifo_types.h
#include "intn_max.h" 

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

#include <mlib/ptr.h>
#include <mlib/patterns.h>
#include <mlib/tech.h>
#include <mlib/read_stream.h>
#include <mlib/sdk/misc.h>
#include <mlib/function.h> // DtorAction

#include <vector>

namespace DVD {

struct Reader
{
    dvd_reader_t* dvd;
    //DtorAction  wrp;

        Reader(dvd_reader_t* dvd_): dvd(dvd_) {}
       ~Reader() { DVDClose(dvd); }

    private:
        Reader();
        Reader(const Reader&);
};

typedef ptr::shared<Reader> ReaderPtr;

// (vts, vobid)
struct VobPos: public std::pair<uint8_t, uint16_t>
{
     uint8_t& Vts()   { return first; }
    uint16_t& VobId() { return second; }
};

std::string VobFName(VobPos& pos, const std::string& suffix = std::string());

struct Vob
{
    struct Part
    {
        uint32_t beg;
        uint32_t end;
        uint32_t off; // абсолютная позиция этой части Vob

                  Part(uint32_t b, uint32_t e, uint32_t off_)
                    : beg(b), end(e), off(off_) {}

        uint32_t  End() { return off + (end - beg); }
    };
    typedef std::vector<Part> LocationArr;

        VobPos  pos;      
   LocationArr  locations; // массив данных контента start_sector/end_sector = last_sector+1
         Point  sz;        // разрешение, аспект берется с VTS
  AspectFormat  aspect;
        double  tmLen;     // продолжительность (высчитываем по pgc)

              Vob() : tmLen(0.) {}

    uint32_t  Count() { return locations.back().End(); } // размер в секторах
};

typedef ptr::shared<Vob> VobPtr;

struct VobArr: public std::vector<VobPtr>
{
    bool  isPAL;

    VobArr(): isPAL(true) {}
};

void FillVobArr(VobArr& dvd_vobs, dvd_reader_t* dvd);
VobPtr FindVob(VobArr& dvd_vobs, uint8_t vts, uint16_t vob_id);

bool IsPAL(ifo_handle_t* vmg);

////////////////////////////////////////////////////////////////

class ReadStreambuf: public io::filebuf<char>
{
    public:
                          ReadStreambuf(): pos() {}

       virtual      bool  is_open() const { return true; }
       virtual      bool  close()         { return true; } // чтоб не смущать пользователей

       virtual  pos_type  Size() = 0;

    protected:
    typedef std::streamsize  streamsize;
    typedef   std::ios_base  ios_base;

        pos_type  pos;

      virtual       void  xsgetnImpl(char* s, streamsize real_n) = 0;

      // по своей сути это peek()
      // утверждается, что реализацией только этой функции можно получить работающий
      // потоковый буфер; однако, прога падает в умолчальном uflow (и, кроме underflow(),
      // uflow() должен передвигать позицию курсора, если требуются seekg()/tellg()
      virtual int_type underflow()
      {
          int_type res = traits_type::eof();
          if( pos < Size() )
          {
              char c;
              xsgetnImpl(&c, 1);
              res = (unsigned char)c;
          }
          return res;
      }
    
    // = underflow() + передвинуть курсор на +1; т.е. это реализация get()
    virtual int_type uflow()
    {
        int_type res = underflow();
        seekoff(1, std::ios_base::cur, std::ios_base::in);
        return res;
    }
    
    static const std::streamoff streamsize_limit;
    // по сути это uflow() * n (раз) (умолчальная версия так и делает)
    // используется функцией read() как более эффективная версия чтения из потока
    // нескольких символов
    virtual streamsize xsgetn(char* s, streamsize n)
    {
        std::streamoff tail = Size()-pos;
        ASSERT( tail >= 0 );
        // чтобы не выйти за пределы streamsize(=int), нужна такая проверка
        streamsize max_read = streamsize(std::min(tail, streamsize_limit));
        streamsize real_n   = std::min(max_read, n);

        xsgetnImpl(s, real_n);
        pos += real_n;
        return real_n;
    }

    // вызывается tellg() = seekoff(0, cur) и seekg(pos, seekdir) = seekoff(pos, seekdir)
    virtual pos_type seekoff(off_type n_pos, ios_base::seekdir dir, ios_base::openmode)
    {
        pos_type cnt = Size();
        switch( dir )
        {
        case std::ios_base::beg:
            pos = n_pos;
            break;
        case std::ios_base::cur:
            pos += n_pos;
            break;
        case std::ios_base::end:
            pos = cnt + n_pos;
            break;
        default:
            ASSERT(0);
        }
        pos = std::max(pos_type(0), pos);
        pos = std::min(pos, cnt);

        return pos;
    }
    // вызывается только методом seekg(pos) - с одним аргументом
    virtual pos_type seekpos(pos_type n_pos, ios_base::openmode mode)
    {
        return seekoff(n_pos, std::ios_base::beg, mode);
    }

    //
    // Для записи
    //
    virtual streamsize xsputn(const char_type* /*__s*/, streamsize /*__n*/)
    {
        ASSERT(0);
        return streamsize();
    }
    virtual int_type overflow(int_type)
    {
        ASSERT(0);
        return int_type();
    }
    virtual int_type pbackfail(int_type)
    {
        ASSERT(0);
        return int_type();
    }
    // flush()
    virtual int sync()
    {
        ASSERT(0);
        return 0;
    }

    // Если используется буферизация
    //virtual std::streambuf* setbuf(char_type* , streamsize)
    //{
    //    ASSERT(0);
    //    return 0;
    //}

    // Опционально; редко реализуется <=> редко используется
    //virtual streamsize showmanyc()
    //{
    //    ASSERT(0);
    //    return streamsize();
    //}

};

struct VobFile
{
    dvd_file_t* file;
        VobPtr  vob;

        VobFile(VobPtr vob_, dvd_reader_t* dvd): 
            file(DVDOpenFile(dvd, vob_->pos.Vts(), DVD_READ_TITLE_VOBS)), vob(vob_) {}
       ~VobFile() { DVDCloseFile(file); }

    private:
        VobFile();
        VobFile(const VobFile&);
};

inline int64_t Size(VobFile& vf)
{
    int64_t sz = vf.vob->Count();
    return sz << 11; // *2048 = 2^11 
}

class VobStreambuf: public ReadStreambuf
{
    public:
                      VobStreambuf(VobPtr vob, dvd_reader_t* dvd): 
                          content(vob, dvd) {}

    virtual pos_type  Size() 
    {
        return DVD::Size(content);
    }

    protected:

             VobFile  content;

    virtual     void  xsgetnImpl(char* s, streamsize real_n);
};

inline int CAdtSize(c_adt_t* cptr)
{
    return (cptr->last_byte+1-C_ADT_SIZE)/sizeof(cell_adr_t);
}

void ExtractVob(ReadFunctor& fnr, VobPtr vob, dvd_reader_t* dvd);
void ExtractVob(VobPtr vob, const std::string& dir_path, dvd_reader_t* dvd);
void ReadVob(char* s, int n, VobFile& vf, int64_t cur_pos);

} // namespace DVD

#endif // __MDEMUX_DVDREAD_H__

