//
// mbase/project/archieve_base.h
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

#ifndef __MBASE_PROJECT_ARCHIEVE_BASE_H__
#define __MBASE_PROJECT_ARCHIEVE_BASE_H__

#include "serialization.h"
#include "const.h"

//
// Идея сериализация взята из библиотеки Boost.Serialization;
// саму B.S (в случае xml) использовать неудобно (разные цели):
// - явно неприглядный конечный вид файла xml (для хранения "документных"
//   данных, читаемых человеком);
// - формат в xml-варинте в B.S не определен, и может измениться,
//   см. док. по ней.
// 

namespace Project
{

class Archieve;

////////////////////////////////////////////////////////////

/*
Copyright Boost.Serialization' authors, boost/serialization/nvp.hpp
*/

// COPY_N_PASTE_ETALON

template<class T>
struct NameValueT : public std::pair<const char*, T*>
{
    typedef std::pair<const char*, T*> MyParent;

                NameValueT(const char* name, T& t)
                    :MyParent(name, (T*)(&t))
                {}
                NameValueT(const NameValueT& nv) : 
                    MyParent(nv.first, (T*)nv.second)
                {}

    const char* Name() const 
                { return this->first; }
             T& Value() const 
                { return *(this->second); }

       const T& ConstValue() const 
                { return *(this->second); }
};

template<typename T>
const NameValueT<T> NameValue(const char* name, const T& t)
{
    return NameValueT<T>(name, const_cast<T&>(t));
}

////////////////////////////////////////////////////////////

namespace Serialization
{

class SaveArchiever: public SaverFnr<SaveArchiever>
{
    public:
                SaveArchiever(Archieve& ar_, const char* nm)
                    :ar(ar_), name(nm) {}

                template<class T>
          void  SerializeObjectImpl(const T& t);
          void  SerializeStringableImpl(ToStringConverter fnr);

    protected:

          Archieve& ar;
        const char* name;
};

class LoadArchiever: public LoaderFnr<LoadArchiever>
{
    public:
                LoadArchiever(Archieve& ar_, const char* nm)
                    :ar(ar_), name(nm) {}

                template<class T>
          void  SerializeObjectImpl(const T& t);
          void  SerializeStringableImpl(FromStringConverter fnr);

    protected:

          Archieve& ar;
        const char* name;
};

} // Serialization

class Archieve
{
    public:

                   Archieve(xmlpp::Element* base_node, bool is_read)
                       : curNode(base_node), isLoad(is_read), isSubNode(false) {}

             bool  IsLoad() const { return isLoad;  }
             bool  IsSave() const { return !isLoad; }

                   // для создания узлов пользоваться MakeChild
   xmlpp::Element* Node() const { return curNode; }
         Archieve& AcceptNode(xmlpp::Element* node) 
                   { 
                       curNode   = node;
                       return *this; 
                   }

                   // добавление подузлов
             void  MakeChild(const char* name = 0) 
                   { 
                       const char* tmp = (name && *name) ? name : "_temp_"; 
                       AcceptNode(curNode->add_child(tmp));
                   }
             void  CloseChild() 
                   { 
                       xmlpp::Element* par_node = curNode->get_parent();
                       if( !par_node )
                           throw std::runtime_error("Out of XML tree");
                       AcceptNode(par_node);
                   }

                   template<typename T>
         Archieve& operator <<(const NameValueT<T>& t)
                   {
                       ASSERT( !isLoad );

                       using namespace Serialization;
                       SaveArchiever sa(*this, t.Name());
                       DoType<SaveArchiever, T>::Invoke(sa, t.ConstValue());
                       return *this;
                   }

                   template<typename T>
         Archieve& operator >>(const NameValueT<T>& t)
                   {
                       ASSERT( isLoad );

                       using namespace Serialization;
                       LoadArchiever la(*this, t.Name());
                       DoType<LoadArchiever, T>::Invoke(la, t.ConstValue());
                       return *this;
                   }

                   template<typename T>
         Archieve& operator &(const NameValueT<T>& t)
                   {
                       if( isLoad )
                           *this >> t;
                       else
                           *this << t;
                       return *this;
                   }
                   // краткий вариант &
                   template<typename T>
         Archieve& operator ()(const char* name, T& t)
                   {
                       return *this & NameValueT<T>(name, t);
                   }

                   //
                   // Спец. функции при "автоматическом чтении >>"
                   //

   xmlpp::Element* OwnerNode() const 
                   { return !isSubNode ? curNode : curNode->get_parent() ; }
                   // зайти/выйти по "стеку" xml
             void  DoStack(bool is_in);
                   // выйти из "автомат." режима -> снять isSubNode
             void  Norm()
                   {
                       if( isSubNode )
                       {
                           isSubNode = false;
                           CloseChild();
                       }
                   }
             bool  IsNormed() const { return !isSubNode; }

    protected:

        xmlpp::Element* curNode;
            const bool  isLoad; // в процессе сериализации нет смысла менять 
                                // направление чтение/запись

    private:
                        // при загрузке показывает когда мы в дочернем узле
                        // используется только в "автоматическом" режиме чтения, потому
                        // при сохранении состояния (ArchieveSave) всегда сбрасывается
                  bool  isSubNode;

                   Archieve();
                   Archieve(const Archieve& ar);
            friend class ArchieveSave;
};

class ArchieveSave
{
    public:
                ArchieveSave(Archieve& ar)
                    : ar_(ar), node(ar.Node()), isSubNode(ar.isSubNode) 
                { 
                    ar_.Norm(); 
                }
               ~ArchieveSave()
                { 
                    ar_.AcceptNode(node);
                    ar_.isSubNode = isSubNode;
                }
    protected:
        Archieve& ar_;
  xmlpp::Element* node;
            bool  isSubNode;
};

class NodeMake: public ArchieveSave
{
    public:
                NodeMake(Archieve& ar, const char* name = 0)
                    : ArchieveSave(ar)
                { ar_.MakeChild(name); }
};

class ArchieveLoadFrame
{
    public:
                ArchieveLoadFrame(Archieve& ar)
                    : ar_(ar)
                { ar_.DoStack(true); }
               ~ArchieveLoadFrame()
                { ar_.DoStack(false); }
    protected:
        Archieve& ar_;
};

// общая форма сохранения объекта
template<typename T>
inline void Serialize(Archieve& ar, T& t)
{
    t.Serialize(ar);
}

template<typename T> inline 
void Serialization::SaveArchiever::SerializeObjectImpl(const T& t)
{
    NodeMake nm(ar, name);
    Serialize(ar, const_cast<T&>(t));
}

void CheckNodeName(Archieve& ar, const char* name);

template<class T> inline
void LoadObjectImpl(Archieve& ar, const char* name, T& t)
{
    CheckNodeName(ar, name);
    Serialize(ar, t);
}

template<class T> inline
void Serialization::LoadArchiever::SerializeObjectImpl(const T& t)
{
    ArchieveLoadFrame asf(ar);
    //CheckNodeName(ar, name);
    //Serialize(ar, const_cast<T&>(t));
    LoadObjectImpl(ar, name, const_cast<T&>(t));
}

#define APROJECT_SRL_SPLIT_FREE(T)          \
inline void Serialize(Archieve& ar, T& t)   \
{                                           \
    if( ar.IsLoad() )                       \
        Load(ar, t);                        \
    else                                    \
        Save(ar, t);                        \
}                                           \
/**/

#define APROJECT_SRL_SPLIT_MEMBER()         \
void Serialize(Archieve &ar)                \
{                                           \
    if( ar.IsLoad() )                       \
        this->Load(ar);                     \
    else                                    \
        this->Save(ar);                     \
}                                           \
/**/

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_ARCHIEVE_BASE_H__

