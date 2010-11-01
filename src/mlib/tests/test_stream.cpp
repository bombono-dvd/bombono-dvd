//
// mlib/tests/test_stream.cpp
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

#include <mlib/tests/_pc_.h>

#include "test_common.h"

#include <mlib/mlib.h>

// Для тестирования кода
#include <errno.h>
#include <vector>
#include <string.h>

//
// "интерактивный" тест
// 
// BOOST_AUTO_TEST_CASE( TestStreams_rare )
// {
//     // проверка стандартных потоков
//     io::cout << "test cout" << io::endl;
//     io::cout << io::hex << 16 << io::endl;
//
//     // конструктор on int
//     {
//         io::stream my_cin(0, iof::in, false);
//         // Введите 777
//         io::cout << "Input 777 (test)" << io::endl;
//         int i = 0;
//         my_cin >> i;
//         BOOST_CHECK( i == 777 );
//
//         my_cin.sync();
//         io::cout.flush();
//     }
// }

BOOST_AUTO_TEST_CASE( TestStreams )
{
    char buffer[STRM_BUF_SZ];
    TmpFileNames tmp_names;
    //tmp_names.ToRemove() = false;

    std::string src_fnam = GetTestFileName("flower.yuv");
    std::string dst_fnam = tmp_names.CreateName("TestStreams");

    // 1 read & write
    {
        io::stream strm(src_fnam.c_str(), iof::in|iof::out);
        io::stream out_strm(dst_fnam.c_str(), iof::out|iof::trunc);

        for( int cnt=0; cnt = strm.raw_read(buffer, STRM_BUF_SZ), strm; )
        {
            out_strm.raw_write(buffer, cnt);
        }

        strm.flush();

        BOOST_CHECK( strm.is_open() );
        strm.close();
        BOOST_CHECK( !strm.is_open() );
    }

    // проверка read & write
    {
        io::stream strm(src_fnam.c_str(), iof::in);
        strm.seekg(0, iof::end);
        io::stream out_strm(dst_fnam.c_str(), iof::in);
        out_strm.seekg(0, iof::end);

        int strm_sz = strm.tellg();
        int out_strm_sz = out_strm.tellg();
        BOOST_CHECK(strm_sz == out_strm_sz);

        int mid = strm_sz/2;
        int after_mid = strm_sz - mid;

        strm.seekg(mid);
        out_strm.seekg(mid);
        char buffer[STRM_BUF_SZ];
        char out_buffer[STRM_BUF_SZ];
        // для интереса начнем со второй половины
        for( int rest=after_mid, cnt; 
             cnt=std::min(rest, STRM_BUF_SZ) , strm.raw_read(buffer, cnt), rest>0; 
             rest -= cnt  )
        {
            out_strm.raw_read(out_buffer, cnt);

            if( memcmp(buffer, out_buffer, cnt) != 0 )
                BOOST_CHECK(false);
        }

        strm.seekg(0);
        out_strm.seekg(0);
        for( int rest=mid, cnt; 
             cnt=std::min(rest, STRM_BUF_SZ) , strm.raw_read(buffer, cnt), rest>0; 
             rest -= cnt  )
        {
            out_strm.raw_read(out_buffer, cnt);

            if( memcmp(buffer, out_buffer, cnt) != 0 )
                BOOST_CHECK(false);
        }

        strm.clear();
        out_strm.clear();
        int c;
        BOOST_CHECK( (c = strm.get()) == out_strm.get() );
        //VerboseStream(strm);
        //VerboseStream(out_strm);
        strm.unget();
        out_strm.unget();
        BOOST_CHECK( c == strm.peek() && c == out_strm.peek() );

    }

    // 2 размер файла - некратный буферу
    {
        std::string tmp_nam = tmp_names.CreateName();
        io::stream strm(tmp_nam.c_str(), iof::trunc|iof::out);

        strm << "111";
        strm.close();

        strm.open(tmp_nam.c_str(), iof::in);
        int all_cnt = 0;
        for( int cnt=0; cnt=strm.raw_read(buffer, STRM_BUF_SZ), strm; all_cnt += cnt )
        {
            BOOST_CHECK(cnt>0);
            BOOST_CHECK(strm);
        }
        BOOST_CHECK(!strm);
        BOOST_CHECK(all_cnt == 3);
        
    }

    // +read (2), +write (2), +sync (=flush для io-потоков), +peek, +unget, +is_open, +tellg/p, 
    // +seekg/p

    // 3 io::stream::file()
    {
        std::string tmp_nam = tmp_names.CreateName();
        io::stream strm(tmp_nam.c_str(), iof::trunc|iof::out);

        FILE* p_file = strm.file();
        BOOST_CHECK( p_file ); // C-file

        strm << "strm";
        int last_pos;
        {
            io::fd_proxy fp = strm.fileno();
            int fd = fp;
            BOOST_CHECK( io::tell(fd) == (int)strm.tellp() );
            // убираем warning: ignoring return value of
            int res = write(fd, " fd", 3);
            (void)res;

            last_pos = io::tell(fd);
        }
        //VerboseStream(strm);
        BOOST_CHECK( last_pos == (int)strm.tellp() );
        strm << " again";

        strm.close();
        strm.open(tmp_nam.c_str(), iof::in);

        char buf[100];
        const char* text="strm fd again";
        strm.read(buf, strlen(text));
        BOOST_CHECK( memcmp(text, buf, strlen(text)) == 0 );

        int c = strm.get();
        BOOST_CHECK( c == io::stream::traits_type::eof() );
    }


    // 4 проверка компиляции 
    {
        BOOST_CHECK( iof::in == std::ios::in );
        BOOST_CHECK( iof::in_out == (std::ios::in|std::ios::out) );

        BOOST_CHECK( iof::beg == std::ios::beg );
    }

    // 5 swap streams
    {
        io::stream strm;

        BOOST_CHECK( !strm.is_open() );
        strm.swap_cstreams(stdout);

        BOOST_CHECK( strm.get_close() );
        BOOST_CHECK( strm.is_open() );

        strm.set_close(false);
        BOOST_CHECK( !strm.get_close() );

        // проверяем через ftell(), что не закрылся (что есть такой FILE*=stdout)
        errno = 0;
        // убираем warning: ignoring return value of
        long res = ftell(stdout);
        (void)res;
        BOOST_CHECK( errno != EBADF );
    }
}
