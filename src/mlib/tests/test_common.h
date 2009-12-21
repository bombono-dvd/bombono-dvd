//
// mlib/tests/test_common.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#ifndef __MLIB_TESTS_TEST_COMMON_H__
#define __MLIB_TESTS_TEST_COMMON_H__

#include <mlib/geom2d.h>
#include <mlib/sdk/stream_util.h>

#include <string>
#include <vector>

#define TEST_DATA_DIR "tools/test-data"

inline std::string GetTestFileName(const char* fnam)
{
    std::string path(TEST_DATA_DIR);
    path += '/';
    path += fnam;

    return path;
}

// = POSIX tmpnam()
char* TmpNam(char* s, const char* prefix = 0);

// удобный класс для тестирования кода - см. примеры использования
// test_ptr.cpp, test_mdata.cpp
class set_bool
{
    bool* b_;
    public:
         set_bool(bool& b): b_(&b) { *b_ = true; }
        ~set_bool() { if( b_ ) *b_ = false; }

         set_bool() : b_(0) {}

         void set_new_bool(bool& b)
         {
             b_ = &b;
             *b_ = true;
         }
};

// имена временных файлов
class TmpFileNames
{
    public:
                    TmpFileNames(): toRemove(true) {}
                   ~TmpFileNames() { ClearTemps(); }

                    // создаем имя файла; все так созданные 
                    // по окончании теста(ов) будут удалены
       std::string  CreateName(const std::string& prefix = std::string());
              void  ClearTemps();

              bool& ToRemove() { return toRemove; }

    protected:
	          bool  toRemove;

                  typedef std::vector<std::string> NamArr;
                  NamArr  namArr; // хранит имена создаваемых файлов для удаления в конце
};

#endif // #ifndef __MLIB_TESTS_TEST_COMMON_H__

