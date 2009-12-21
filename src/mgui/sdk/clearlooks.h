
#include <mgui/win_utils.h>

/*
Copyright for:

 * Helpful functions when dealing with cairo in gtk engines

 * Clearlooks Theme Engine
   Copyright (C) 2005 Richard Stellingwerff.

*/

// COPY_N_PASTE_ETALON

// typedef struct
// {
//         gdouble r;
//         gdouble g;
//         gdouble b;
//         gdouble a;
// } CairoColor;
typedef CR::Color CairoColor;

void
ge_shade_color(const CairoColor *base, gdouble shade_ratio, CairoColor *composite);

void 
ge_cairo_set_color (cairo_t *cr, const CairoColor *color);

void
ge_saturate_color (const CairoColor * base, gdouble saturate_level, CairoColor *composite);

///////////////////////////////////////////////////////////////////////////////////////////

// заполнить область шкалы градиентом 
void FillScaleGradient(CR::RefPtr<CR::Context> mm_cr, Rect lct, const CR::Color& clr);






