//
// mbase/project/serialization.h
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

#ifndef __MBASE_PROJECT_SERIALIZATION_H__
#define __MBASE_PROJECT_SERIALIZATION_H__

#include <iomanip>
#include <boost/type_traits.hpp>

#include <mlib/string.h>
#include "const.h"

namespace Project { namespace Serialization {

namespace mpl = boost::mpl;

typedef boost::function<std::string()> ToStringConverter;
typedef boost::function<void(const std::string& str)> FromStringConverter;

//
// ArchieveFunctor - базовый класс для управления сериализацией объекта,
// для чего используется статический полиморфизм - см. шаблонный аргумент Archiever - 
// конечный класс, порожденный от этого
//
template<class Archiever>
struct ArchieveFunctor
{
       Archiever* This() { return static_cast<Archiever*>(this); }

                  template<class T>
            void  SerializeObject(const T& t)
                  { this->This()->SerializeObjectImpl(t); }

                  template<class ConverterFunctor>
            void  SerializeStringable(ConverterFunctor fnr)
                  { this->This()->SerializeStringableImpl(fnr); }
};

//
// :TODO: используются только как признаки отличия загрузки от сохранения
// переделать на статический bool в ArchieveFunctor вроде 
// 
//    typedef mpl::bool_<true>  IsLoading;
//    typedef mpl::bool_<false> IsSaving;
// 
template<class Archiever>
class SaverFnr: public ArchieveFunctor<Archiever>
{};

template<class Archiever>
class LoaderFnr: public ArchieveFunctor<Archiever>
{};

namespace ToString
{

template<typename T>
std::string MakeString(const T& t)
{
    return (str::stream() << t).str();
}

template<typename T>
std::string MakeStringFromFloat(const T t)
{
    str::stream strm;
    strm << set_hi_precision<T>();
    strm << t;
    return strm.str();
}

template<> inline 
std::string MakeString<float>(const float& t)
{
    return MakeStringFromFloat<float>(t);
}

template<> inline 
std::string MakeString<double>(const double& t)
{
    return MakeStringFromFloat<double>(t);
}

template<> inline 
std::string MakeString<std::string>(const std::string& str)
{
    return str;
}

template<typename T>
ToStringConverter GetConverter(const T& t)
{
    return bb::bind(&MakeString<T>, t);
}

} // namespace ToString

namespace FromString
{

template<typename T>
void MakeType(T& t, const std::string& str)
{
    str::stream(str) >> t;
}

template<> inline 
void MakeType<char>(char& t, const std::string& str)
{
    t = str[0];
}

template<> inline
void MakeType<std::string>(std::string& t_str, const std::string& str)
{
    t_str = str;
}

template<typename T>
FromStringConverter GetConverter(const T& t)
{
    return bb::bind(&MakeType<T>, boost::ref(const_cast<T&>(t)), _1);
}

} // namespace FromString

//
// int, float, char, bool, ...
// std::string
// 

template<class T>
struct DoFundamentalType
{
    template<class Archiever>
    static void Invoke(SaverFnr<Archiever>& ar, const T& t)
    {
        //io::cout << "This is fundamental type, " << t << io::endl;
        ar.SerializeStringable(ToString::GetConverter(t));
    }

    template<class Archiever>
    static void Invoke(LoaderFnr<Archiever>& ar, const T& t)
    {
        //ar.cs(FromString::GetConverter(t));
        ar.SerializeStringable(FromString::GetConverter(t));
    }
};

//
// enum
// 

template<class T>
struct DoEnumType
{
    template<class Archiever>
    static void Invoke(SaverFnr<Archiever>& ar, const T& t)
    {
        //io::cout << "This is enum, " << t << io::endl;
        ar.SerializeStringable(ToString::GetConverter(reinterpret_cast<const int&>(t)));
    }

    template<class Archiever>
    static void Invoke(LoaderFnr<Archiever>& ar, const T& t)
    {
        ar.SerializeStringable(FromString::GetConverter(reinterpret_cast<const int&>(t)));
    }
};

//
// char* 
// (только сохранение)
// 

template<class T>
struct DoStringType
{
    struct saver
    {
        const char* str;
        saver(const char* s): str(s) {}
        std::string operator()() const { return str; }
    };

    template<class Archiever>
    static void Invoke(SaverFnr<Archiever>& ar, const T& t)
    {
        //io::cout << "This is char*-variant, " << t << io::endl;
        
        // при замене B.L. -> B.B. оказалось, что последний не дружит
        // с MakeString<T>() при T = const char[N]; потому заменяем
        // (заодно упрощаем)
        //ar.SerializeStringable(ToString::GetConverter(t));
        ar.SerializeStringable(saver(t));
    }

    template<class Archiever>
    static void Invoke(LoaderFnr<Archiever>& ar, const T& t)
    {
        // загружать (const) char* нельзя!
        //BOOST_MPL_ASSERT(( boost::is_same<T,void> ));
        BOOST_STATIC_ASSERT( 0 == sizeof(T) );
    }
};

//
// general case
//

template<class Archiever, class T>
struct DoGeneralType
{
    static void Invoke(Archiever& ar, const T& t)
    {
        //io::cout << "This is not simple" << io::endl;
        ar.SerializeObject(t);
    }
};

template<class Archiever, class T>
struct DoArrayType;

template<class Archiever, class R, int N>
struct DoArrayType<Archiever, R[N]>
{
    typedef R T[N];
    static void Invoke(Archiever& ar, const T& t)
    {
        typedef
            // if( const char[N] )
            typename mpl::eval_if<boost::is_same<R, char>,
                mpl::identity<DoStringType<T> >,
            // else
                mpl::identity<DoGeneralType<Archiever, T> >
            >::type typex;
        typex::Invoke(ar, t);
    }
};

template<class Archiever, class T>
struct DoPointerType;

template<class Archiever, class S>
struct DoPointerType<Archiever, S*>
{
    typedef S* T;
    static void Invoke(Archiever& ar, const T& t)
    {
        //io::cout << "This is pointer type" << io::endl;
        typedef
            // if( char )
            typename mpl::eval_if<boost::is_same<typename boost::remove_const<S>::type, char>,
                mpl::identity<DoStringType<T> >,
            // else
                mpl::identity<DoGeneralType<Archiever, T> >
            >::type typex;
        typex::Invoke(ar, t);
    }
};

template<class Archiever, class T>
struct DoNonTrivialType
{
    static void Invoke(Archiever& ar, const T& t)
    {
        typedef
            // if( std::string )
            typename mpl::eval_if<boost::is_same<std::string, T>,
                mpl::identity<DoFundamentalType<T> >,
            // else if( s* )
            typename mpl::eval_if<boost::is_pointer<T>,
                mpl::identity<DoPointerType<Archiever, T> >,
            // else if( r[n] )
            typename mpl::eval_if<boost::is_array<T>,
                mpl::identity<DoArrayType<Archiever, T> >,
            // else
                mpl::identity<DoGeneralType<Archiever, T> >
            >
            >
            >::type typex;
        typex::Invoke(ar, t);
    }
};

template<class Archiever, class T>
struct DoType
{
    static void Invoke(Archiever& ar, const T& t)
    {
        typedef 
            typename mpl::eval_if<boost::is_fundamental<T>,
                mpl::identity<DoFundamentalType<T> >,
            // else
            typename mpl::eval_if<boost::is_enum<T>,
                mpl::identity<DoEnumType<T> >,
            // else
                mpl::identity<DoNonTrivialType<Archiever, T> >
            >
            >::type typex;
        typex::Invoke(ar, t);
    }
};


} } // namespace Serialization } namespace Project

#endif // #ifndef __MBASE_PROJECT_SERIALIZATION_H__

