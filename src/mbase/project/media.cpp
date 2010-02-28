//
// mbase/project/media.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#include <mbase/_pc_.h>

#include "media.h"

#include <mlib/string.h>
#include <mlib/filesystem.h> // MakeAbsolutePath()
#include <mlib/gettext.h>

namespace Project
{

void StorageMD::SetPath(const std::string& path)
{
    mdPath = path;
}

std::string ConvertPathToUtf8(const std::string& path)
{
    return Glib::filename_to_utf8(path).raw();
}

std::string ConvertPathFromUtf8(const std::string& path)
{
    return Glib::filename_from_utf8(path);
}

void StorageMD::MakeByPath(const std::string& path, bool cnv_to_utf8,
                           const std::string& cur_dir)
{
    std::string pth = cnv_to_utf8 ? ConvertPathToUtf8(path)    : path;
    std::string dir = cnv_to_utf8 ? ConvertPathToUtf8(cur_dir) : cur_dir;
    pth = MakeAbsolutePath(pth, dir).string();

    SetPath(pth);
    mdName = get_basename(fs::path(mdPath));
}

std::string MakeAutoName(const std::string& str, int old_sz)
{
    return (str::stream() << str << " " << old_sz+1).str();
}

void VideoMD::AddChapter(ChapterItem chp)
{
    chp->mdName = MakeAutoName(_("Chapter"), chpLst.size());
    chpLst.push_back(chp);
}

bool operator <(const ChapterItem& c1, const ChapterItem& c2)
{
    return c1->chpTime < c2->chpTime;
}

void VideoMD::OrderByTime()
{
    std::stable_sort(chpLst.begin(), chpLst.end(), std::less<ChapterItem>());
}

ChapterItem IsChapter(MediaItem mi)
{
    return ptr::dynamic_pointer_cast<VideoChapterMD>(mi);
}

VideoItem IsVideo(MediaItem mi)
{
    return ptr::dynamic_pointer_cast<VideoMD>(mi);
}

StorageItem IsStorage(MediaItem mi)
{
    return ptr::dynamic_pointer_cast<StorageMD>(mi);
}

std::string GetFilename(StorageMD& smd)
{
    return ConvertPathFromUtf8(smd.GetPath());
}

} // namespace Project

