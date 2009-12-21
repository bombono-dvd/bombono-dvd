//
// mbase/obj_bind.cpp
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

#include <mbase/_pc_.h>

#include "obj_bind.h"

MenuLink::MenuLink(Project::MediaItem ref_, Comp::Object* subj_)
    : ref(ref_), subjDef(0), subj(subj_)
{
    ASSERT( ref && subj );
}

MenuLink::MenuLink(Project::MediaItem ref_, bool is_lower)
    : ref(ref_), subjDef(0), subj(0)
{
    SetBound(is_lower);
    ASSERT( ref );
}

// сравнение одной компоненты, если неравенство (!) не достигается,
// то ворачивает false
template<typename T>
bool CompareComponent(const T& e1, const T& e2, bool& cmp_res)
{
    bool res = true;
    if( e1 < e2 )
        cmp_res = true;
    else if( e2 < e1 )
        cmp_res = false;
    else
        res = false;

    return res;
}

bool MenuLinkLessOp::operator()(const MenuLink& e1, const MenuLink& e2) const
{
    bool cmp_res;
    if( CompareComponent(e1.ref, e2.ref, cmp_res)         ||
        CompareComponent(e1.subjDef, e2.subjDef, cmp_res) || 
        CompareComponent(e1.subj, e2.subj, cmp_res)          )
        return cmp_res;

    return false;
}

void ResetLink(Comp::Object* obj, Project::MediaItem new_ref, Project::MediaItem old_ref)
{
    ASSERT( obj );
    MenuLinkList& Set = MenuLinkList::Instance();
    MenuLinkList::Itr end = Set.end();

    // * удаляем старую связь
    if( old_ref )
    {
        MenuLink lnk(old_ref, obj);
        MenuLinkList::Itr it = Set.find(lnk);
        ASSERT( it != end );
        Set.erase(it);
    }

    // * создаем новую
    if( new_ref )
    {
        bool res = Set.insert(MenuLink(new_ref, obj)).second;
        ASSERT( res );
    }
}

MenuLinkRange LinkedObjects(Project::MediaItem ref)
{
    MenuLinkList& Set = MenuLinkList::Instance();
    MenuLinkList::Itr end = Set.end();

    if( !ref )
        return std::make_pair(end, end);

    MenuLink lnk(ref, true);
    MenuLinkList::Itr from = Set.lower_bound(lnk);

    lnk.SetBound(false);
    MenuLinkList::Itr to   = Set.upper_bound(lnk);
    return std::make_pair(from, to);
}

