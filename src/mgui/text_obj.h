//
// mgui/text_obj.h
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

#ifndef __MGUI_TEXT_OBJ_H__
#define __MGUI_TEXT_OBJ_H__

#include "text_style.h"

#include <mbase/composite/component.h>

// просчитать, сколько будет занимать места текст txt
// (если dpi = 0, то оно берется из MEdt::GetDpi())
void CalcAbsSizes(const std::string& txt, const Pango::FontDescription& dsc, 
                      double& wdh, double& hgt, double dpi = 0.);
DPoint CalcAbsSizes(RefPtr<Pango::Layout> lay, double dpi);


// :TODO: вставить в иерархию Object
class TextObj: public Comp::MediaObj
{
    typedef Comp::MediaObj MyParent;
    public:
                                TextObj();

                                // = SetText() + SetPlacement()
                          void  Load(const std::string& txt, const Rect& rct, const Editor::TextStyle& style)
                                {
                                    text  = txt;
                                    mdPlc = rct;
                                    SetStyle(style);
                                    CalcDimensions(false);
                                }

        virtual           void  Accept(Comp::ObjVisitor& vis);

                                // атрибуты
     const   Editor::TextStyle& Style() { return tStyle; }
                          void  SetStyle(const Editor::TextStyle& ts) { tStyle = ts; }

             const std::string& Text() { return text; }
                          void  SetTextRaw(const std::string& txt) { text = txt; }

                                // производные для стиля
  const Pango::FontDescription& FontDesc() { return tStyle.fntDsc; }
                          void  SetFontDesc(const Pango::FontDescription& fnt) { tStyle.fntDsc = fnt; }
                          void  SetFontDesc(const std::string& fnt);
                   RGBA::Pixel  Color() { return tStyle.color; }
                          void  SetColor(RGBA::Pixel clr) { tStyle.color = clr; }

                                // в pt (96 dpi)
                        double  FontSize();
                          void  SetFontSize(double sz);
                          
                                // особенность текстового объекта - необходимость "вмещения"
                                // текста в границы самого объекта, потому SetText() и SetPlacement()
                                // "воздействуют" друг на друга
                                // by_text - по тексту/размерам mdPlc
                          void  CalcDimensions(bool by_text);

                          void  SetText(const std::string& txt)
                                {
                                    text = txt;
                                    CalcDimensions(true);
                                }
            virtual       void  SetPlacement(const Rect& rct)
                                {
                                    bool need_calc = (rct.Size() != mdPlc.Size());
                                    mdPlc = rct;
                                    if( need_calc )
                                        CalcDimensions(false);
                                }
                          void  SetLocation(const Point& lct)
                                { SetPlacement( Rect(lct, lct + mdPlc.Size()) ); }

    protected:

                   std::string  text;
             Editor::TextStyle  tStyle;

                                // храним отношение высоты текста fontDsc к размерам mdPlc,
                                // чтобы точно устанавливать размер для SetText()
                        double  hgtMult;

void  CalcBySize(const Point& sz);

};

namespace MEdt
{
// 96 dpi
// так как мы манирулируем точечными данными, то dpi по большому счету не нужен.
// Но, размер текста принято->удобно учитывать в физ. размерах (pt), потому выберем
// точку отчета
inline double GetDpi()
{
    return 96.;
}

double FontSize(const Pango::FontDescription& dsc);
void SetFontSize(Pango::FontDescription& dsc, double sz);

// проверка на допустимость шрифта у стиля
// установит "Sans Bold 27", если пустой
void CheckDescNonNull(Editor::TextStyle& ts);

}

#endif // __MGUI_TEXT_OBJ_H__

