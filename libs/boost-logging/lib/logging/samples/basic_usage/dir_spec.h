#ifndef jt_DIR_SPEC_H
#define jt_DIR_SPEC_H

#include "extensions.h"
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

#include "file_statistics.h"

/** 
    Contains a directory specification 
    - what directories we're to search
    - what type of files we're to seach
*/
struct dir_spec
{
    dir_spec(const std::string & path, const extensions & ext);
    ~dir_spec(void);

    void iterate();

private:
    void iterate_dir(const fs::path & dir);

private:
    std::string m_path;
    extensions m_ext;

    file_statistics m_stats;
    int m_file_count;
};

#endif
