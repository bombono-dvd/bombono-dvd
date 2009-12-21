//
// mgui/sdk/browser.cpp
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

#include <mgui/_pc_.h>

#include <mlib/ptr.h> 
#include <mgui/img_utils.h>

//#include "magick_pixbuf.h"
#include "browser.h"

#if 0
RefPtr<Gdk::Pixbuf> GetThumbnail(const char* fpath, int thumb_wdh)
{
    Magick::Image* img = NULL;
    bool is_movie;
    ptr::one<Comp::Media> md = CmdOptions::CreateMedia(fpath, is_movie);
    if( md )
    {
        if( is_movie )
        {
            Comp::MovieMedia* mm = static_cast<Comp::MovieMedia*>(md.get());
            if( mm->Begin() && mm->NextFrame() )
            {
                mm->MakeImage();
                img = &mm->GetImage();
            }
        }
        else
            img = &md->GetImage();
    }

    const Point thumb_sz = Project::Calc4_3Size(thumb_wdh);
    RefPtr<Gdk::Pixbuf> pix = CreatePixbuf(thumb_sz);
    if( img )
    {
        AlignToPixbuf( *img );
        RefPtr<Gdk::Pixbuf> orig_pix = GetAsPixbuf(*img);

        RGBA::Scale(pix, orig_pix, Rect0Sz(thumb_sz));
    }
    else
        pix->fill(0x000000ff); // черный
    return pix;
}
#endif // #if 0

bool OpenMediaFile(std::string& fname, Gtk::Window& par_win)
{
    Gtk::FileChooserDialog dialog("Open Media", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(par_win);

    // кнопки
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    // картинки "png", "jpg", "jpeg", "bmp"
    Gtk::FileFilter filter_img;
    filter_img.set_name("Still image files");
    filter_img.add_pattern("*.png");
    filter_img.add_pattern("*.jpg");
    filter_img.add_pattern("*.jpeg");
    filter_img.add_pattern("*.bmp");
    dialog.add_filter(filter_img);

    // видео
    Gtk::FileFilter filter_mpeg;
    filter_mpeg.set_name("MPEG1/2 files");
    filter_mpeg.add_pattern("*.m2v");
    filter_mpeg.add_pattern("*.mpeg");
    filter_mpeg.add_pattern("*.mpg");
    filter_mpeg.add_pattern("*.dva");
    dialog.add_filter(filter_mpeg);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    // :TODO: - нервирует, но не опасно
    // (<unknown>:15478): libgnomevfs-WARNING **: Failed to activate daemon: 
    //      The name org.gnome.GnomeVFS.Daemon was not provided by any .service files
    if( Gtk::RESPONSE_OK == dialog.run() )
    {
        fname = dialog.get_filename();
        return true;
    }
    return false;
}

