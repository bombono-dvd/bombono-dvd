//
// mbase/project/archieve.h
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

#ifndef __MBASE_PROJECT_ARCHIEVE_H__
#define __MBASE_PROJECT_ARCHIEVE_H__

#include <mlib/ptr.h>
#include "archieve_base.h"

namespace Project
{

xmlpp::Element* GetAsElement(xmlpp::Node* node);
std::string GetValue(Archieve& ar, const char* name);

// обход дерева XML при загрузке
Archieve& OpenFirstChild(Archieve& ar);
Archieve& NextNode(Archieve& ar);

//
// для загрузки массива
// 

template<class T>
class ArchieveFunctor: public boost::function<const NameValueT<T>()>
{
    public:
    typedef boost::function<const NameValueT<T>()> FunctorType;

                        ArchieveFunctor() {}
                        ArchieveFunctor(const ArchieveFunctor& af)
                            : FunctorType((const FunctorType&)af) {}
                        ArchieveFunctor(const FunctorType& fnr)
                            : FunctorType(fnr) {}

       ArchieveFunctor& operator =(const FunctorType& fnr)
                        {
                            (FunctorType&)*this = fnr;
                            return *this;
                        }
};

template<class T> inline
ArchieveFunctor<T> MakeArchieveFunctor(typename ArchieveFunctor<T>::FunctorType fnr)
{
    return ArchieveFunctor<T>(fnr);
}

template<class T>
void LoadArray(Archieve& ar, ArchieveFunctor<T> fnr)
{
    ArchieveSave as(ar);

    typedef xmlpp::Node::NodeList NodeList;
    NodeList list = ar.Node()->get_children();
    for(NodeList::iterator itr = list.begin(), end = list.end(); itr != end; ++itr, ar.Norm() )
    {
        xmlpp::Element* node = GetAsElement(*itr);
        if( node )
        {
            ar.AcceptNode(node);

            NameValueT<T> nv = fnr();
            LoadObjectImpl(ar, nv.Name(), nv.Value());
        }
    }
}

//
// Сериализовать "виртуально" блок с именем name, 
// см. пример TestArchieveStackFrame
//
class ArchieveStackFrame
{
    public:
                ArchieveStackFrame(Archieve& ar, const char* name)
                    : ar_(ar)
                {
                    if( ar_.IsLoad() )
                    {
                        ar_.DoStack(true);
                        CheckNodeName(ar_, name);
                    }
                    else
                        nm = new NodeMake(ar_, name);
                }

               ~ArchieveStackFrame()
                {
                    if( ar_.IsLoad() )
                        ar_.DoStack(false);
                }
    protected:

                 Archieve& ar_;
        ptr::one<NodeMake> nm;
};

} // namespace Project


#endif // #ifndef __MBASE_PROJECT_ARCHIEVE_H__

