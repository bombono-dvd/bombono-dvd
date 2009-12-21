#include <mgui/_pc_.h>

#include "clearlooks.h"

/*
Copyright for:

 * Helpful functions when dealing with cairo in gtk engines

 * Clearlooks Theme Engine
   Copyright (C) 2005 Richard Stellingwerff.

*/

// COPY_N_PASTE_ETALON

/***********************************************
 * ge_hsb_from_color -
 *  
 *   Get HSB values from RGB values.
 *
 *   Modified from Smooth but originated in GTK+
 ***********************************************/
void
ge_hsb_from_color (const CairoColor *color, 
                        gdouble *hue, 
                        gdouble *saturation,
                        gdouble *brightness) 
{
	gdouble min, max, delta;
	gdouble red, green, blue;

	red = color->r;
	green = color->g;
	blue = color->b;
  
	if (red > green)
	{
		max = MAX(red, blue);
		min = MIN(green, blue);      
	}
	else
	{
		max = MAX(green, blue);
		min = MIN(red, blue);      
	}
  
	*brightness = (max + min) / 2;
 	
	if (max == min)
	{
		*hue = 0;
		*saturation = 0;
	}	
	else
	{
		if (*brightness <= 0.5)
			*saturation = (max - min) / (max + min);
		else
			*saturation = (max - min) / (2 - max - min);
       
		delta = max -min;
 
		if (red == max)
			*hue = (green - blue) / delta;
		else if (green == max)
			*hue = 2 + (blue - red) / delta;
		else if (blue == max)
			*hue = 4 + (red - green) / delta;
 
		*hue *= 60;
		if (*hue < 0.0)
			*hue += 360;
	}
}
 
/***********************************************
 * ge_color_from_hsb -
 *  
 *   Get RGB values from HSB values.
 *
 *   Modified from Smooth but originated in GTK+
 ***********************************************/
#define MODULA(number, divisor) (((gint)number % divisor) + (number - (gint)number))
void
ge_color_from_hsb (gdouble hue, 
                        gdouble saturation,
                        gdouble brightness, 
                        CairoColor *color)
{
	gint i;
	gdouble hue_shift[3], color_shift[3];
	gdouble m1, m2, m3;

	if (!color) return;
  	  
	if (brightness <= 0.5)
		m2 = brightness * (1 + saturation);
	else
		m2 = brightness + saturation - brightness * saturation;
 
	m1 = 2 * brightness - m2;
 
	hue_shift[0] = hue + 120;
	hue_shift[1] = hue;
	hue_shift[2] = hue - 120;
 
	color_shift[0] = color_shift[1] = color_shift[2] = brightness;	
 
	i = (saturation == 0)?3:0;
 
	for (; i < 3; i++)
	{
		m3 = hue_shift[i];
 
		if (m3 > 360)
			m3 = MODULA(m3, 360);
		else if (m3 < 0)
			m3 = 360 - MODULA(ABS(m3), 360);
 
		if (m3 < 60)
			color_shift[i] = m1 + (m2 - m1) * m3 / 60;
		else if (m3 < 180)
			color_shift[i] = m2;
		else if (m3 < 240)
			color_shift[i] = m1 + (m2 - m1) * (240 - m3) / 60;
		else
			color_shift[i] = m1;
	}	
 
	color->r = color_shift[0];
	color->g = color_shift[1];
	color->b = color_shift[2];	
	color->a = 1.0;	
}

void
ge_gdk_color_to_cairo (const GdkColor *c, CairoColor *cc)
{
	gdouble r, g, b;

	g_return_if_fail (c && cc);

	r = c->red / 65536.0;
	g = c->green / 65536.0;
	b = c->blue / 65536.0;

	cc->r = r;
	cc->g = g;
	cc->b = b;
	cc->a = 1.0;
}

void
ge_cairo_color_to_gtk (const CairoColor *cc, GdkColor *c)
{
	gdouble r, g, b;

	g_return_if_fail (c && cc);

	r = cc->r * 65536.0;
	g = cc->g * 65536.0;
	b = cc->b * 65536.0;

	c->red   = (guint16) r;
	c->green = (guint16) g;
	c->blue  = (guint16) b;
}

void
ge_shade_color(const CairoColor *base, gdouble shade_ratio, CairoColor *composite)
{
	gdouble hue = 0;
	gdouble saturation = 0;
	gdouble brightness = 0;
 
	g_return_if_fail (base && composite);

	ge_hsb_from_color (base, &hue, &saturation, &brightness);
 
	brightness = MIN(brightness*shade_ratio, 1.0);
	brightness = MAX(brightness, 0.0);
  
	saturation = MIN(saturation*shade_ratio, 1.0);
	saturation = MAX(saturation, 0.0);
  
	ge_color_from_hsb (hue, saturation, brightness, composite);
	composite->a = base->a;	
}

void
ge_saturate_color (const CairoColor * base, gdouble saturate_level, CairoColor *composite)
{
	gdouble hue = 0;
	gdouble saturation = 0;
	gdouble brightness = 0;
 
	g_return_if_fail (base && composite);

	ge_hsb_from_color (base, &hue, &saturation, &brightness);

	saturation = MIN(saturation*saturate_level, 1.0);
	saturation = MAX(saturation, 0.0);

	ge_color_from_hsb (hue, saturation, brightness, composite);
	composite->a = base->a;	
}

void 
ge_cairo_set_color (cairo_t *cr, const CairoColor *color)
{
	g_return_if_fail (cr && color);

	cairo_set_source_rgba (cr, color->r, color->g, color->b, color->a);	
}

////////////////////////////////////////////////////////

// реализация взята из отрисовки кнопки, темы Clearlooks, 
// см. clearlooks_draw_button()
void FillScaleGradient(CR::RefPtr<CR::Context> mm_cr, Rect lct, const CR::Color& clr)
{
    CairoStateSave save(mm_cr);

    cairo_t *cr = mm_cr->cobj();
    CairoColor fill_clr(clr);
    CairoColor* fill = &fill_clr;

    double yoffset = 0;
    int x = lct.lft, y = lct.top;
    int height = lct.Height();

    cairo_translate (cr, x, y);

    //if (!params->active)
    {
        cairo_pattern_t *pattern;
        gdouble shade_size = ((100.0/height)*8.0)/100.0;
        CairoColor top_shade, bottom_shade, middle_shade;

        ge_shade_color (fill, 1.1, &top_shade);
        ge_shade_color (fill, 0.98, &middle_shade);
        ge_shade_color (fill, 0.93, &bottom_shade);

//         ge_shade_color (fill, 0.85, &top_shade);
//         ge_shade_color (fill, 0.93, &middle_shade);
//         ge_shade_color (fill, 1.05, &bottom_shade);

        pattern = cairo_pattern_create_linear (0, 0, 0, height);
        cairo_pattern_add_color_stop_rgb (pattern, 0.0, top_shade.r, top_shade.g, top_shade.b);
        cairo_pattern_add_color_stop_rgb (pattern, shade_size, fill->r, fill->g, fill->b);
        cairo_pattern_add_color_stop_rgb (pattern, 1.0 - shade_size, middle_shade.r, middle_shade.g, middle_shade.b);
//         cairo_pattern_add_color_stop_rgb (pattern, shade_size, middle_shade.r, middle_shade.g, middle_shade.b);
//         cairo_pattern_add_color_stop_rgb (pattern, 1.0 - shade_size, fill->r, fill->g, fill->b);
        cairo_pattern_add_color_stop_rgb (pattern, (height-(yoffset*2)-1)/height, bottom_shade.r, bottom_shade.g, bottom_shade.b);
        cairo_pattern_add_color_stop_rgba (pattern, (height-(yoffset*2)-1)/height, bottom_shade.r, bottom_shade.g, bottom_shade.b, 0.7);
        cairo_pattern_add_color_stop_rgba (pattern, 1.0, bottom_shade.r, bottom_shade.g, bottom_shade.b, 0.7);

        cairo_set_source (cr, pattern);
        cairo_fill_preserve (cr);
        cairo_pattern_destroy (pattern);
    }
}

