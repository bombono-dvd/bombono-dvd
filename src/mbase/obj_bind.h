//
// mbase/obj_bind.h
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

#ifndef __MBASE_OBJ_BIND_H__
#define __MBASE_OBJ_BIND_H__

#include "composite/component.h"

#include <mlib/patterns.h>
#include <mlib/function.h>

#include <set>


struct MenuLink
{
    Project::MediaItem  ref;
                        // -1 - ищем связи только по ref, нижняя граница
                        // 0  - по ref и subj (полный индекс)
                        // 1  - только по ref, верхняя граница
                   int  subjDef;
          Comp::Object* subj;

          // для поиска по полному ключу 
          // (без нулевых значений)
          MenuLink(Project::MediaItem ref, Comp::Object* subj);
          // для поиска по ref
          MenuLink(Project::MediaItem ref, bool is_lower);

    void  SetBound(bool is_lower) { subjDef = is_lower ? -1 : 1; }
    private:
          MenuLink(); // запрещен
};

struct MenuLinkLessOp
{
    bool operator()(const MenuLink& e1, const MenuLink& e2) const;
};

//
// MenuLinkList - связи пунктов меню с медиа (их содержимым):
//  MenuRegion    -> bgRef
//  MediaObj      -> mdItem
// 
typedef std::set<MenuLink, MenuLinkLessOp> MenuLinkListType;

class MenuLinkList: public MenuLinkListType, public Singleton<MenuLinkList>
{
    typedef MenuLinkListType MyParent;
    public:

    typedef MyParent::iterator Itr;
};

// установить/изменить/удалить связь
void ResetLink(Comp::Object* obj, Project::MediaItem new_ref, Project::MediaItem old_ref);
// список пунктов меню, связанных с ref
typedef std::pair<MenuLinkList::Itr, MenuLinkList::Itr> MenuLinkRange;
MenuLinkRange LinkedObjects(Project::MediaItem ref);

typedef boost::function<void(Comp::Object*)> CompObjectFunctor;
void ForeachLinked(Project::MediaItem mi, CompObjectFunctor fnr);

#endif // #ifndef __MBASE_OBJ_BIND_H__

