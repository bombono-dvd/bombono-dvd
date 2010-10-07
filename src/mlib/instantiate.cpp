
#include "format.h"

#ifdef EXTERNAL_INSTANTIATE

#include <boost/format.hpp> // определение класса 

template class boost::basic_format<char>;

INST_BFRMT_PERCENT(const std::string)
INST_BFRMT_PERCENT(std::string)
INST_BFRMT_PERCENT(char const*)
INST_BFRMT_PERCENT(char* const) // = const char*
INST_BFRMT_PERCENT(char const* const)
INST_BFRMT_PERCENT(unsigned long long) // для uint64_t
INST_BFRMT_PERCENT(const double)

// :KLUDGE: в зависимости от режима оптимизации компиляции инстанцировать приходится разные объекты,
// в зависимости от того, inline'ит компилятор operator%() и feed() или нет
// Поэтому оставим режим инстанцирования только для разработки (такая же причина, что и "быстрая" сборка)

//#define INST_BFRMT_FEED(Type) template boost::format& boost::io::detail::feed<char, std::char_traits<char>, std::allocator<char>, Type&>(boost::format& self, Type& x);
//
//INST_BFRMT_FEED(char const* const)
//INST_BFRMT_FEED(double const)
//INST_BFRMT_FEED(int const)
//INST_BFRMT_FEED(int)
//INST_BFRMT_FEED(std::string)

#endif // EXTERNAL_INSTANTIATE


