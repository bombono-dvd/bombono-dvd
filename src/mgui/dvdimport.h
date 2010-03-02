//
// mgui/dvdimport.h
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

#ifndef __MGUI_DVDIMPORT_H__
#define __MGUI_DVDIMPORT_H__

#include "mguiconst.h"
#include "timer.h"

#include <mdemux/dvdread.h>
#include <mdemux/player.h>

#include <mlib/patterns.h>

namespace DVD {

struct ImportData;
void ConstructImporter(ImportData& id);

enum ImportPage
{
    ipNONE_PAGE     = -1,
    ipCHOOSE_SOURCE = 0, // выбор источника импорта
    ipSELECT_VOBS,       // выбор импортируемых VOB + предпросмотр
    ipCHOOSE_DEST,       // куда сохранить
    ipIMPORT_PROC,       // процесс импорта
    ipEND,               // конец импорта

    ipPAGE_NUM,          // число страниц
};

void InitPreview(ImportData& id, AspectFormat af);
inline void InitDefPreview(ImportData& id) { InitPreview(id, af4_3); }

struct ImportData
{
            Gtk::Assistant  ast;
                      bool  addToProject;
                ImportPage  curPage; // текущая страница
                Gtk::Label  errLbl;

    Gtk::FileChooserWidget  srcChooser;
                
                 ReaderPtr  reader;
                    VobArr  dvdVobs;
     RefPtr<Gtk::ListStore> vobList;

                            // для расчета изображений
           Mpeg::FwdPlayer  thumbPlyr;
                       int  numToThumb; // номер для получения миниатюры plyr'ом
               EventSource  thumbIdler;

                            // для предпросмотра
           Gtk::Adjustment  previewAdj;
                Gtk::Image  previewImg;
                            // в принципе можно одним плейером обойтись, но тогда
                            // придется вводить состояние - чем из 2 дел в idle заниматься
           Mpeg::FwdPlayer  previewPlyr;
               EventSource  previewIdler;

               std::string  destPath;

                Gtk::Label  prgLabel;
          Gtk::ProgressBar  prgBar;
                      bool  isBreak; // пользователь хочет прерваться

                Gtk::Label  finalMsg;
  
    ImportData();
   ~ImportData() { CloseIdlers(); }

    void
    CloseIdlers()
    {
        thumbIdler.Disconnect();
        InitDefPreview(*this);
    }
};

} // namespace DVD

#endif // #ifndef __MGUI_DVDIMPORT_H__


