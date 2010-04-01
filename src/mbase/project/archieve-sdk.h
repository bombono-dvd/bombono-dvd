#ifndef __MBASE_PROJECT_ARCHIEVE_SDK_H__
#define __MBASE_PROJECT_ARCHIEVE_SDK_H__

#include "archieve-fwd.h"

namespace Project
{

void DoLoadArchieve(const std::string& fname, const ArchieveFnr& afnr, const char* root_tag);
void DoSaveArchieve(xmlpp::Element* root_node, const ArchieveFnr& afnr);

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_ARCHIEVE_SDK_H__


