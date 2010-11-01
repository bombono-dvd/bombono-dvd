//
// mgui/tests/test_burn.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009-2010 Ilya Murav'jov
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

#include <mgui/tests/_pc_.h>

#include "mgui_test.h"

#include <mgui/author/burn.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/packing.h>

std::string GetTestFNameContents(const std::string& fname)
{
    std::string path = GetTestFileName(fname.c_str());
    return Glib::file_get_contents(path);
}

namespace Author
{

BOOST_AUTO_TEST_CASE( TestProbeDevices )
{
    return;
    InitGtkmm();

    BurnData& bd = GetInitedBD();

    Gtk::Window win;
    Gtk::VBox& vbox = Add(win, NewManaged<Gtk::VBox>());
    PackStart(vbox, bd.DVDDevices());
    PackStart(vbox, bd.SpeedBtn());

    RunWindow(win);
}

static DVDInfo ParseDVDInfoExample(bool is_good, const std::string& fname)
{
    return ParseDVDInfo(is_good, GetTestFNameContents("dvd+rw-mediainfo/" + fname));
}

static DVDType ParseDVDTypeExample(bool is_good, const std::string& fname)
{
    return ParseDVDInfoExample(is_good, fname).typ;
}

// для BOOST_CHECK_EQUAL
std::ostream& operator << (std::ostream& os, const DVDInfo& inf)
{
    os << "{ " << (int)inf.typ << ", " << inf.name << ", " << inf.isBlank << " }";
    return os;
}

BOOST_AUTO_TEST_CASE( TestDVDDisc )
{
    InitGtkmm();

    BOOST_CHECK_EQUAL( dvdERROR,         ParseDVDTypeExample(false, "01_not_drive.txt") );
    BOOST_CHECK_EQUAL( dvdCD_DRIVE_ONLY, ParseDVDTypeExample(false, "02_ide_cdr10.txt") );
    BOOST_CHECK_EQUAL( dvdCD_DISC,       ParseDVDTypeExample(false, "03_non-DVD.txt")   );
    BOOST_CHECK_EQUAL( dvdEMPTY_DRIVE,   ParseDVDTypeExample(false, "04_empty_drive.txt") );

    DVDInfo inf(dvdR);
    inf.name = "DVD+R";
    BOOST_CHECK_EQUAL( inf, ParseDVDInfoExample(true, "05_dvd+r_new.txt") );
    inf.typ  = dvdRW;
    inf.name = "DVD-RW";
    BOOST_CHECK_EQUAL( inf, ParseDVDInfoExample(true, "06_dvd-rw_new.txt") );
    inf.typ  = dvdR;
    inf.name = "DVD+R";
    BOOST_CHECK_EQUAL( inf, ParseDVDInfoExample(true, "07_dvd_dl_new.txt") );

    inf.typ  = dvdOTHER;
    inf.name = "DVD-ROM";
    BOOST_CHECK_EQUAL( inf, ParseDVDInfoExample(true, "08_dvd-rom.txt") );
    inf.name = "DVD-RAM";
    BOOST_CHECK_EQUAL( inf, ParseDVDInfoExample(true, "09_dvd-ram_full.txt") );

    // isBlank
    inf.typ     = dvdR;
    inf.name    = "DVD+R";
    inf.isBlank = false;
    BOOST_CHECK_EQUAL( inf, ParseDVDInfoExample(true, "10_dvd+r_1write.txt") );
    inf.typ     = dvdRW;
    inf.name    = "DVD-RW";
    inf.isBlank = true;
    BOOST_CHECK_EQUAL( inf, ParseDVDInfoExample(true, "11_dvd-rw_formatted.txt") );
    inf.isBlank = false;
    BOOST_CHECK_EQUAL( inf, ParseDVDInfoExample(true, "12_dvd-rw_full.txt") );
}

} // namespace Author
