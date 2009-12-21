//
// mbase/project/tests/test_serialization.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#include <mlib/lambda.h>
#include <mlib/stream.h>

#include <mbase/project/serialization.h>

namespace Project { namespace Serialization {

// :TODO: убрать дублирование
typedef boost::function<void()> GeneralCheckArchiever;

class SaveChecker: public SaverFnr<SaveChecker>
{
    public:
    typedef boost::function<void(ToStringConverter)> CnvArchiever;

            CnvArchiever  cs;
   GeneralCheckArchiever  os;

                
                template<class T>
          void  SerializeObjectImpl(const T& /*t*/)
                {
                    os();
                }
          void  SerializeStringableImpl(ToStringConverter fnr)
                {
                    cs(fnr);
                }
};

class LoadChecker: public LoaderFnr<LoadChecker>
{
    public:
    typedef boost::function<void(FromStringConverter)> CnvArchiever;

            CnvArchiever  cs;
   GeneralCheckArchiever  os;

                template<class T>
          void  SerializeObjectImpl(const T& /*t*/)
                {
                    os();
                }
          void  SerializeStringableImpl(FromStringConverter fnr)
                {
                    cs(fnr);
                }
};

void CheckFunction(bool is_true, const std::string& msg)
{
    BOOST_CHECK_MESSAGE( is_true, msg );
}

SaveChecker MakeTypeChecker(bool is_stringable, const char* err_msg)
{
    SaveChecker fnr;
    using namespace boost;

    fnr.cs = lambda::bind(&CheckFunction,  is_stringable, 
                          std::string("type is failed to be general, ") + err_msg);
    fnr.os = lambda::bind(&CheckFunction, !is_stringable, 
                          std::string("type is failed to be fundamental, ") + err_msg);

    return fnr;
}

template<class T>
inline void CheckArchieveType(const T& t, const char* err_str, bool is_stringable = true)
{
    typedef SaveChecker Archiever;
    Archiever ar(MakeTypeChecker(is_stringable, err_str));
    DoType<Archiever, T>::Invoke(ar, t);
}

static std::string ArchieveErrorPrefix(const char* file, int line)
{
    return (str::stream() << "\n    see error location,-\n    " << file 
                          << "(" << line << "): ").str();
}

template<class T>
inline void CheckArchieveTypeFileLine(const T& t, const char* file, int line, bool is_stringable)
{
    std::string err_str = ArchieveErrorPrefix(file, line) + "Archieve Type Deduce Error\n";
    CheckArchieveType(t, err_str.c_str(), is_stringable);
}

#define CHECK_ARCH_TYPE(t, is_stringable) CheckArchieveTypeFileLine(t, __FILE__, __LINE__, is_stringable)
#define CHECK_STR_ARCH_TYPE(t) CHECK_ARCH_TYPE(t, true)
#define CHECK_OTH_ARCH_TYPE(t) CHECK_ARCH_TYPE(t, false)

namespace
{

enum EnumType
{ et1 = 1 };

struct StructType
{};

} // namespace

BOOST_AUTO_TEST_CASE( TestTypeSplitting )
{
    // фундаментальные
    CHECK_STR_ARCH_TYPE(1);
    int i = 99;
    CHECK_STR_ARCH_TYPE(i);
    CHECK_STR_ARCH_TYPE(2l);
    CHECK_STR_ARCH_TYPE(2.3f);
    CHECK_STR_ARCH_TYPE(4.5);
    CHECK_STR_ARCH_TYPE('a');
    CHECK_STR_ARCH_TYPE(true);

    EnumType v_et(et1);
    CHECK_STR_ARCH_TYPE(v_et);
    
    // строки
    const char* str1 = "1";
    CHECK_STR_ARCH_TYPE(str1);
    const char* str11 = "11";
    CHECK_STR_ARCH_TYPE(str11);

    CHECK_STR_ARCH_TYPE("2");
    CHECK_STR_ARCH_TYPE("22");

    char str_arr1[2];
    strcpy(str_arr1, "3");
    CHECK_STR_ARCH_TYPE(str_arr1);
    CHECK_STR_ARCH_TYPE((char*)str_arr1);
    char str_arr2[3];
    strcpy(str_arr2, "33");
    CHECK_STR_ARCH_TYPE(str_arr2);

    std::string std_str("str");
    CHECK_STR_ARCH_TYPE(std_str);
    const std::string c_std_str("const_str");
    CHECK_STR_ARCH_TYPE(c_std_str);

    // не "строко-отображаемые" типы
    StructType st;
    CHECK_OTH_ARCH_TYPE(st);

    typedef int my_arr_type[3];
    my_arr_type my_arr;
    CHECK_OTH_ARCH_TYPE(my_arr);
}

template<class T>
static void CheckLoading(FromStringConverter fnr, T& t, const char* src_str, const T& res, const char* err_str)
{
    fnr(src_str);
    BOOST_CHECK_MESSAGE( t == res, (str::stream() << set_hi_precision<double>() << "loading is not correct, " 
                                                  << t << " != " << res << err_str).str() );
}

template<class T>
inline void CheckTypeLoading(T& t, const char* src_str, const T& res, const char* err_str)
{
    typedef LoadChecker Archiever;
    Archiever ar;
    ar.cs = boost::lambda::bind(CheckLoading<T>, boost::lambda::_1, boost::ref(t), src_str, boost::cref(res), err_str);

    DoType<Archiever, T>::Invoke(ar, t);
}

template<class T>
inline void CheckArchieveLoading(T& t, const char* src_str, const T& res, const char* file, int line)
{
    std::string err_str = ArchieveErrorPrefix(file, line) + "Archieve Type Loading Error\n";
    CheckTypeLoading(t, src_str, res, err_str.c_str());
}

#define CHECK_ARCH_LOADING(t, src_str, res) CheckArchieveLoading(t, src_str, res, __FILE__, __LINE__)

BOOST_AUTO_TEST_CASE( TestTypeLoading )
{
    int num = 0;
    CHECK_ARCH_LOADING(num, "231", 231);
    CHECK_ARCH_LOADING(num, "-11", -11);

    double dbl = 0.;
    CHECK_ARCH_LOADING(dbl, "8.123456", 8.123456);
    CHECK_ARCH_LOADING(dbl, "-1.4", -1.4);

    char c = 0;
    CHECK_ARCH_LOADING(c, "a", 'a');

    bool b = false;
    CHECK_ARCH_LOADING(b, "1", true);
    CHECK_ARCH_LOADING(b, "0", false);

    EnumType v_et = EnumType(et1-1);
    CHECK_ARCH_LOADING(v_et, "1", et1);

    std::string str("");
    CHECK_ARCH_LOADING(str, "test_str", std::string("test_str"));

//     char arr[4];
//     CHECK_ARCH_LOADING(arr, "abc", arr);
};

template<class T>
static void CheckSaving(ToStringConverter fnr, const char* dst_str, const char* err_str)
{
    std::string res = fnr();
    BOOST_CHECK_MESSAGE( res == dst_str, (str::stream() << set_hi_precision<double>() << "saving is not correct, " 
                                                        << res << " != " << dst_str << err_str).str() );
}

template<class T>
inline void CheckTypeSaving(const T& t, const char* dst_str, const char* err_str)
{
    typedef SaveChecker Archiever;
    Archiever ar;
    ar.cs = boost::lambda::bind(CheckSaving<T>, boost::lambda::_1, dst_str, err_str);

    DoType<Archiever, T>::Invoke(ar, t);
}

template<class T>
inline void CheckArchieveSaving(const T& t, const char* dst_str, const char* file, int line)
{
    std::string err_str = ArchieveErrorPrefix(file, line) + "Archieve Type Saving Error\n";
    CheckTypeSaving(t, dst_str,err_str.c_str());
}

#define CHECK_ARCH_SAVING(t, dst_str) CheckArchieveSaving(t, dst_str, __FILE__, __LINE__)

BOOST_AUTO_TEST_CASE( TestTypeSaving )
{
    CHECK_ARCH_SAVING(231, "231");

    CHECK_ARCH_SAVING(0.12345678, "0.12345678");

    CHECK_ARCH_SAVING('a', "a");

    CHECK_ARCH_SAVING(true,  "1");
    CHECK_ARCH_SAVING(false, "0");

    CHECK_ARCH_SAVING(et1, "1");

    CHECK_ARCH_SAVING(std::string("test_str"), "test_str");
};


} } // namespace Serialization } namespace Project

