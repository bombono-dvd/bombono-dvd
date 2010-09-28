
#include <mgui/_pc_.h>

#include "test_author.h"

#include <mgui/project/mconstructor.h>
#include <mgui/win_utils.h>

#include <boost/test/minimal.hpp>

int test_main(int /*argc*/, char */*argv*/[])
{
    InitGtkmm();
    std::string prj_fname = Project::TestProjectPath();
    Project::RunConstructor(prj_fname, false);

    return 0;
}
