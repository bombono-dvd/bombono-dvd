//
// mlib/patterns.h
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

#ifndef __MLIB_ITERATOR_H__
#define __MLIB_ITERATOR_H__

// Стандартный итератор
template <class Item>
class Iterator
{
    public:

        virtual                ~Iterator() {}
    
                               // в начале
        virtual          void  First() = 0;
                               // следующий
        virtual          void  Next()  = 0;
                               // проверка
        virtual          bool  IsDone() const = 0;
                               // значение
        virtual          Item& CurrentItem() const = 0;

                               // STL-интерфейс
                     Iterator& operator++()
                               { Next(); return *this; }
                               // проверки
                               operator void*() const
                               { return IsDone() ? 0 : *this ; }
                         bool  operator !() const 
                               { return IsDone(); }

    protected:
                               Iterator() {}
};

// простой шаблон для превращения класс в образец "Одиночка"
template<typename Si>
class Singleton
{
    public:
    static        Si& Instance()
                      {
                          static Si si;
                          return si;
                      }
    protected:
                      Singleton() { } // получение возможно только через Instance()
};

//
// Образец "Посетитель"
// 

// Шаблон для классов объектов с простым методом Accept(), 
// Accept(vis) = vis.Visit(*this);
// 
// создаем класс подобного рода так:
//      class MyObj: public SimpleVisitorObject<MyObj, OtherObj> { ... };
// 
template<class ObjT, class ParentObjT, class VisitorT>
class SimpleVisitorObject: public ParentObjT
{
    public:
  virtual  void  Accept(VisitorT& vis) { vis.Visit(static_cast<ObjT&>(*this)); }
};


#endif // #ifndef __MLIB_ITERATOR_H__

