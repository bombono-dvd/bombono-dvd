//
// mgui/tests/test_mbrowser.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

#ifndef __MGUI_TESTS_MBROWSER_H__
#define __MGUI_TESTS_MBROWSER_H__

#include "mgui_test.h"

#include <mbase/project/tests/menu-utils.h>

namespace Project
{

struct GtkmmDBInit: public DBCleanup
{
    GtkmmDBInit() { InitGtkmm(); }
};

} // namespace Project

#endif // __MGUI_TESTS_MBROWSER_H__

