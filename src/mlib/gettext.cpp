
#include "gettext.h"
#include "tech.h"

const char* _context_gettext_(const char* msgid, size_t msgid_offset)
{
    ASSERT( msgid_offset > 0 );
    const char* trans = gettext(msgid);

    if( trans == msgid )
        // без перевода
        trans = msgid + msgid_offset;
    return trans;
}

boost::format BF_(const char* str)
{
    return boost::format(gettext(str));
}
boost::format BF_(const std::string& str)
{
    return boost::format(gettext(str.c_str()));
}

std::string _dots_(const char* str)
{
    return gettext(str) + std::string("...");
}

std::string _semicolon_(const char* str)
{
    return gettext(str) + std::string(":");
}

