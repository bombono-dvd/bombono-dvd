//
// mlib/function.h
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

#ifndef __MLIB_FUNCTION_H__
#define __MLIB_FUNCTION_H__

#include <boost/function.hpp>

typedef boost::function<void()> ActionFunctor;

namespace boost {
inline void function_identity() {}
}

// Выполнить действие в конце блока
// 
// Замечание: удобнее использовать при быстрой разработке
// Недостатки:
//  - если из простого последействия код вырос в нечто более сложное,
//    то (удобней & проще & правильней) с нуля написать оригинальную 
//    спец. обертку (с конструтором/деструктором)
//  - Boost.Lambda (1.32) замыкает C-шные функции (не C++) с ошибками
//    (проверил компилятором Comeau)
class DtorAction
{
    public:

        DtorAction(const ActionFunctor& f): fnr(f) {}
       ~DtorAction() { fnr(); }

    private:
        ActionFunctor fnr;
};

#endif // #ifndef __MLIB_FUNCTION_H__

