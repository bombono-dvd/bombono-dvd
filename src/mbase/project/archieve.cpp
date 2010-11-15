//
// mbase/project/archieve.cpp
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

#include <libxml/tree.h>

#include "archieve.h"

namespace Project
{

void Archieve::DoStack(bool is_in)
{
    if( is_in )
    {
        if( isSubNode )
            NextNode(*this);
        else
            OpenFirstChild(*this);
    }
    else
    {
        if( isSubNode )
            CloseChild();
    }
    isSubNode = !is_in;
}

void Serialization::SaveArchiever::SerializeStringableImpl(ToStringConverter fnr)
{
    xmlpp::Element* node = ar.Node();
    if( name && *name )
        node->set_attribute(name, fnr());
    else
        // считаем, что неименованный параметр - только один
        node->set_child_text(fnr());
}

void Serialization::LoadArchiever::SerializeStringableImpl(FromStringConverter fnr)
{
    if( name && *name )
        fnr(GetValue(ar, name));
    else
        // считаем, что неименованный параметр - только один
        fnr(ar.OwnerNode()->get_child_text()->get_content());
}

// // :TODO: в отладочном режиме посылаем более подробное исключение вместо
// // std::runtime_error
// static void PrintNode(xmlpp::Node* node)
// {
//     io::cout << "#\n# GetNextNode: " << node->get_name() << "; line: " << node->get_line()
//              << "; XPath: " << node->get_path();
//     io::cout << "; Class name: " << typeid(node).name();
//     io::cout << (node ? "" : "; node is 0!");
//
//     if( xmlpp::ContentNode* content = dynamic_cast<xmlpp::ContentNode*>(node) )
//         io::cout << "\n# Content: " << content->get_content();
//
//     io::cout << "\n#" << io::endl;
// }

xmlpp::Element* GetAsElement(xmlpp::Node* node)
{
    if( node->cobj()->type == XML_ELEMENT_NODE )
        return static_cast<xmlpp::Element*>(node);
    return 0;
}

// xmlpp::Node* GetNextNode(xmlpp::Node::NodeList& list)
// {
//     if( list.empty() )
//         throw std::runtime_error("No node available (2)");
//
//     xmlpp::Node* node = list.front();
//     list.pop_front();
//
//     return node;
// }

// xmlpp::Element* GetElementNode(xmlpp::Node::NodeList& list)
// {
//     xmlpp::Element* node = 0;
//     for( ; node = GetAsElement(GetNextNode(list)), !node; )
//         ;
//     ASSERT( node );
//     return node;
// }

// typedef boost::function<void(xmlpp::Element*)> ElementFunctor;
//
// void ForAllElements(xmlpp::Element* branch_node, ElementFunctor fnr)
// {
//     typedef xmlpp::Node::NodeList NodeList;
//     NodeList list = branch_node->get_children();
//     for(NodeList::iterator itr = list.begin(), end = list.end(); itr != end; ++itr )
//     {
//         xmlpp::Element* node = GetAsElement(*itr);
//         if( node )
//             fnr(node);
//     }
// }

xmlpp::Attribute* GetAttr(Archieve& ar, const char* name)
{
    xmlpp::Attribute* attr = ar.OwnerNode()->get_attribute(name);
    if( !attr )
        throw std::runtime_error(std::string("Can't find attribute ") + name);
    return attr;
}

std::string GetValue(Archieve& ar, const char* name)
{
    return GetAttr(ar, name)->get_value();
}

static xmlpp::Element* GetNextElement(xmlpp::Node* node)
{
    xmlpp::Element* elem = 0;
    if( node )
        for( ; elem = GetAsElement(node), !elem; )
        {
            node = node->get_next_sibling();
            if( !node )
                break;
        }

    if( !elem )
        throw std::runtime_error("No node available");
    return elem;
}

static Archieve& AcceptNextElement(Archieve& ar, xmlpp::Node* from_node)
{
    return ar.AcceptNode(GetNextElement(from_node));
}

Archieve& OpenFirstChild(Archieve& ar)
{
    xmlpp::Node* child = reinterpret_cast<xmlpp::Node*>(ar.Node()->cobj()->children->_private);
    return AcceptNextElement(ar, child);
}

Archieve& NextNode(Archieve& ar)
{
    return AcceptNextElement(ar, ar.Node()->get_next_sibling());
}

void CheckNodeName(Archieve& ar, const char* name)
{
    ASSERT( ar.IsNormed() );
    const std::string node_name = ar.Node()->get_name().raw();
    if( node_name != name )
        throw std::runtime_error(std::string("Node mismatch: ") + 
                                 name + "(expected) vs " + node_name);
}

} // namespace Project
