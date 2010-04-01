
#include <mbase/_pc_.h>

#include "archieve.h"
#include "archieve-sdk.h"

namespace Project
{

void DoSaveArchieve(xmlpp::Element* root_node, const ArchieveFnr& afnr)
{
    Archieve ar(root_node, false);
    afnr(ar);
}

void DoLoadArchieve(const std::string& fname, const ArchieveFnr& afnr, const char* root_tag)
{
    xmlpp::DomParser parser;
    try
    {
        parser.parse_file(fname);
    }
    catch(const std::exception& err)
    {
        // заменяем, потому что из сообщения непонятно, что произошло
        throw std::runtime_error(std::string(fname) + " is not existed or corrupted");
    }

    xmlpp::Element* root_node = parser.get_document()->get_root_node();
    if( root_node->get_name() != root_tag ) 
        throw std::runtime_error("The file is not " APROGRAM_PRINTABLE_NAME " document.");

    Archieve ar(root_node, true);
    afnr(ar);
}

} // namespace Project

