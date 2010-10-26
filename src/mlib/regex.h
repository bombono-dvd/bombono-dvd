
#ifndef __MLIB_REGEX_H__
#define __MLIB_REGEX_H__

// :TRICKY: хорошо бы автор не поменял v4 на другой адрес
#include <boost/regex/config.hpp>
#include <boost/regex/v4/regbase.hpp>
#include <boost/regex/v4/match_flags.hpp>

#include <mlib/ptr.h>

#include <string>
#include <utility> // std::pair

namespace re
{

//
// Boost.Regex очень тяжелый при разработке; любое использование
// в исходнике:
// - увеличивает размер объектника на 600Kb
// - увеличивает на 0.6 секунды время компиляции
// 
// Вроде бы надо добавить ошибку в базу Boost'а, но вряд ли займутся,-
// много кода "перекладывать", а результат незаметен. Потому лучше "обернуть"
// и просто пользоваться без проблем. 
//


namespace constants = boost::regex_constants;

//
// forward decl
//
class pattern;
class match_results;

typedef std::string::const_iterator const_iterator;
typedef std::string string_type;
typedef constants::match_flags match_flag_type;


bool match(const std::string& s, 
           match_results& m, 
           const pattern& e, 
           match_flag_type flags = constants::match_default);

bool search(const_iterator beg, const_iterator end, 
            match_results& m, 
            const pattern& e, 
            match_flag_type flags = constants::match_default);

//
// ~ boost::regex = boost::basic_regex<char>
//
class pattern: public boost::regbase
{
    public:
    typedef constants::syntax_option_type flag_type;

    explicit 
    pattern(const char* p, flag_type f = constants::normal);

   ~pattern();

    private:
        class impl;
        ptr::one<impl> pimpl_;

    friend bool match(const std::string& s, match_results& m, 
                      const pattern& e, match_flag_type flags);
    friend bool search(const_iterator beg, const_iterator end, match_results& m, 
                       const pattern& e, match_flag_type flags);
};

//
// ~ boost::sub_match<const_iterator>
//
struct sub_match: public std::pair<const_iterator, const_iterator>
{
    bool matched;

    sub_match(): matched(false) {}

    string_type str() const
    {
        return matched ? string_type(first, second) : string_type() ;
    }
};

//
// ~ boost::smatch = boost::match_results<>
//
class match_results
{
    public:
    typedef size_t size_type;

    explicit 
    match_results();

   ~match_results();

    size_type size() const;
    sub_match operator[](int sub) const;

    string_type str(int sub = 0) const
    {
        return (*this)[sub].str();
    }

    // :TODO: по требованию
    //string_type str(const std::string& sub) const;
    //string_type str(const char* sub) const;
    //sub_match operator[](const std::string& sub) const;
    //sub_match operator[](const char* sub) const;

    private:
        class impl;
        ptr::one<impl> pimpl_;

    friend bool match(const std::string& s, match_results& m, 
                      const pattern& e, match_flag_type flags);
    friend bool search(const_iterator beg, const_iterator end, match_results& m, 
                       const pattern& e, match_flag_type flags);
};

//
// ~ boost::regex_match()
//
inline 
bool match(const std::string& s, 
           const pattern& e, 
           match_flag_type flags = constants::match_default)
{
    match_results m;
    return match(s, m, e, flags | constants::match_any);
}

//
// ~ boost::regex_search()
//
inline
bool search(const std::string& s, 
            match_results& m, 
            const pattern& e, 
            match_flag_type flags = constants::match_default)
{
    return search(s.begin(), s.end(), m, e, flags);
}


inline 
bool search(const std::string& s, 
            const pattern& e, 
            match_flag_type flags = constants::match_default)
{
    match_results m;
    return search(s, m, e, flags | constants::match_any);
}

} // namespace re

#define RG_BW "\\<"           // начало слова
#define RG_EW "\\>"           // конец  слова
#define RG_SPS "[[:space:]]*" // пробелы
#define RG_NUM "([0-9]+)"     // число
#define RG_CMD_BEG RG_BW // "^"RG_SPS  // начало команды

#endif // #ifndef __MLIB_REGEX_H__

