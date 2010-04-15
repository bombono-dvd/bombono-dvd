//
// mgui/sdk/treemodel.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#ifndef __MGUI_SDK_TREEMODEL_H__
#define __MGUI_SDK_TREEMODEL_H__

template<typename FieldsT>
struct ColumnRecord: public Singleton<ColumnRecord<FieldsT> >
{
    //typedef ColumnRecord<FieldsT> Type;
    Gtk::TreeModelColumnRecord  rec;
                       FieldsT  flds;

    protected:
    ColumnRecord(): flds(rec) {}
    friend class Singleton<ColumnRecord>; // из-за не тривиального конструктора
};

template<typename FieldsT>
FieldsT& GetColumnFields()
{
    typedef ColumnRecord<FieldsT> Type;
    return Type::Instance().flds;
}

template<typename FieldsT>
Gtk::TreeModelColumnRecord& GetColumnRecord()
{
    typedef ColumnRecord<FieldsT> Type;
    return Type::Instance().rec;
}

#endif // __MGUI_SDK_TREEMODEL_H__

