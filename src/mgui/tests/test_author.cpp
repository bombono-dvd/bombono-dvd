//
// mgui/tests/test_author.cpp
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

#include <mgui/tests/_pc_.h>

#include "test_author.h"
#include "test_mbrowser.h"

#include <mgui/init.h>
#include <mgui/author/script.h>

#include <boost/filesystem/convenience.hpp> // fs::create_directories()

namespace Project 
{

bool AuthorDVDTest(const std::string& dir)
{
    fs::create_directories(dir);
    std::string err_str;
    BOOST_CHECK( ClearAllFiles(dir, err_str) );

    return Author::IsGood(AuthorDVD(dir));
}


static void TestAuthoringEx(const std::string& prj_fname, const std::string& out_dir)
{
    GtkmmDBInit gd_init;
    InitAndLoadPrj(prj_fname);

    Execution::ConsoleMode cam;
    BOOST_CHECK( AuthorDVDTest(out_dir) );
}

// неинтерактивный вариант авторинга
BOOST_AUTO_TEST_CASE( TestAuthoring )
{
    if( !IsTestOn("./test_author") )
        return;
    TestAuthoringEx(TestProjectPath(), "../dvd_out");
}

// :TODO: это долгий тест, по умолчанию не включаем;
// реализовать декларацию функции BOOST_AUTO_TEST_CASE_LOOOONG()
//
// регрессионный тест
//
//BOOST_AUTO_TEST_CASE( TestAuthoringRegression )
//{
//    TestAuthoringEx(TestProjectPath("menus-regression.xml"), "../dvd_out-regression");
//}

} // namespace Project


