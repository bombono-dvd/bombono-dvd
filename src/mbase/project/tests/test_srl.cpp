//
// mbase/project/test_srl.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008, 2010 Ilya Murav'jov
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

#include <mbase/tests/_pc_.h>

#include <mbase/project/archieve.h>
#include <mbase/project/srl-common.h>

using namespace Project;

////////////////////////////////////////////////////////////
// TestArchieveStackFrame

void TestSerializeBool(xmlpp::Element* root_node, bool is_read, bool& true_)
{
    Archieve ar(root_node, is_read);

    ArchieveStackFrame asf(ar, "test_bool");
    BOOST_CHECK_EQUAL( ar.OwnerNode()->get_name(), "test_bool" );

    ar & NameValue("true", true_);
}

BOOST_AUTO_TEST_CASE( TestArchieveStackFrame )
{
    xmlpp::Document doc;
    xmlpp::Element* root_node = doc.create_root_node("test");

    bool true_ = true;
    TestSerializeBool(root_node, false, true_);
    true_ = false;
    //doc.write_to_file("../ttt");
    TestSerializeBool(root_node, true, true_);

    BOOST_CHECK( true_ );
}

////////////////////////////////////////////////////////////
// TestNormArray - проверка сериализация массива с нетривиальными 

namespace 
{

struct TestData
{
    Point pnt;
};

void Serialize(Archieve& ar, TestData& td)
{
    ar & NameValue("Point", td.pnt);
}

typedef std::vector<TestData> TDArray;

NameValueT<TestData> LoadTestData(TDArray& td_arr)
{
    td_arr.push_back(TestData());
    return NameValue("Data", td_arr[td_arr.size()-1]);
}

} // namespace 

BOOST_AUTO_TEST_CASE( TestNormArray )
{
    xmlpp::Document doc;
    xmlpp::Element* root_node = doc.create_root_node("test");

    TDArray arr;
    arr.push_back(TestData());
    arr[0].pnt = Point(0, 1);
    arr.push_back(TestData());
    arr[0].pnt = Point(2, 3);

    // * запись
    {
        Archieve ar(root_node, false);
        for( TDArray::iterator itr = arr.begin(), end = arr.end(); itr != end; ++itr )
            ar << NameValue("Data", *itr);
        //doc.write_to_file("../test_points");
    }

    TDArray arr2;
    // * загрузка
    {
        Archieve ar(root_node, true);
        ArchieveFunctor<TestData> fnr =
            MakeArchieveFunctor<TestData>( bb::bind(&LoadTestData, boost::ref(arr2)) );
        LoadArray(ar, fnr);
    }

    BOOST_CHECK( arr.size() == arr2.size() );
    BOOST_CHECK( arr[0].pnt == arr2[0].pnt );
    BOOST_CHECK( arr[1].pnt == arr2[1].pnt );
}

