//
// mgui/tests/test_mbrowser.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2010 Ilya Murav'jov
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

#include "test_mbrowser.h"
#include "mgui_test.h"

#include <mgui/trackwindow.h>
#include <mgui/project/mb-actions.h>
#include <mgui/project/handler.h>
#include <mgui/project/mconstructor.h>
#include <mgui/sdk/window.h>

namespace Project
{

//
// TestMBrowserActions
//

static int GetMediasSize(RefPtr<MediaStore> ms)
{
    return Size(ms);
}

static void CheckMediaPaths(RefPtr<MediaStore> ms, int have_cnt = -1)
{
    int i = 0;
    for( MediaStore::iterator itr = ms->children().begin(), end = ms->children().end(); 
         itr != end; ++itr, ++i )
    {
        Gtk::TreePath path = ms->get_path(itr);
        MediaItem mi = ms->GetMedia(itr);
        BOOST_CHECK_MESSAGE( mi,  "Null Media in browser for path: " << path.to_string() );

        StorageItem si = IsStorage(mi);
        BOOST_CHECK_MESSAGE( si, "Not Media in browser for path, name: " << path.to_string() 
                             << " , " << mi->mdName);

        Gtk::TreePath l_path = GetBrowserPath(si);
        BOOST_CHECK_MESSAGE( path == l_path, "Media is not synced for real path != local path: " << path.to_string() 
                             << " != " << l_path.to_string() );
    }
    if( have_cnt != -1 )
    {
        //BOOST_CHECK_EQUAL( GetMediasSize(), have_cnt );
        BOOST_CHECK_EQUAL( i, have_cnt );
    }
}

static void CopyRow(Gtk::TreeRow& dst, Gtk::TreeRow& src, RefPtr<MediaStore> ms)
{
    dst.set_value(ms->columns.media,     src.get_value(ms->columns.media));
    dst.set_value(ms->columns.thumbnail, src.get_value(ms->columns.thumbnail));
}

// (переносим только картинку => главы переносить не надо)
void CheckEmulateDrag(int from_pos, int to_pos, RefPtr<MediaStore> ms)
{
    int cnt = GetMediasSize(ms);
    Gtk::TreeRow row = ms->children()[from_pos];
    Gtk::TreeRow new_row = *ms->insert(ms->children()[to_pos]);
    CopyRow(new_row, row, ms);
    if( from_pos < to_pos )
        SyncOnDragReceived(ms->get_path(new_row), ms->get_path(row), ms);
    else
    {
        Gtk::TreeIter itr = row;
        SyncOnDragReceived(ms->get_path(new_row), ms->get_path(--itr), ms);
    }
    ms->erase(row);
    CheckMediaPaths(ms, cnt);
    
}

static void ChangeChapter(VideoItem vi, int from, int to)
{
    VideoMD::Itr ci0 = vi->List().begin() + from;
    ChapterItem ci = *ci0;
    ci->GetData<int>("ChapterMoveIndex") = from;

    VideoMD::Itr ci1 = vi->List().begin() + to;
    //VideoMD::Itr last = from < to ? ci1 : ci0 ;
    if( from < to )
        std::rotate(ci0, ci0+1, ci1+1);
    else
        std::rotate(ci1, ci0, ci0+1);

    InvokeOnChange(ci);
}

BOOST_AUTO_TEST_CASE( TestMBrowserActions )
{
    InitGtkmm();
    AMediasLoader adl(GetTestFileName("medias.xml"));

    RefPtr<MediaStore> ms = CreateMediaStore();
    // * проверка путей вначале
    CheckMediaPaths(ms, 3);

    // * добавление медиа
    std::string tmp_str = GetTestFileName("colors.png");
    StorageItem si = CreateMedia(tmp_str.c_str(), tmp_str);
    BOOST_CHECK( si );
    // перед AcerTX.png
    PublishMedia(ms->insert(ms->get_iter("1")), ms, si);

    Gtk::TreeIter itr = ms->get_iter("1");
    BOOST_CHECK_EQUAL( ms->GetMedia(itr)->mdName, "colors" );
    CheckMediaPaths(ms, 4);

    // * удаление медиа
    DeleteBrowserMedia(si, itr, ms);
    itr = ms->get_iter("1");
    BOOST_CHECK_EQUAL( ms->GetMedia(itr)->mdName, "AcerTX" );
    CheckMediaPaths(ms, 3);

    // * перемещение медиа (эмуляция ДнД)
    CheckEmulateDrag(1, 0, ms);
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("0"))->mdName, "AcerTX" );

    CheckEmulateDrag(0, 2, ms);
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("1"))->mdName, "AcerTX" );

    // * изменение главы
    VideoItem vi = IsVideo(ms->GetMedia(ms->get_iter("0")));
    BOOST_CHECK( vi ); // главы 1, 2, 3

    vi->List()[0]->chpTime = 25;
    ChangeChapter(vi, 0, 1);
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("0:0"))->mdName, "Chapter 2" );
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("0:1"))->mdName, "Chapter 1" );

    vi->List()[2]->chpTime = 5;
    ChangeChapter(vi, 2, 0);
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("0:0"))->mdName, "Chapter 3" );
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("0:1"))->mdName, "Chapter 2" );
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("0:2"))->mdName, "Chapter 1" );

    // * удаление главы
    DeleteMedia(vi->List()[0]);
    BOOST_CHECK_EQUAL( (int)vi->List().size(), 2 );
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("0:0"))->mdName, "Chapter 2" );

    // * добавление главы (в конец)
    ChapterItem ci = VideoChapterMD::CreateChapter(vi.get(), 30);
    Project::InvokeOnInsert(ci);
    BOOST_CHECK_EQUAL( (int)vi->List().size(), 3 );
    BOOST_CHECK_EQUAL( ms->GetMedia(ms->get_iter("0:2")), ci );
}

//
// 
// 

void TestMBrowserImpl(MediasWindowPacker pack_fnr)
{
    InitGtkmm();
    AMediasLoader adl(GetTestFileName("medias.xml"));

    // 1
    RefPtr<MediaStore> ms = CreateMediaStore();
    Gtk::Window win;
    win.set_title("MediaBrowser");
    SetAppWindowSize(win, 1000); // 800);
    PackMediasWindow(win, ms, pack_fnr);

    RunWindow(win);
}

BOOST_AUTO_TEST_CASE( TestMBrowser )
{
    //TestMBrowserImpl(&Project::PackMediaBrowser);
}

BOOST_AUTO_TEST_CASE( TestFullMBrowser )
{
    //TestMBrowserImpl(&Project::PackFullMBrowser);
}

} // namespace Project

