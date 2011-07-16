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

#include "archieve.h"

#include <mlib/format.h>

#include <libxml/tree.h>

namespace Project
{

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

static std::string NodeName(xmlpp::Element* node)
{
    return node->get_name().raw();
}

static std::string QuotedNodeName(xmlpp::Element* node)
{
    return QuotedName(NodeName(node));
}

static void Error(const std::string& err_str)
{
    throw std::runtime_error(err_str);
}

static xmlpp::Element* FindNextElement(xmlpp::Node* node)
{
    xmlpp::Element* elem = 0;
    if( node )
        for( ; elem = GetAsElement(node), !elem; )
        {
            node = node->get_next_sibling();
            if( !node )
                break;
        }

    return elem;
}

static void AcceptNode(Archieve& ar, xmlpp::Element* elem, bool is_parent)
{
    if( !elem )
    {
        const char* msg = is_parent ? "No node in %1%" : "No node after %1%" ;
        Error(boost::format(msg) % QuotedNodeName(ar.Node()) % bf::stop);
    }
    ar.AcceptNode(elem);
}

#define TEST_NUM_COND(PREFIX, num, COND) \
    (PREFIX##_VERSION > (num) || (PREFIX##_VERSION == (num) && (COND)))

// обобщение GTK_CHECK_VERSION
#define	IS_VERSION_GE(PREFIX, major, minor, micro) \
    TEST_NUM_COND(PREFIX##_MAJOR, major, TEST_NUM_COND(PREFIX##_MINOR, minor, PREFIX##_MICRO_VERSION >= (micro)))

static void OpenFirstChild(Archieve& ar)
{
    xmlpp::Element* node = ar.Node();

    // :KLUDGE: у libxml++ нет оф. возможности получить только одного, первого потомка,
    // потому используем неофиц. доступ
    xmlNode* children = node->cobj()->children;
    if( !children )
        Error(boost::format("No content in %1%") % QuotedNodeName(node) % bf::stop);

    // С 2.33.1 С++-обертки создаются по требованию (прозрачно, для не использующих
    // неофиц. доступ)
    // :KLUDGE: libxml++ не публикует в заголовках свою версию (а усложнение сборки
    // через pkg-config --modversion libxml++-2.6 не стоит делать,- игра не стоит свеч)
#if IS_VERSION_GE(GLIBMM, 2, 27, 4)
    xmlpp::Node::create_wrapper(children);
#endif
    xmlpp::Node* child = reinterpret_cast<xmlpp::Node*>(children->_private);
    AcceptNode(ar, FindNextElement(child), true);
}

static void NextNode(Archieve& ar)
{
    AcceptNode(ar, FindNextElement(ar.Node()->get_next_sibling()), false);
}

void CheckNodeName(Archieve& ar, const char* name)
{
    ASSERT( ar.IsNormed() );
    const std::string node_name = NodeName(ar.Node());
    if( node_name != name )
        Error(boost::format("Node mismatch: %1%(expected) vs %2%") 
              % QuotedName(name) % QuotedName(node_name) % bf::stop);
}

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

void LoadArray(Archieve& ar, const ArchieveFnr& fnr, const char* req_name)
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

            CheckNodeName(ar, req_name);
            fnr(ar);
        }
    }
}

} // namespace Project
