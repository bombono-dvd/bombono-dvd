//
// mgui/timeline/service.h
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

#ifndef __MGUI_TIMELINE_SERVICE_H__
#define __MGUI_TIMELINE_SERVICE_H__

#include "layout.h"

namespace Timeline
{

class TLContext
{
    public:

                    TLContext(CR::RefPtr<CR::Context> cr_, TrackLayout& trk_lay);
    virtual        ~TLContext() {}

        CR::RefPtr<CR::Context> cont;
                   TrackLayout& trkLay;
                         Point  winSz; // размеры окна
                         Point  shift; // смещение дорожки
};

class Service: public TLContext
{
    typedef TLContext MyParent;
    public:

                    Service(CR::RefPtr<CR::Context> cr_, TrackLayout& trk_lay)
                        : MyParent(cr_, trk_lay) {}
                    Service(const TLContext& tl_cont): MyParent(tl_cont) {}

                    // выполнить работу
              void  FormLayout();

    protected:

                                //
                                // Закэшированные данные
                                //
                                // положение шкалы, sclRct.lft == 0
                          Rect  sclRct; 

    virtual   void  Process() = 0;

              void  FormBigLabel();
    virtual   void  ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos) = 0;

                    // создать("отрисовать") шкалу
              void  FormScale();
    virtual   void  ProcessScale() = 0;

              void  FormHandlers();

              void  FormDVDMarks();
    virtual   void  ProcessDVDMark(int idx, const Point& pos) = 0;

                    // создать("отрисовать") указатель
              void  FormPointer();
    virtual   void  ProcessPointer(const Point& pos) = 0;

              void  FormContent();
    virtual   void  ProcessContent() = 0;

              void  FormTrack();
    virtual   void  ProcessTrack() = 0;

              void  FormDVDThumbnails();
    virtual   void  ProcessDVDThumbnail(int idx, const Rect& lct) = 0;

              void  FormTrackName();
    virtual   void  ProcessTrackName(RefPtr<Pango::Layout> lay, const Rect& lct) = 0;

int  GetSclLevel();
DPoint CalcTextSize(RefPtr<Pango::Layout> lay);

// true - вся область дорожки ("желоб")
// false - видимая часть дорожки
Rect  GetTrackLocation(bool is_trough);

void PaintPointer(const Point& pos, bool form_only);
};

// отрисовка TrackLayout
class RenderSvc: public Service
{
    public:
                    RenderSvc(CR::RefPtr<CR::Context> cr_, TrackLayout& trk_lay, const DRect& drw_rct)
                        : Service(cr_, trk_lay), drwRct(drw_rct) {}

    protected:

     const DRect  drwRct;
            
    virtual   void  Process();
    virtual   void  ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos);
    virtual   void  ProcessScale();
    virtual   void  ProcessDVDMark(int idx, const Point& pos);
    virtual   void  ProcessPointer(const Point& pos);
    virtual   void  ProcessContent();
    virtual   void  ProcessTrack();
    virtual   void  ProcessDVDThumbnail(int idx, const Rect& lct);
    virtual   void  ProcessTrackName(RefPtr<Pango::Layout> lay, const Rect& lct);
};

inline CR::RefPtr<CR::Context> CreateImgContext(int wdh, int hgt)
{
    CR::RefPtr<CR::ImageSurface> sur = CR::ImageSurface::create(Cairo::FORMAT_ARGB32, wdh, hgt);
    return CR::Context::create(sur);
}

inline CR::RefPtr<CR::Context> CreateCalcContext()
{
    return CreateImgContext(1, 1);
}

// общий класс для работы с "интерактивными" областями
class CalcSvc: public Service
{
    typedef Service MyParent;
    public:
                    CalcSvc(TrackLayout& trk_lay)
                        : MyParent(CreateCalcContext(), trk_lay) {}
                    CalcSvc(const TLContext& tl_cont): MyParent(tl_cont) {}

    protected:

    virtual   void  ProcessContent() {}
    virtual   void  ProcessTrack() {}
    virtual   void  ProcessDVDThumbnail(int, const Rect&) {}
    virtual   void  ProcessTrackName(RefPtr<Pango::Layout>, const Rect&) {}
};

class SimpleCalcSvc: public CalcSvc
{
    typedef CalcSvc MyParent;
    public:

                    SimpleCalcSvc(TrackLayout& trk_lay)
                        : MyParent(trk_lay) {}

    protected:

    virtual   void  ProcessBigLabel(RefPtr<Pango::Layout> /*lay*/, const Point& /*pos*/) {}
    virtual   void  ProcessScale() {}
    virtual   void  ProcessDVDMark(int /*idx*/, const Point& /*pos*/) {}
    virtual   void  ProcessPointer(const Point& /*pos*/) {}
};

// 
// HookSvc
// 

class HookAction: public TLContext
{
    typedef TLContext MyParent;
    public:

                      HookAction(TrackLayout& trk_lay, const DPoint& lct_)
                        : MyParent(CreateCalcContext(), trk_lay), lct(lct_) {}
        virtual      ~HookAction() {}

        const DPoint& Location()  { return lct; }

        virtual void  Process() = 0;

        virtual void  AtBigLabel() {}
        virtual void  AtScale() {}
        virtual void  AtDVDMark(int /*idx*/) {}
        virtual void  AtPointer() {}

    protected:

                   DPoint  lct;
};

// действие по левой кнопке мыши
class LeftMouseHook: public HookAction
{
    typedef HookAction MyParent;
    public:

                        LeftMouseHook(TrackLayout& trk_lay, const DPoint& lct_)
                            : MyParent(trk_lay, lct_), actStt(0) {}

          virtual void  Process();
          virtual void  AtBigLabel();
          virtual void  AtScale();
          virtual void  AtDVDMark(int idx);
          virtual void  AtPointer();

    protected:

            ActionTL* actStt;
};

// определение действия по координатам
class HookSvc: public CalcSvc
{
    typedef CalcSvc MyParent;
    typedef ptr::shared<HookAction> PAction;
    public:

                    HookSvc(PAction actn): MyParent(*actn), pAction(actn) {}

           PAction  Action() { return pAction; }

    protected:

               PAction  pAction;

    virtual   void  Process();
    virtual   void  ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos);
    virtual   void  ProcessScale();
    virtual   void  ProcessDVDMark(int idx, const Point& pos);
    virtual   void  ProcessPointer(const Point& pos);

const DPoint& PhisPos() { return pAction->Location(); }
};

// общий класс для покрытия объектов прямоугольниками, для перерисовки
class CoverSvc: public CalcSvc
{
    typedef CalcSvc MyParent;
    public:
                    CoverSvc(TrackLayout& trk_lay): MyParent(trk_lay) {}

    protected:

    virtual   void  ProcessPointer(const Point&) {}
    virtual   void  ProcessDVDMark(int, const Point&) {}
    virtual   void  ProcessBigLabel(RefPtr<Pango::Layout>, const Point&) {}

              void  AddRect(const DRect& user_rct);
};

// покрытие указателя (+ линия + большая метка)
class PointerCoverSvc: public CoverSvc
{
    typedef CoverSvc MyParent;
    public:

                    PointerCoverSvc(TrackLayout& trk_lay): MyParent(trk_lay) {}

    protected:

    virtual   void  Process();
    virtual   void  ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos);
    virtual   void  ProcessScale();
    virtual   void  ProcessPointer(const Point& pos);
};

class DVDLabelCoverSvc: public CoverSvc
{
    typedef CoverSvc MyParent;
    public:

                    DVDLabelCoverSvc(TrackLayout& trk_lay, int idx): MyParent(trk_lay), dvdIdx(idx) {}
              void  SetIndex(int idx) { dvdIdx = idx; }

    protected:
                int dvdIdx;

    virtual   void  Process();
    virtual   void  ProcessScale();
    virtual   void  ProcessDVDMark(int idx, const Point& pos);
    virtual   void  ProcessContent() { FormTrack(); }
    virtual   void  ProcessTrack()   { FormDVDThumbnails(); }
    virtual   void  ProcessDVDThumbnail(int idx, const Rect& lct);
};

//
// Разное
// 

inline int GetSclLevel(int top, int btm)
{
    const double prop = 0.35;
    return Round(top*prop + btm*(1-prop));
}

inline bool IsVisibleObj(int pos, int rgn_wdh)
{
    int p_wdh = 10;
    return (pos >= -p_wdh) && (pos <= rgn_wdh+p_wdh);
}

// отрисовать курсор
void PaintPointer(CR::RefPtr<CR::Context> cr, DPoint pos, bool form_only = false);

extern Project::VideoItem CurrVideo;

typedef Project::VideoMD::ListType DVDArrType;
typedef Project::ChapterItem       DVDMark;

inline DVDArrType& DVDMarks()
{
    ASSERT( CurrVideo );
    return CurrVideo->List();
}

struct DVDMarkData
{ 
                           int  pos; 
   CR::RefPtr<CR::ImageSurface> thumbPix;
}; 

// получение данных главы для монтажного окна
inline DVDMarkData& GetMarkData(const DVDMark& ci)
{ return ci->GetData<DVDMarkData>(); }
inline DVDMarkData& GetMarkData(int idx)
{ return GetMarkData(DVDMarks()[idx]); }

Project::VideoItem SetCurrentVideo(Project::VideoItem new_vd);
DVDMark PushBackDVDMark(int frame_pos);

void InsertDVDMark(TrackLayout& trk_lay);

bool DrawRedLine(CR::RefPtr<CR::Context> cr, TrackLayout& trk_lay);

} // namespace Timeline

#endif // #ifndef __MGUI_TIMELINE_SERVICE_H__

