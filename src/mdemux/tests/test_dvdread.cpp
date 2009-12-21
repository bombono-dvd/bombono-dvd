//
// mdemux/tests/test_dvdread.cpp
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

#include <mdemux/tests/_pc_.h>

#include <mdemux/dvdread.h>
#include <mdemux/util.h>
#include <mdemux/player.h>

#include <mlib/lambda.h>
#include <mlib/filesystem.h>


namespace DVD {

static void PrintCell(cell_adr_t& cell)
{
    io::cout << cell.vob_id << " " << int(cell.cell_id) << ":\t" 
	     << cell.start_sector << ", " << cell.last_sector << io::endl;
}

static bool LessCell(cell_adr_t* c1, cell_adr_t* c2)
{
    return c1->start_sector < c2->start_sector; 
}

void PrintSortedCAdt(ifo_handle_t* ifo)
{
    typedef std::vector<cell_adr_t*> cell_arr_t;
    cell_arr_t cell_arr;

    c_adt_t* cptr = ifo->vts_c_adt;
    for ( int i=0, cnt=CAdtSize(cptr); i<cnt; i++ )
    {
        cell_adr_t& cell = cptr->cell_adr_table[i];
        //PrintCell(cell);
        cell_arr.push_back(&cell);
    }

    std::stable_sort(cell_arr.begin(), cell_arr.end(), LessCell);
    for ( cell_arr_t::iterator itr = cell_arr.begin(), beg = cell_arr.begin(), end = cell_arr.end(); itr != end; ++itr )
    {
        cell_adr_t& cell = **itr;
        if ( itr != beg )
        {
            cell_adr_t& prev_cell = **(itr-1);
            int diff = cell.start_sector - prev_cell.last_sector;
            if ( diff != 1 )
            {
                io::cout << (diff <= 0 ? "***" : "<<<") << " \n";
            }
        }
        PrintCell(cell);
    }
}

void PrintVobTime(int vob_id, double tm)
{
    io::cout << "vob " << vob_id << " length(sec): " << tm << io::endl;
}

BOOST_AUTO_TEST_CASE( TestDVDRead )
{
    return;
    //const char* dvd_path = "/home/ilya/opt/programming/atom-project/t/Disc1";
    //const char* dvd_path = "/media/Portable80/Video/TheMatrix/dvd"; //"/home/ilya/opt/programming/atom-project/t/TheMatrix";
    const char* dvd_path = "/mnt/ntfs/DVD_Demystified";
    //const char* dvd_path = "/dev/dvd";

    dvd_reader_t* dvd = DVDOpen(dvd_path);
    BOOST_CHECK(dvd);
    Reader reader(dvd);

    VobArr dvd_vobs; // массив всех VOB
    FillVobArr(dvd_vobs, dvd);

    io::cout << "VOB count: " << dvd_vobs.size() << io::endl;
    for( VobArr::iterator itr = dvd_vobs.begin(), end = dvd_vobs.end(); itr != end; ++itr )
    {
        Vob& vob = **itr;
        using Mpeg::set_hms;
        io::cout << "vts, vobid = " << set_hms() << int(vob.pos.Vts()) << ", " << set_hms() << vob.pos.VobId() << "; ";
        io::cout << "time = " << set_hms() << vob.tmLen << "; " << "parts = " << vob.locations.size() << io::endl;
    }

    //ExtractVob(FindVob(dvd_vobs, 2, 39), "..", dvd);
    //ExtractVob(FindVob(dvd_vobs, 34, 1), "..", dvd);
}

///////////////////////////////////////////////////////////

// buf = {0,1,2,3,4,5,6,7,8,9}
class BufTestReadStreambuf: public ReadStreambuf
{
    public:
            BufTestReadStreambuf()
            {
                for( int i=0, cnt = Size(); i<cnt; i++ )
                    buf[i] = i;
            }
    protected:

                char  buf[10];

    virtual pos_type  Size() { return ARR_SIZE(buf); }
    virtual     void  xsgetnImpl(char* s, streamsize real_n) { memcpy(s, buf+pos, real_n); }
};

BOOST_AUTO_TEST_CASE( TestCustomStreambuf )
{
    BufTestReadStreambuf strm_buf;
    std::iostream strm(&strm_buf);

    BOOST_CHECK_EQUAL( (int)strm.tellg(), 0 );
    BOOST_CHECK_EQUAL( (int)strm.get(), 0 );
    BOOST_CHECK_EQUAL( (int)strm.tellg(), 1 );
    BOOST_CHECK_EQUAL( (int)strm.peek(), 1 );

    char buf[10];
    strm.read(buf+1, 9);
    for( int i=1; i<10; i++ )
        BOOST_CHECK_EQUAL( buf[i], i );

    BOOST_CHECK( strm.good() );
    BOOST_CHECK_EQUAL( (int)strm.get(), std::char_traits<char>::eof() );
    BOOST_CHECK( strm.eof() );
    strm.clear();
    BOOST_CHECK( strm.good() );
    strm.seekg(5);
    BOOST_CHECK_EQUAL( (int)strm.tellg(), 5 );
    strm.seekg(3, iof::cur);
    BOOST_CHECK_EQUAL( (int)strm.tellg(), 8 );
}

BOOST_AUTO_TEST_CASE( TestNullStreambuf )
{
    // тест поведения при смене файловых драйверов
    io::stream strm;
    BOOST_CHECK( strm.good() );
    strm.init_buf(ptr::shared<io::fbuf>());
    BOOST_CHECK( !strm.good() );
    strm.init_buf(ptr::shared<io::fbuf>(new io::stdio_sync_filebuf<char>()));
    BOOST_CHECK( strm.good() );
    
    // тест "закрытия" плейера через открытие пустого файлового драйвера
    Mpeg::FwdPlayer plyr;
    io::stream& plyr_strm = plyr.VLine().GetParseContext().dmx.ObjStrm();
    BOOST_CHECK(  plyr_strm.rdbuf() );
    BOOST_CHECK(  plyr.CloseFBuf()  );
    BOOST_CHECK( !plyr_strm.rdbuf() );
}

//////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( TestVobStreambuf )
{
    return;
    const char* dvd_path = "/mnt/ntfs/DVD_Demystified";
    const char* dir_path = "..";
    dvd_reader_t* dvd = DVDOpen(dvd_path);
    BOOST_CHECK(dvd);
    Reader reader(dvd);

    VobArr dvd_vobs; // массив всех VOB
    FillVobArr(dvd_vobs, dvd);

    // 2 способами импортируем vob (34, 1) из DVD, прилагаемый к книге
    // "DVD Demystified".
    // Это 1 из 9 углов показательного видео про робота и громадную надпись
    // microsoft.com.
    // 
    // P.S. Я не фанат Microsoft, просто пример подходящий (небольшой размер + vob
    // состоит из многих кусков, точнее из 21)
    VobPtr vob = FindVob(dvd_vobs, 34, 1); 
    ExtractVob(vob, dir_path, dvd);

    VobStreambuf strm_buf(vob, dvd);
    int len = 16324608;
    BOOST_CHECK_EQUAL( (int)strm_buf.Size(), len );

    //// карта vob
    //Vob::LocationArr& arr = vob->locations;
    //for( Vob::LocationArr::iterator itr = arr.begin(), end = arr.end(); itr != end; ++itr )
    //{
    //    Vob::Part& part = *itr;
    //    io::cout << "off = " << part.off << "; beg = " << part.beg << "; end = " << part.end << io::endl;
    //}

    std::iostream src_strm(&strm_buf);
    io::stream    dst_strm(AppendPath(dir_path, VobFName(vob->pos, "_")).c_str(), iof::out);
    char buf[3000];
    for( int cnt, read_cnt = 0; cnt=std::min(len-read_cnt, (int)ARR_SIZE(buf)), cnt>0; read_cnt += cnt )
    {
        ASSERT( cnt );
        //io::cout << "read_cnt = " << read_cnt << io::endl;

        src_strm.read(buf, cnt);
        BOOST_REQUIRE( src_strm );
        BOOST_CHECK_EQUAL( src_strm.gcount(), cnt );

        dst_strm.write(buf, cnt);
    }
}

} // namespace DVD

