//
// mlib/dataware.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#ifndef __MLIB_DATAWARE_H__
#define __MLIB_DATAWARE_H__

#include <vector>       // vector
#include <typeinfo>     // type_info

struct DWConstructorTag
{
                  DWConstructorTag() {}
    private:
                  DWConstructorTag(const DWConstructorTag&);
  const DWConstructorTag& operator=(const DWConstructorTag&);
};

typedef const std::type_info TypeInfo; 

class DataWare; 

namespace Detail
{

template<class DataT>
inline DataT* CreateObject(DataWare* /*dw*/, ...)
{
    return new DataT();                                           
}

template<class DataT>
inline DataT* CreateObject(DataWare* dw, DWConstructorTag* /*hint*/)
{
    return new DataT(*dw);
}

template<class DataT> 
void  KillObject(void* o) { delete reinterpret_cast<DataT*>(o); }

} // namespace Detail

//
//  DataWare - массив данных c доступом к элементам по типу (не по индексу)
// Формат элементов может быть двух типов:
// 1) любой тип + конструктор по умолчанию (который и будет вызван при первом
//    вызове); при этом скалярные типы (bool, int, double, указатели) будут
//    инициализированы нулями, (T)0; если вызвать вариант GetData<T, DefInit>(),
//    то при первом создании объект будет проинициализирован оператором DefInit()(T&);
// 2) класс, порожденный от DWConstructorTag + конструктор с одним аргументом,-
//    DataWare& (владелец элемент);
// Примеры:
//  class MyData: public DWConstructorTag
//  {
//      public:
//                          MyData(DataWare&);
//              int var1;
//              int var2;
//  };
//  ...
//  {
//      DataWare dware;
//      MyData& my_dat = dware.GetData<MyData>();
//      int& owned_i   = dware.GetData<int>();
//      ...
//  } 
// 

struct EmptyFunctor
{
    template<class T>
    void operator()(T& ) {}
};

class DataWare
{
    public:

    typedef void(*KillFunctor)(void*);
    struct ElementData
    {
           TypeInfo* type;
               void* data;
        KillFunctor  kFnr;
        
        ElementData(TypeInfo* t, void* d, KillFunctor k)
            :type(t), data(d), kFnr(k) {}
    };
    typedef ElementData ElemType;
    typedef std::vector<ElemType>::iterator ElemIterType;

                  DataWare();
         virtual ~DataWare();

                  // получение локальных данных - основной сервис
                  template<class DataT>
           DataT& GetData()
                  {
                      return GetDataImpl<DataT>(this);
                  }

                  template<class DataT, class DefInitFunctor>
           DataT& GetData()
                  {
                      return GetDataImpl<DataT, DefInitFunctor>(this);
                  }

//                   // :WISH:
//                   template<class DataT>
//             void  RemoveData();

                  // Число данных
             int  Size() { return datArr.size(); }
                  // удалить все локальные данные
            void  Clear();

                  // Работа с именованнованными данными
                  template<class DataT>    
           DataT& GetData(const char* tag) { return GetTaggedDW(tag).GetDataImpl<DataT>(this); }
                  template<class DataT, class DefInitFunctor>    
           DataT& GetData(const char* tag) { return GetTaggedDW(tag).GetDataImpl<DataT, DefInitFunctor>(this); }

        DataWare& GetTaggedDW(const char* tag);
             int  Size(const char* tag)    { return GetTaggedDW(tag).Size(); }
            void  Clear(const char* tag)   { GetTaggedDW(tag).Clear(); }

                  // удалить и неименованные, и именнованные данные
            void  ClearAll();

    protected:

        std::vector<ElemType> datArr;
        struct Pimpl;
                       Pimpl* pimpl;

                  // :WISH: может потребоваться
                  // получается, что помимо деструктора надо еще
                  // хранить конструктор копирования и оператор присваивания
                  DataWare(const DataWare& dw);
        DataWare& operator =(const DataWare& dw);

                  template<class DataT>
           DataT& GetDataImpl(DataWare* owner)
                  {
                      return GetDataImpl<DataT, EmptyFunctor>(owner);
                  }

                  template<class DataT, class DefInitFunctor>
           DataT& GetDataImpl(DataWare* owner)
                  {
                      int idx;
                      TypeInfo* ti = GetTypeInfo<DataT>();
                      if( FindData(ti, idx) )
                      {
                          DataT& dat = GetData<DataT>(idx);
                          return dat;
                      }

                      // создаем новый
                      DataT* pdat = Detail::CreateObject<DataT>(owner, reinterpret_cast<DataT*>(0));
                      Insert(ElemType(ti, reinterpret_cast<void*>(pdat), 
                                        &Detail::KillObject<DataT>), idx);

                      DefInitFunctor()(*pdat);
                      return *pdat;
                  }

                  // найти объект с типом ti (вернется позиция idx)
                  // если не нашли, то idx установлен в позицию, 
                  // куда надо вставлять елемент
            bool  FindData(TypeInfo* ti, int& idx);

                  // взять данные по индексу
                  // за правильностью типа DataT следит вызывающий
                  template<class DataT>    
           DataT& GetData(int idx)
                  {
                      return *reinterpret_cast<DataT*>(datArr[idx].data);
                  }

                  // 
            void  Insert(const ElemType& elem, int idx)
                  {
                      ElemIterType it = datArr.begin() + idx;
                      datArr.insert(it, elem);
                  }

                  template<class DataT>
 static TypeInfo* GetTypeInfo()
                  {
                      static TypeInfo* ti = 0;
                      if( !ti )
                          ti = &typeid(DataT*);
                      return ti;
                  }

};

//
// Различные инициализаторы
// 

// Замечание: для инициализации скалярных типов в 0/false/0.0
// инициализатор не требуется
template<bool DefVal>
struct DefBoolean
{
    void operator()(bool& val) { val = DefVal; }
};

template<int DefVal>
struct DefInteger
{
    void operator()(int& val) { val = DefVal; }
};

struct DefNull
{
    template<typename T>
    void operator()(T*& t) { t = 0; }
};


#endif // #ifndef __MLIB_DATAWARE_H__

