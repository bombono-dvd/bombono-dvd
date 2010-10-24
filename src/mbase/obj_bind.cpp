//
// mbase/obj_bind.cpp
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

#include <mbase/_pc_.h>

#include "obj_bind.h"

#include <mlib/patterns.h>

#include <set>

struct MILink
{
    Project::MediaItem  ref;
                        // -1 - ищем связи только по ref, нижняя граница
                        // 0  - по ref и subj (полный индекс)
                        // 1  - только по ref, верхняя граница
                   int  subjDef;
          Comp::Object* subj;

          // для поиска по полному ключу 
          // (без нулевых значений)
          MILink(Project::MediaItem ref, Comp::Object* subj);
          // для поиска по ref
          MILink(Project::MediaItem ref, bool is_lower);

    void  SetBound(bool is_lower) { subjDef = is_lower ? -1 : 1; }
    private:
          MILink(); // запрещен
};

struct MILinkLessOp
{
    bool operator()(const MILink& e1, const MILink& e2) const;
};

//
// MILinkList - связи пунктов меню с медиа (их содержимым):
//  MenuRegion    -> bgRef
//  MediaObj      -> mdItem
// 
typedef std::set<MILink, MILinkLessOp> MILinkListType;

class MILinkList: public MILinkListType
{
    typedef MILinkListType MyParent;
    public:

    typedef MyParent::iterator Itr;
};

// список пунктов меню, связанных с ref
typedef std::pair<MILinkList::Itr, MILinkList::Itr> MenuLinkRange;
MenuLinkRange LinkedObjects(Project::MediaItem ref);

MILink::MILink(Project::MediaItem ref_, Comp::Object* subj_)
    : ref(ref_), subjDef(0), subj(subj_)
{
    ASSERT( ref && subj );
}

MILink::MILink(Project::MediaItem ref_, bool is_lower)
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

bool MILinkLessOp::operator()(const MILink& e1, const MILink& e2) const
{
    bool cmp_res;
    if( CompareComponent(e1.ref, e2.ref, cmp_res)         ||
        CompareComponent(e1.subjDef, e2.subjDef, cmp_res) || 
        CompareComponent(e1.subj, e2.subj, cmp_res)          )
        return cmp_res;

    return false;
}

// установить/изменить/удалить связь
void ResetLink(MILinkList& Set, Comp::Object* obj, Project::MediaItem new_ref, Project::MediaItem old_ref)
{
    ASSERT( obj );
    MILinkList::Itr end = Set.end();

    // * удаляем старую связь
    if( old_ref )
    {
        MILink lnk(old_ref, obj);
        MILinkList::Itr it = Set.find(lnk);
        ASSERT( it != end );
        Set.erase(it);
    }

    // * создаем новую
    if( new_ref )
    {
        bool res = Set.insert(MILink(new_ref, obj)).second;
        ASSERT_OR_UNUSED( res );
    }
}

MenuLinkRange LinkedObjects(MILinkList& Set, Project::MediaItem ref)
{
    MILinkList::Itr end = Set.end();

    if( !ref )
        return std::make_pair(end, end);

    MILink lnk(ref, true);
    MILinkList::Itr from = Set.lower_bound(lnk);

    lnk.SetBound(false);
    MILinkList::Itr to   = Set.upper_bound(lnk);
    return std::make_pair(from, to);
}

void ForeachLinked(MILinkList& links, Project::MediaItem mi, CompObjectFunctor fnr)
{
    for( MenuLinkRange rng = LinkedObjects(links, mi); rng.first != rng.second ; )
    {
        ASSERT( rng.first->ref == mi );
        Comp::Object* obj = rng.first->subj;
        ASSERT( obj );
        ++rng.first; // чтоб можно было удалять ссылки в цикле

        fnr(obj);
    }
}

MILinkList& MenuLinks()
{
    static MILinkList List;
    return List;
}

MILinkList& PosterLinks()
{
    static MILinkList List;
    return List;
}

//////////////////////////////////
// MediaLink

void CommonMediaLink::SetLink(Project::MediaItem mi)
{
    ResetLink(mi, link);
    link = mi;
}

CommonMediaLink::CommonMediaLink(Comp::Object* own): owner(own)
{
    ASSERT( owner );
}

void CommonMediaLink::ResetLink(Project::MediaItem new_ref, Project::MediaItem old_ref)
{
    ::ResetLink(GetLinks(), owner, new_ref, old_ref);
}

MILinkList& MediaLink::GetLinks() 
{
    return MenuLinks();
}

PosterLink::PosterLink(Comp::FramedObj* own): MyParent(own) {}

MILinkList& PosterLink::GetLinks() 
{
    return PosterLinks();
}

//////////////////////////////////
// API

void ForeachLinked(Project::MediaItem mi, CompObjectFunctor fnr)
{
    ForeachLinked(MenuLinks(), mi, fnr);
}

static void PosterFunctorImpl(Comp::Object* obj, const Composition::FOFunctor& fnr)
{
    // только ради страховки проверяем
    Comp::FramedObj* f_obj = dynamic_cast<Comp::FramedObj*>(obj);
    ASSERT( f_obj );

    fnr(*f_obj);
}

void ForeachWithPoster(Project::MediaItem mi, Composition::FOFunctor fnr)
{
    ForeachLinked(PosterLinks(), mi, bl::bind(&PosterFunctorImpl, bl::_1, boost::ref(fnr)));
}

std::string MediaItem2String(Project::MediaItem mi)
{
    std::string res("0(Null)");
    if( mi )
        res = mi->mdName + "(" + mi->TypeString() + ")";
    return res;
}

void PrintAbandonedLinks(MILinkList& links)
{
    io::cout << "###############" << io::endl;
    io::cout << "Abandoned Links" << io::endl;
    io::cout << "###############" << io::endl;
    boost_foreach( const MILink& lnk, links )
        io::cout << MediaItem2String(lnk.ref) << io::endl;
    io::cout << "###############" << io::endl;
}

// удостовериться, что все связей нет
void CheckObjectLinksEmpty()
{
    // видеопереходы
    if( !MenuLinks().empty() )
    {
        PrintAbandonedLinks(MenuLinks());
        ASSERT( 0 );
    }
    // постеры для кнопок
    if( !PosterLinks().empty() )
    {
        PrintAbandonedLinks(PosterLinks());
        ASSERT( 0 );
    }
}


