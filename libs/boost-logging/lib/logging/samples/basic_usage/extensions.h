#ifndef jt_EXTENSIONS_H_
#define jt_EXTENSIONS_H_

#include <vector>
#include <string>
#include <iostream>

struct extensions
{
    typedef std::vector<std::string> array;
    array vals;

    extensions(const std::string & ext = "") {
        if ( !ext.empty())
            vals.push_back(ext);
    }

    extensions & add(const std::string & ext) {
        vals.push_back(ext);
        return *this;
    }
};

inline std::ostream & operator<<(std::ostream & out, const extensions& exts) {
    out << "[";
    for ( extensions::array::const_iterator b = exts.vals.begin(), e = exts.vals.end(); b != e ; ++b) {
        if ( b != exts.vals.begin())
            out << "; ";
        out << "." << (*b);
    }
    out << "]";
    return out;
}

#endif
