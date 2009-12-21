//
// mlib/sigc.h
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

#ifndef __MLIB_SIGC_H__
#define __MLIB_SIGC_H__

#include <sigc++/functors/slot.h>

/////////////////////////////////////////////////////////////////////
// Функция wrap_return()
// 
// В gtkmm хорошо реализован механизм подключения обработчиков к сигналам,
// при этом используется механизм слотов из libsigc++. Не умаляя достоинств
// libsigc (и функций ptr_fun(), mem_fun() в частности), она не обладает такой
// мощью, как Boost.Lambda в части создания неименованных функций (лямбда-функций);
// поэтому и захотелось "прикрутить" Boost.Lambda к libsigc++. Фактически это
// означает, что хочется писать код вроде:
//     bool LayoutExpose(bool is_good, GdkEventExpose* event);
//     using namespace boost;
//     sigc::slot<bool, long> sl = lambda::bind(&MyTempTLayoutExpose, true, lambda::_1);
// чтобы затем подключить слот (=функтор=замыкание) к объекту:
//     layout.signal_expose_event().connect( sl );
//
// Для этого и сделана шаблонная функция wrap_return<Ret>( lambda_expr ); единственный
// шаблонный аргумент, который необходимо указать,- это возвращаемый аргумент, который
// практически всегда должен быть равен возвращ. аргументу самого слота. Итого:
//     sigc::slot<bool, long> sl = wrap_return<bool>( lambda::bind(&MyTempTLayoutExpose, true, lambda::_1) );
// или сразу
//     layout.signal_expose_event().connect( 
//         wrap_return<bool>( lambda::bind(&MyTempTLayoutExpose, true, lambda::_1) ) );
// 
// Замечание: в случае, когда возвращ. значение void, функтор можно не обрамлять wrap_return;
// дело в том, что при создании слота запоминается вся информация о функторе, но не возвращемое
// значение (почему?), см. шаблонные конструкторы классов slotN в sigc++/functors/slot.h, а также
// доки проекта по libsigc++.
// 

template<typename Ret, typename Functor>
struct wrap_return_t: public sigc::functor_base
{
    typedef Ret result_type;
    Functor functor_;

         wrap_return_t(const Functor& fnr): functor_(fnr) {}
         wrap_return_t(const wrap_return_t& wrt): functor_(wrt.functor_) {}
         wrap_return_t&
         operator =(const wrap_return_t& wrt)
         {
             functor_ = wrt.functor_;
             return *this;
         }

         // 0 аргументов
    Ret  operator()() const 
         { return functor_(); }

         // 1
         template <class T_arg1>
    Ret  operator()(T_arg1 A_arg1) const
         { return functor_(A_arg1); }

         // 2
         template <class T_arg1, class T_arg2>
    Ret  operator()(T_arg1 A_arg1, T_arg2 A_arg2) const
         { return functor_(A_arg1, A_arg2); }

         // 3
         template <class T_arg1, class T_arg2, class T_arg3>
    Ret  operator()(T_arg1 A_arg1, T_arg2 A_arg2, T_arg3 A_arg3) const
         { return functor_(A_arg1, A_arg3, A_arg3); }

         // 4
         template <class T_arg1, class T_arg2, class T_arg3, class T_arg4>
    Ret  operator()(T_arg1 A_arg1, T_arg2 A_arg2, T_arg3 A_arg3, T_arg4 A_arg4) const
         { return functor_(A_arg1, A_arg2, A_arg3, A_arg4); }

         // 5
         template <class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5>
    Ret  operator()(T_arg1 A_arg1, T_arg2 A_arg2, T_arg3 A_arg3, T_arg4 A_arg4, T_arg5 A_arg5) const
         { return functor_(A_arg1, A_arg2, A_arg3, A_arg4, A_arg5); }

         // 6
         template <class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5, class T_arg6>
    Ret  operator()(T_arg1 A_arg1, T_arg2 A_arg2, T_arg3 A_arg3, T_arg4 A_arg4, T_arg5 A_arg5, T_arg6 A_arg6) const
         { return functor_(A_arg1, A_arg2, A_arg3, A_arg4, A_arg5, A_arg6); }

         // 7
         template <class T_arg1, class T_arg2, class T_arg3, class T_arg4, class T_arg5, class T_arg6, class T_arg7>
    Ret  operator()(T_arg1 A_arg1, T_arg2 A_arg2, T_arg3 A_arg3, T_arg4 A_arg4, T_arg5 A_arg5, T_arg6 A_arg6, T_arg7 A_arg7) const
         { return functor_(A_arg1, A_arg2, A_arg3, A_arg4, A_arg5, A_arg6, A_arg7); }
};

template<typename Ret, typename Functor>
wrap_return_t<Ret, Functor> wrap_return(Functor fnr)
{
    return wrap_return_t<Ret, Functor>(fnr);
}


#endif // #ifndef __MLIB_SIGC_H__

