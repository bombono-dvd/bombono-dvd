#ifndef jt_PARSE_FILE_h
#define jt_PARSE_FILE_h

#include "file_statistics.h"
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

file_statistics parse_file(const fs::path& file);

#endif

