//
// mlib/regex.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#include "regex.h"
#include "string.h"

#include <boost/regex/v4/regex.hpp>

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

bool ExtractDouble(double& val, const re::match_results& what, int idx)
{
    std::string p_str = what.str(idx) + "." + (what[idx+2].matched ? what.str(idx+2) : std::string("0"));
    return Str::GetType(val, p_str.c_str());
}

