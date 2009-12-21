//
// mlib/dataware.cpp
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

#include <algorithm>    // lower_bound
#include <string>

#include "dataware.h"

// 
// DataWare::Pimpl
// 

struct DataWare::Pimpl
{
    typedef std::pair<std::string, DataWare*> NamedDW;
    typedef std::vector<NamedDW> DWArray;

    DWArray dwArr;

                ~Pimpl();

    static bool  NamedDWLessOp(const NamedDW& e1, const NamedDW& e2)
    {
        return e1.first < e2.first;
    }
};

DataWare::Pimpl::~Pimpl()
{
    for( DWArray::iterator it = dwArr.begin(), end = dwArr.end(); it != end; ++it )
        delete it->second;
}

DataWare& DataWare::GetTaggedDW(const char* tag)
{
    // * отложенная инициализация
    if( !pimpl )
        pimpl = new Pimpl;

    typedef Pimpl::DWArray DWArray;
    typedef Pimpl::NamedDW NamedDW;
    // * поиск
    DWArray& arr = pimpl->dwArr;
    NamedDW obj(tag, 0);

    DWArray::iterator beg = arr.begin(), end = arr.end();
    DWArray::iterator itr = std::lower_bound(beg, end, obj, Pimpl::NamedDWLessOp);
    if( (itr != end) && (obj.first == itr->first) )
        return *itr->second;

    // * создание
    obj.second = new DataWare;
    arr.insert(itr, obj);
    return *obj.second;
}


DataWare::DataWare() : pimpl(0)
{}

DataWare::~DataWare() 
{ 
    ClearAll();
}

void DataWare::ClearAll()
{
    Clear();
    if( pimpl )
        delete pimpl;
    pimpl = 0;
}

void DataWare::Clear()
{
    for( ElemIterType iter = datArr.begin(), end = datArr.end(); iter != end; ++iter )
        iter->kFnr(iter->data);
    datArr.clear();
}

static bool ElemTypeLessOp(const DataWare::ElemType& e1, const DataWare::ElemType& e2)
{
    return e1.type->before(*e2.type);
}

bool DataWare::FindData(TypeInfo* ti, int& idx)
{
    ElemType elem(ti, 0, 0);
    ElemIterType beg = datArr.begin(), end = datArr.end();

    ElemIterType iter = std::lower_bound(beg, end, elem, ElemTypeLessOp);
    idx = iter - beg;

    return (iter != end) && (*ti == *iter->type);
}

