
#include "format.h"

#ifndef EXT_BOOST

#include <boost/format.hpp> // определение класса 

template class boost::basic_format<char>;

INST_BFRMT_PERCENT(const std::string)
INST_BFRMT_PERCENT(std::string)
INST_BFRMT_PERCENT(char const*)
INST_BFRMT_PERCENT(char* const) // = const char*
INST_BFRMT_PERCENT(char const* const)
INST_BFRMT_PERCENT(unsigned long long) // для uint64_t
INST_BFRMT_PERCENT(const double)

#endif // EXT_BOOST


