//
// mgui/author/burn.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009 Ilya Murav'jov
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

#ifndef __MGUI_AUTHOR_BURN_H__
#define __MGUI_AUTHOR_BURN_H__

#include "gb_devices.h"

#include <mgui/mguiconst.h>

#include <mlib/patterns.h> // Singleton<>
#include <mlib/ptr.h>

namespace Author
{

struct Burner
{
    std::string  devName;
    std::string  devPath;

        Burner(const std::string& dev_name, const std::string& dev_path)
        : devName(dev_name), devPath(dev_path) {}
};

struct BurnData: public Singleton<BurnData>
{
    typedef std::vector<Burner> BurnerArray;

                   BurnerArray  brnArr;
 ptr::shared<Gtk::ComboBoxText> dvdDevices; // приводы DVD

     ptr::shared<Gtk::ComboBox> speedBtn;   // список скоростей для болванки
        ptr::shared<Gtk::Entry> label;      // метка диска

     Gtk::ComboBoxText& DVDDevices() { return *dvdDevices; }
         Gtk::ComboBox& SpeedBtn()   { return *speedBtn; }
            Gtk::Entry& Label()      { return *label; }
};

inline BurnData& GetBD()
{
    return BurnData::Instance();
}

BurnData& GetInitedBD();

bool IsBurnerSetup(std::string& dev_path);
double GetBurnerSpeed();

enum DVDType
{
    // явные ошибки
    dvdERROR,         // все остальные ошибки
    dvdCD_DRIVE_ONLY, // только для CD
    dvdCD_DISC,       // это CD-диск
    dvdEMPTY_DRIVE,   // в приводе ничего нет

    // подробнее о DVD-диске
    dvdR,             // DVD-/+R  (DL)
    dvdRW,            // DVD-/+RW (DL)
    dvdOTHER,         // все остальные (не подходят для записи DVD-Video)
};

struct DVDInfo
{
        DVDType  typ;
    std::string  name;    // "DVD+R", ...
           bool  isBlank; // чистый ли (только для R и RW определяется)

           DVDInfo(DVDType t): typ(t), isBlank(true) {}

      bool operator ==(const DVDInfo& inf) const
      {
          return (typ == inf.typ) && (name == inf.name) && (isBlank == inf.isBlank);
      }
};

DVDInfo ParseDVDInfo(bool is_good, const std::string& out_info);

} // namespace Author

#endif // #ifndef __MGUI_AUTHOR_BURN_H__


