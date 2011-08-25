//
// mgui/editor/text.h
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

#ifndef __MGUI_EDITOR_TEXT_H__
#define __MGUI_EDITOR_TEXT_H__

#include <mgui/mcommon_vis.h>
#include <mgui/rectlist.h>
#include <mgui/timer.h>
#include <mgui/render/text.h>

#include "const.h"

class TextContext
{
    public:
                         ~TextContext() { Clear(); }
                
                    void  Clear();
                    void  Init(RefPtr<Gdk::Pixbuf> canv_pix, double dpi = 0.);

    RefPtr<Pango::Layout> PanLay() { return panLay; }
    Cairo::RefPtr<Cairo::Context> CaiCont() { return caiCont; }

    protected:
    Cairo::RefPtr<Cairo::ImageSurface> caiSur;
         Cairo::RefPtr<Cairo::Context> caiCont;
                 RefPtr<Pango::Layout> panLay;
};


//////////////////////////////////////////////////////////////////////
// CursorState

class TextRenderer;

class CursorState
{
    public:

     virtual      ~CursorState() {}

                   /* Управление состояниями */
                   // при переходе в другое состояние
     virtual void  OnChange(TextRenderer& txt_rndr, bool is_in) = 0;
                   // объект получил фокус
     virtual void  OnGetFocus(TextRenderer& txt_rndr) = 0;
                   // потерял
     virtual void  OnLeaveFocus(TextRenderer& txt_rndr) = 0;
                   // что-то вводят
     virtual void  OnInput(TextRenderer& txt_rndr) = 0;
                   // спрятать/показать курсор
     virtual void  OnShow(TextRenderer& txt_rndr) = 0;

                   // показывать ли курсор
     virtual bool  IsToShow(TextRenderer& txt_rndr) = 0;

     protected:

             void  ChangeState(TextRenderer& txt_rndr, CursorState& stt);
};

class HideCursor: public CursorState, public Singleton<HideCursor>
{
    public:

     virtual void  OnChange(TextRenderer&, bool) {}
     virtual void  OnGetFocus(TextRenderer& txt_rndr);
     virtual void  OnLeaveFocus(TextRenderer&) {}
     virtual void  OnInput(TextRenderer&) {}
     virtual void  OnShow(TextRenderer& txt_rndr);

     virtual bool  IsToShow(TextRenderer&) { return false; }
};

class ShowCursor: public CursorState
{
    public:

     virtual void  OnGetFocus(TextRenderer&) {}
     virtual void  OnLeaveFocus(TextRenderer& txt_rndr);
     virtual void  OnShow(TextRenderer& txt_rndr);
};

class BlinkCursor: public ShowCursor, public Singleton<BlinkCursor>
{
    public:
                   struct Data
                   {
                                   bool  toShow;
                       //sigc::connection  timer;
                                  Timer  timer;
                   };

     virtual void  OnChange(TextRenderer& txt_rndr, bool is_in);
     virtual bool  IsToShow(TextRenderer& txt_rndr);
     virtual void  OnInput(TextRenderer& txt_rndr);

    protected:

             void  SetTimer(TextRenderer& txt_rndr);
             bool  OnTimeout(TextRenderer* txt_rndr);
};

class PendingCursor: public ShowCursor, public Singleton<PendingCursor>
{
    public:
                   struct Data
                   {
                       //sigc::connection  timer;
                                  Timer  timer;
                   };

     virtual void  OnChange(TextRenderer& txt_rndr, bool is_in);
     virtual bool  IsToShow(TextRenderer& ) { return true; }
     virtual void  OnInput(TextRenderer& txt_rndr);

    protected:

             void  SetTimer(TextRenderer& txt_rndr);
             bool  OnTimeout(TextRenderer* txt_rndr);
};


//////////////////////////////////////////////////////////////////////
// TextRenderer

// Соглашения в TextRenderer:
// - Draw* - положить область в очередь для отрисовки
// - Render* - отрисовать "по-настоящему"

class TextRenderer: public TextContext,
                    public BlinkCursor::Data, public PendingCursor::Data
{
    public:
    typedef ActionFunctor DrwFunctor;
    typedef Gtk::DrawingArea OwnerWidget;

                      TextRenderer();
  virtual            ~TextRenderer();

                      // добавить область для перерисовки
  virtual       void  DrawForRegion(RectListRgn& r_lst) = 0;
                      // перед/после отрисовки на rct
  virtual       void  RenderBegin(const Rect& rct) = 0;
  virtual       void  RenderEnd(const Rect& rct) = 0;
                            /* Доступ к данным */
 virtual     TextObj& GetTextObj() = 0;
 virtual OwnerWidget* GetOwnerWidget() = 0;
  //virtual ImageCanvas& GetCanvas() = 0;
  virtual const Planed::Transition& GetTransition() = 0;

                      // отрисоваться на прямоугольнике
                void  RenderByRegion(const Rect& drw_rct);
                void  RenderWithFunctor(DrwFunctor drw_fnr, const Rect& drw_rct, 
                                        const Rect& obj_rct);
                      // рисовать не сразу, а складывать предварительно в delayedRgn
                      // Причина: в некоторых ситуациях TextRenderer "лучше" знает, когда
                      // отрисовываться (MoveCursor, ChangeText), потому уместнее стратегию
                      // перерисовки поручить ему
                void  ClearDelayedDrawing();
                void  CommitDelayedDrawing();
                void  DrawForRegionDelayed(RectListRgn& r_lst);

                      // is_init - установить позицию курсора в начало текста
                void  ShowCursor(bool is_show, bool is_init = false);
                bool  IsShowCursor();
                Rect  CalcCursorPlc();
  virtual       Rect  CalcTextPlc();
  virtual       void  DoLayout();
                      /*Курсор*/
                      // без фокуса курсор не отображаем
                bool  HasFocus();
                      // относительные размеры курсора
              double  CursAspectRatio(); // 0.04 - стандарт
                      // время мигания курсора в миллисек.
                 int  CursBlinkTime();   // 1200 - стандарт

                      // действия на события
                void  OnFocusInEvent();
                void  OnFocusOutEvent();
                void  OnKeyPressEvent(GdkEventKey* event);
                void  OnButtonPressEvent(GdkEventButton* event);

                      // вставить текст в текущую позицию
                void  InsertText(const std::string& insert_str);
          const char* GetText();
                void  ChangeText(const std::string& new_txt, int new_cur_pos);

    protected:

                            Planed::Ratio  txtRat;
                                      int  txtSht;

                              CursorState* curStt;
                                      int  curPos;  // позиция курсора
                                     bool  curShow;

                              RectListRgn  delayedRgn;


            void  ChangeState(CursorState* new_stt);
                  friend class CursorState;

Rect  RawCursorPos();
 int  ByteCursorPos();
void  MoveCursor(int new_pos);
void  MoveCursorToLine(bool is_up);

void  CalcTextSize(Pango::FontDescription& dsc, double font_mult,
                               double& wdh, double& hgt);


void  ApplyTextTrans();
void  RenderCursor();
void  RenderText();

};

void RedrawText(TextRenderer& txt_rndr, bool delayed = false);
void RedrawCursor(TextRenderer& txt_rndr, bool delayed = false);


//////////////////////////////////////////////////////////////////////

namespace utf8
{

class trans
{
    public:
                trans(const char* s): str(s) {}
           int  from_offset(int off)
                { return g_utf8_offset_to_pointer(str, off) - str; }
           int  to_offset(int idx)
                { return g_utf8_pointer_to_offset(str, str+idx); }
           int  to_offset(const char* p_idx)
                { return g_utf8_pointer_to_offset(str, p_idx); }
           int  length() 
                { return g_utf8_strlen(str, -1); }
    private:
        const char* str;
};


};

//////////////////////////////////////////////////////////////////////

class CanvasBuf;

class EdtTextRenderer: public DWConstructorTag,
                       public TextRenderer
{
    typedef MEditorArea EditorType;
    public:

                      EdtTextRenderer(DataWare& dw);
                      // когда в редакторе => можем редактировать
                void  SetEditor(EditorType* edt);
                      // нужна только отрисовка
                void  SetCanvasBuf(CanvasBuf* cnv_buf);

  virtual       void  DrawForRegion(RectListRgn& r_lst);
  virtual       void  RenderBegin(const Rect& rct);
  virtual       void  RenderEnd(const Rect& rct);
  virtual    TextObj& GetTextObj() { return *owner; }
  virtual const Planed::Transition& GetTransition();
  virtual OwnerWidget* GetOwnerWidget();

                      // влияют на процесс отрисовки - если не нуль, то
                      // в rLst аккумулируются все, что нужно отрисовать (позже),
                      // иначе вызывается явная отрисовка
                void  SetRgnAccumulator(RectListRgn& r_lst) { rLst = &r_lst; }
                void  ClearRgnAccumulator() { rLst = 0; }

                bool& ConvertPrecisely() { return cnvPrecisely; }
    protected:

                 TextObj* owner;
             RectListRgn* rLst; 

              EditorType* edtOwner;
               CanvasBuf* cnvBuf;

                    bool  cnvPrecisely; // выбор вариант перевода Cairo <-> Pixbuf
};

EdtTextRenderer& FindTextRenderer(TextObj& t_obj, CanvasBuf& cnv_buf);

namespace Project {

TextObj* CreateEditorText(const std::string& text, Editor::TextStyle style, const Rect& plc);
FrameThemeObj* CreateNewFTO(const FrameTheme& theme, const Rect& lct, MediaItem p_mi, 
                            bool hl_border);
} // namespace Project

EdtTextRenderer& InitETR(TextObj* t_obj, MEditorArea& edt);

#endif // __MGUI_EDITOR_TEXT_H__

