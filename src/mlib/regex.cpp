
#include "regex.h"

#include <boost/regex.hpp>

namespace re
{

//
// pattern
// 
struct pattern::impl: public boost::regex
{
    typedef boost::regex base_t;

    explicit impl(const char* p, flag_type f): base_t(p, f) {}
};

pattern::pattern(const char* p, flag_type f): pimpl_(new impl(p, f)) {}

pattern::~pattern() {}

//
// match_results
// 
struct match_results::impl: public boost::smatch
{
    typedef boost::smatch base_t;
};

match_results::match_results(): pimpl_(new impl) {}

match_results::~match_results() {}

match_results::size_type match_results::size() const
{
    return pimpl_->size();
}

sub_match match_results::operator[](int sub) const
{
    boost::smatch::const_reference pimpl_res = (*pimpl_)[sub];

    sub_match res;
    res.matched = pimpl_res.matched;
    res.first   = pimpl_res.first;
    res.second  = pimpl_res.second;

    return res;
}

//
// ~ boost::regex_match()
//
bool match(const std::string& s, 
           match_results& m, 
           const pattern& e, 
           match_flag_type flags)
{
    return boost::regex_match(s, *m.pimpl_, *e.pimpl_, flags);  
}

//
// ~ boost::regex_search()
//
bool search(const_iterator beg, const_iterator end, 
            match_results& m, 
            const pattern& e, 
            match_flag_type flags)
{
    return boost::regex_search(beg, end, *m.pimpl_, *e.pimpl_, flags);  
}


} // namespace re


