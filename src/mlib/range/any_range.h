#ifndef __MLIB_RANGE_ANY_RANGE_H__
#define __MLIB_RANGE_ANY_RANGE_H__

#include <mlib/any_iterator.h>

#include <boost/range/iterator_range.hpp>
#include <boost/range/reference.hpp>

namespace fe
{

using boost::range_reference;

template <class Reference>
struct traits
{
    typedef typename boost::remove_reference<Reference>::type Value;
    // in practice, only bidirectional one is needed
#ifdef ADOBE_AIT
    typedef adobe::bidirectional_iter<Value, Reference> iterator;
#else
    typedef IteratorTypeErasure::any_iterator<Value, boost::bidirectional_traversal_tag, Reference> iterator;
#endif

    typedef boost::iterator_range<iterator> range_base;
};

template <class Reference>
class range: public traits<Reference>::range_base
{
    typedef typename traits<Reference>::range_base super_t;
    public:

        range() {}

        template< class Iterator >
        range(const Iterator& Begin, const Iterator& End): super_t(Begin, End) {}

        // only identical type ranges are allowed (to restrict more than one wrapping with any_iterator!)
        range(const range& r): super_t(r) {}
};

//
// make_any
//

template <class Range>
struct range_range
{
    typedef range<typename range_reference<Range>::type> type;
};

template <class Range> 
typename range_range<Range>::type
_make_any(Range& r)
{
    typedef typename range_reference<Range>::type reference;
    return range<reference>(boost::begin(r), boost::end(r));
}

template <class Range>
typename range_range<Range>::type
make_any(Range& r)
{
    return _make_any(r);
}

template <class Range>
typename range_range<const Range>::type
make_any(const Range& r)
{
    return _make_any(r);
}

} // namespace fe


#endif // #ifndef __MLIB_RANGE_ANY_RANGE_H__

