//  Enumerate adapter for Boost.Range & Foreach
// 
//  Copyright (c) 2010 Ilya Murav'jov
// 
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __MLIB_RANGE_ENUMERATE_H__
#define __MLIB_RANGE_ENUMERATE_H__

#include <mlib/range/adaptor/argument_fwd.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/iterator/iterator_traits.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

//#include <utility> // std::pair
#include <boost/tuple/tuple.hpp>

namespace boost
{
    namespace adaptors
    {
        struct enumerated
        {
            explicit enumerated(std::size_t x = std::size_t()) : val(x) {}
            std::size_t val;
        };
    }

    // Why yet another "pair" class:
    // - std::pair can't store references
    // - no need for typing for index type (default to "int"); this is extremely useful
    //   in BOOST_FOREACH() expressions that have pitfalls with commas 
    //   ( see http://www.boost.org/doc/libs/1_44_0/doc/html/foreach/pitfalls.html )
    // - meaningful access functions index(), value()
    template<class T, class Indexable = ptrdiff_t>
    class index_value: public tuple<Indexable, T>
    {
        public:
        typedef tuple<Indexable, T> super_t;

        template <int N>
        struct iv_types
        {
            typedef typename tuples::element<N, super_t>::type n_type;

            typedef typename tuples::access_traits<n_type>::non_const_type non_const_type;
            typedef typename tuples::access_traits<n_type>::const_type const_type;
        };

        index_value() {}

        index_value(typename tuples::access_traits<Indexable>::parameter_type t0,
                    typename tuples::access_traits<T>::parameter_type t1)
        : super_t(t0, t1) {}

        // member functions index(), value() (non-const and const)
        typename iv_types<0>::non_const_type
        index() { return boost::tuples::get<0>(*this); }

        typename iv_types<0>::const_type
        index() const { return boost::tuples::get<0>(*this); }

        typename iv_types<1>::non_const_type
        value() { return boost::tuples::get<1>(*this); }

        typename iv_types<1>::const_type
        value() const { return boost::tuples::get<1>(*this); }
    };

    namespace range_detail
    {
        template<typename Iter>
        class enumerated_iterator;

        // like zip_iterator!
        template<typename Iterator>
        struct enumerated_iterator_base
        {
         private:
            typedef typename iterator_reference<Iterator>::type iterator_reference;
            typedef typename iterator_difference<Iterator>::type difference_type;
            typedef index_value<iterator_reference, difference_type> reference;

            typedef reference value_type;
            typedef typename iterator_category<Iterator>::type iterator_category;
         public:

            typedef iterator_adaptor<
                enumerated_iterator<Iterator>,
                Iterator,
                value_type,  
                use_default, // let it be the same
                reference,
                difference_type
            > type;
        };

        template<typename Iter>
        class enumerated_iterator: public enumerated_iterator_base<Iter>::type
        {
        private:
            typedef BOOST_DEDUCED_TYPENAME enumerated_iterator_base<Iter>::type super_t;

            typedef BOOST_DEDUCED_TYPENAME super_t::difference_type index_type;

            index_type m_index;

        public:
            typedef BOOST_DEDUCED_TYPENAME super_t::reference reference;

            explicit enumerated_iterator( Iter i, index_type index )
            : super_t(i), m_index(index)
            {
                BOOST_ASSERT( m_index >= 0 && "Indexed Iterator out of bounds" );
            }

         private:
            friend class boost::iterator_core_access;

            reference dereference() const
            {
                return reference(m_index, *this->base()); 
            }

            void increment()
            {
                ++m_index;
                ++(this->base_reference());
            }

            void decrement()
            {
                BOOST_ASSERT( m_index > 0 && "enumerated Iterator out of bounds" );
                --m_index;
                --(this->base_reference());
            }

            void advance( index_type n )
            {
                m_index += n;
                BOOST_ASSERT( m_index >= 0 && "enumerated Iterator out of bounds" );
                this->base_reference() += n;
            }
        };

        template< class Rng >
        struct enumerated_range :
            iterator_range< enumerated_iterator<BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type> >
        {
        private:
            typedef enumerated_iterator<BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type>
                iter_type;
            typedef iterator_range<iter_type>
                base;
        public:
            template< class Index >
            enumerated_range( Index i, Rng& r )
              : base( iter_type(boost::begin(r), i), iter_type(boost::end(r),i) )
            { }
        };

    } // 'range_detail'
    using range_detail::enumerated_range;

    namespace adaptors
    {
        template< class SinglePassRange >
        inline enumerated_range<SinglePassRange>
        operator|( SinglePassRange& r,
                   const enumerated& f )
        {
            return enumerated_range<SinglePassRange>( f.val, r );
        }

        template< class SinglePassRange >
        inline enumerated_range<const SinglePassRange>
        operator|( const SinglePassRange& r,
                   const enumerated& f )
        {
            return enumerated_range<const SinglePassRange>( f.val, r );
        }

        template<class SinglePassRange, class Index>
        inline enumerated_range<SinglePassRange>
        enumerate(SinglePassRange& rng, Index index_value = Index())
        {
            return enumerated_range<SinglePassRange>(index_value, rng);
        }

        template<class SinglePassRange, class Index>
        inline enumerated_range<const SinglePassRange>
        enumerate(const SinglePassRange& rng, Index index_value = Index())
        {
            return enumerated_range<const SinglePassRange>(index_value, rng);
        }
    } // 'adaptors'

}

namespace fe {
using boost::index_value;
using boost::adaptors::enumerate;
using boost::adaptors::enumerated;
}

#endif // __MLIB_RANGE_ENUMERATE_H__

