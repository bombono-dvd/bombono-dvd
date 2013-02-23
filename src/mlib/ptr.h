//
// mlib/ptr.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008, 2010 Ilya Murav'jov
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

#ifndef __MLIB_PTR_H__
#define __MLIB_PTR_H__

#include <memory>
#include <boost/smart_ptr.hpp>

//
//  Умные указатели: ptr::one и ptr::shared
//
// В силу нескольких причин, указанных ниже, использовать
// общеизвестные std::auto_ptr и boost::shared_ptr, неудобно;
// потому здесь определены их замены, ptr::one и ptr::shared.
// Однако синтаксис их api точно такой, же что и оригиналов,- 
//      get(), reset(), release(), use_count(), ...
// 
// Причины:
// 1) std::auto_ptr неудобно использовать, потому что конструкция
//      if( ptr ) ...
//    не скомпилируется; то же самое со boost::scoped_ptr - бедный интерфейс
// 2) почему ptr::one, а не ptr::auto? - Слово "auto" зарезервировано
//    в языке C, хотя сейчас и не имеет никакого значения (исторически
//    сложилось). Потому подобрано (короткое) слово "one" - "владеть одному, 
//    единолично", в противовес "shared" - "общее владение".
// 3) boost::shared_ptr "не хватает" возможности присваивать обычный указатель,-
//      boost::shared_ptr<obj> s_ptr;
//      s_ptr = (obj*)0;                   // ошибка компиляции
// 

namespace ptr {

// std::auto_ptr<T> без изъянов
template<typename T>
class one
{
    private:
        T* ptr;

        one(const one& a);
    
    public:

            one() : ptr(0) {}
            // не explicit
            template<class Y>
            one(Y* p) : ptr(p) {}

            template<class Y>
            one(one<Y>& a): ptr(a.release()) {}

           ~one() { /*delete ptr*/ boost::checked_delete(ptr); }
            
            // 1 присваивание указателя
       one& operator =(T* p)
            {
                reset(p);
                return *this;
            }
            
            template<class Y>
       one& operator=(one<Y>& a)
            {
                reset(a.release());
                return *this;
            }
            
         T& operator*() const
            {
                return *ptr;
            }
            
         T* operator->() const
            {
                return ptr;
            }
            
         T* get() const { return ptr;}
          
      void  reset(T* p = 0)
            {
                if(p != ptr)
                {
                    delete ptr;
                    ptr = p;
                }
            }
            
         T* release()
            {
                T* tmp = ptr;
                ptr = 0;
                return tmp;
            }

            // 2 для конструкции "if( ptr ) ..."
            typedef T* (one::*ptr_to_bool_type)() const;
   
            operator ptr_to_bool_type() const
            {
                return get() == 0 ? 0: &one::get;
            }
};

// = boost::shared_ptr<T>
template<class T>
class shared: public boost::shared_ptr<T>
{
    typedef boost::shared_ptr<T> my_parent;
    public:
    
        // 0 Конструкторы, как у базы
        // сгенерированные конструктор копирования и присваивания подходят
        shared(): my_parent() {}
    
        // не explicit
        template<class Y>
        shared(Y* p): my_parent(p) {}
    
        // will release p by calling d(p)
        template<class Y, class D> shared(Y* p, D d): my_parent(p, d) {}
    
        template<class Y>
        explicit shared(one<Y> & r): my_parent(r.release()) {}

        template<class Y>
        shared(const shared<Y>& r)
            : my_parent(r) {}

        // 1 присваивание указателя
        shared& operator =(T* ptr)
        {
            this->reset(ptr);
            return *this;
        }

#if 0
        // кострукторы приведения
        template<class Y>
        shared(const shared<Y>& r, boost::detail::static_cast_tag t)
            : my_parent(r, t) {}

        template<class Y>
        shared(const shared<Y>& r, boost::detail::const_cast_tag t)
            : my_parent(r, t) {}

        template<class Y>
        shared(const shared<Y>& r, boost::detail::dynamic_cast_tag t)
            : my_parent(r, t) {}
#endif
};

template<class T, class U> shared<T> static_pointer_cast(shared<U> const & r)
{
    typedef typename shared<T>::element_type E;

    E* p = static_cast<E*>(r.get());
    return shared<T>(r, p);
}

template<class T, class U> shared<T> const_pointer_cast(shared<U> const & r)
{
    typedef typename shared<T>::element_type E;

    E* p = const_cast<E*>(r.get());
    return shared<T>( r, p );
}

template<class T, class U> shared<T> dynamic_pointer_cast(shared<U> const & r)
{
    typedef typename shared<T>::element_type E;

    E* p = dynamic_cast<E*>(r.get());
    return p ? shared<T>(r, p) : shared<T>();
}

// Базовый класс для объектов с ссылками (=> intrusive_ptr)
class base
{
    typedef boost::shared_ptr<bool> shared_bool;
    public:

    long  use_count() const
          {
              return use_count_;
          }

    inline friend 
    void  intrusive_ptr_add_ref(base * p)
          {
              ++p->use_count_;
          }

    inline friend 
    void  intrusive_ptr_release(base * p)
          {
              if(--p->use_count_ == 0) delete p;
          }

          // для weak
    shared_bool  weak_indicator() { return is_exist_; }
          

    protected:

             base(): use_count_(0), is_exist_(new bool(true)) {}
    virtual ~base() 
             {
                 *is_exist_ = false;
             }

    private:

    //boost::detail::atomic_count use_count_;
        int use_count_;
        shared_bool is_exist_;

             base(base const &);
             base & operator=(base const &);
};

//
// ptr::weak_intrusive<>
// В связке с ptr::base дает аналог boost::weak_ptr<>, только 
// для boost::intrusive_ptr<>
// 
template<class T>
class weak_intrusive: public std::pair<T*, boost::shared_ptr<const bool> >
{
    typedef  boost::shared_ptr<const bool> bool_type;
    typedef std::pair<T*, bool_type> my_parent;
    typedef boost::intrusive_ptr<T> intrusive_ptr;

    public:
                    weak_intrusive(T* t = 0)
                    {
                        sync(t);
                    }

                    template<class Y>
                    weak_intrusive(const weak_intrusive<Y> & r): my_parent(r) {}
    
                    template<class Y>
    weak_intrusive& operator = (const weak_intrusive<Y> & r)
                    {
                        this->first  = r.first;
                        this->second = r.second;
                        return *this;
                    }

                    template<class Y>
                    weak_intrusive(const boost::intrusive_ptr<Y> & r)
                    {
                        sync(r.get());
                    }

     intrusive_ptr  lock()    const { return intrusive_ptr( expired() ? 0 : this->first ); }
              bool  expired() const 
                    { 
                        return !(*this->second); 
                    }

    protected:
              void  sync(T* t)
                    {
                        this->first = t;
                        if( t )
                            this->second = this->first->weak_indicator();
                        else
                            this->second = bool_type(new const bool(false));
                    }
};

using boost::static_pointer_cast;
using boost::const_pointer_cast;
using boost::dynamic_pointer_cast;

} // namespace ptr


#endif // __MLIB_PTR_H__
