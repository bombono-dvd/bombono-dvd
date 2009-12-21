#include <mgui/tests/_pc_.h>

#include "mgui_test.h"

#include <mgui/sdk/clearlooks.h> 

/*
Copyright for:

 * Helpful functions when dealing with cairo in gtk engines

 * Clearlooks Theme Engine
   Copyright (C) 2005 Richard Stellingwerff.

*/


typedef enum
{
	CR_CORNER_NONE        = 0,
	CR_CORNER_TOPLEFT     = 1,
	CR_CORNER_TOPRIGHT    = 2,
	CR_CORNER_BOTTOMLEFT  = 4,
	CR_CORNER_BOTTOMRIGHT = 8,
	CR_CORNER_ALL         = 15
} CairoCorners;

void 
ge_cairo_pattern_add_color_stop_color (cairo_pattern_t *pattern, 
						gfloat offset, 
						const CairoColor *color)
{
	g_return_if_fail (pattern && color);

	cairo_pattern_add_color_stop_rgba (pattern, offset, color->r, color->g, color->b, color->a);	
}

void
ge_cairo_pattern_add_color_stop_shade(cairo_pattern_t *pattern, 
						gdouble offset, 
						const CairoColor *color, 
						gdouble shade)
{
	CairoColor shaded;

	g_return_if_fail (pattern && color && (shade >= 0) && (shade <= 3));

	shaded = *color;

	if (shade != 1)
	{
		ge_shade_color(color, shade, &shaded);
	}

	ge_cairo_pattern_add_color_stop_color(pattern, offset, &shaded);	
}

////////////////////////////////////////////////////////

namespace 
{
// int my_xthickness = 2;
// int my_ythickness = 2;
//
// bool tweak_active = true;
// bool tweak_disabled = false;

struct ButtonOpt
{
    int my_xthickness;
    int my_ythickness;
    
    bool tweak_active;
    bool tweak_disabled;
    bool tweak_shadow;

    ButtonOpt(): my_xthickness(2), my_ythickness(2), tweak_active(false), tweak_disabled(false),
        tweak_shadow(true) {}
};

}

static void
clearlooks_set_border_gradient (cairo_t *cr, const CairoColor *color, double hilight, int width, int height)
{
	cairo_pattern_t *pattern;

	CairoColor bottom_shade;
	ge_shade_color (color, hilight, &bottom_shade);

	pattern	= cairo_pattern_create_linear (0, 0, width, height);
	cairo_pattern_add_color_stop_rgb (pattern, 0, color->r, color->g, color->b);
	cairo_pattern_add_color_stop_rgb (pattern, 1, bottom_shade.r, bottom_shade.g, bottom_shade.b);
	
	cairo_set_source (cr, pattern);
	cairo_pattern_destroy (pattern);
}

static void
clearlooks_draw_top_left_highlight (cairo_t *cr, const CairoColor *color,
//                                     const WidgetParameters *params,
//                                     int width, int height, gdouble radius)
                                    int width, int height, ButtonOpt opt)
{
	CairoColor hilight; 

	double light_top = opt.my_ythickness-1,
	       light_bottom = height - opt.my_ythickness - 1,
	       light_left = opt.my_xthickness-1,
	       light_right = width - opt.my_xthickness - 1;

	ge_shade_color (color, 1.3, &hilight);
    //cairo_move_to         (cr, light_left, light_bottom - (int)radius/2);
	cairo_move_to         (cr, light_left, light_bottom);

    //ge_cairo_rounded_corner (cr, light_left, light_top, radius, params->corners & CR_CORNER_TOPLEFT);
	cairo_line_to (cr, light_left, light_top);

    //cairo_line_to         (cr, light_right - (int)radius/2, light_top);
	cairo_line_to         (cr, light_right, light_top);
	cairo_set_source_rgba (cr, hilight.r, hilight.g, hilight.b, 0.5);
	cairo_stroke          (cr);
}

void
ge_cairo_rounded_rectangle (cairo_t *cr,
                                 double x, double y, double w, double h,
                                 double radius, CairoCorners corners)
{
	g_return_if_fail (cr != NULL);

	if (radius < 0.0001 || corners == CR_CORNER_NONE)
	{
		cairo_rectangle (cr, x, y, w, h);
		return;
	}
#ifdef DEVELOPMENT
	if ((corners == CR_CORNER_ALL) && (radius > w / 2.0 || radius > h / 2.0))
		g_warning ("Radius is too large for width/height in ge_rounded_rectangle.\n");
	else if (radius > w || radius > h) /* This isn't perfect. Assumes that only one corner is set. */
		g_warning ("Radius is too large for width/height in ge_rounded_rectangle.\n");
#endif

	if (corners & CR_CORNER_TOPLEFT)
		cairo_move_to (cr, x+radius, y);
	else
		cairo_move_to (cr, x, y);
	
	if (corners & CR_CORNER_TOPRIGHT)
		cairo_arc (cr, x+w-radius, y+radius, radius, G_PI * 1.5, G_PI * 2);
	else
		cairo_line_to (cr, x+w, y);
	
	if (corners & CR_CORNER_BOTTOMRIGHT)
		cairo_arc (cr, x+w-radius, y+h-radius, radius, 0, G_PI * 0.5);
	else
		cairo_line_to (cr, x+w, y+h);
	
	if (corners & CR_CORNER_BOTTOMLEFT)
		cairo_arc (cr, x+radius,   y+h-radius, radius, G_PI * 0.5, G_PI);
	else
		cairo_line_to (cr, x, y+h);
	
	if (corners & CR_CORNER_TOPLEFT)
		cairo_arc (cr, x+radius,   y+radius,   radius, G_PI, G_PI * 1.5);
	else
		cairo_line_to (cr, x, y);
}

static void
clearlooks_draw_button (cairo_t *cr,
                        //const ClearlooksColors *colors,
                        //const WidgetParameters *params,
                        CairoColor *fill, CairoColor *border_normal, CairoColor *border_disabled,
                        int x, int y, int width, int height, ButtonOpt opt)
{
    double xoffset = 0, yoffset = 0;
//     double radius = params->radius;
//     const CairoColor *fill = &colors->bg[params->state_type];
//     const CairoColor *border_normal = &colors->shade[6];
//     const CairoColor *border_disabled = &colors->shade[4];

    CairoColor shadow;
    ge_shade_color (border_normal, 0.925, &shadow);

    cairo_save (cr);

    cairo_translate (cr, x, y);
    cairo_set_line_width (cr, 1.0);

    if (opt.my_xthickness == 3 || opt.my_ythickness == 3)
    {
        if (opt.my_xthickness == 3)
            xoffset = 1;
        if (opt.my_ythickness == 3)
            yoffset = 1;
    }

//     radius = MIN (radius, MIN ((width - 2.0 - xoffset * 2.0) / 2.0, (height - 2.0 - yoffset * 2) / 2.0));

//     if (params->xthickness == 3 || params->ythickness == 3)
//     {
//         cairo_translate (cr, 0.5, 0.5);
//         params->style_functions->draw_inset (cr, colors, width-1, height-1, radius, params->corners);
//         cairo_translate (cr, -0.5, -0.5);
//     }

    //ge_cairo_rounded_rectangle (cr, xoffset+1, yoffset+1,
    cairo_rectangle              (cr, xoffset+1, yoffset+1,
                                         width-(xoffset*2)-2,
                                         height-(yoffset*2)-2
                                        ); //radius, params->corners);

    //if (!params->active)
    if ( !opt.tweak_active )
    {
        cairo_pattern_t *pattern;
        gdouble shade_size = ((100.0/height)*8.0)/100.0;
        CairoColor top_shade, bottom_shade, middle_shade;

        ge_shade_color (fill, 1.1, &top_shade);
        ge_shade_color (fill, 0.98, &middle_shade);
        ge_shade_color (fill, 0.93, &bottom_shade);

//         ge_shade_color (fill, 0.93, &top_shade);
//         ge_shade_color (fill, 0.98, &middle_shade);
//         ge_shade_color (fill, 1.1,  &bottom_shade);

        pattern = cairo_pattern_create_linear (0, 0, 0, height);
        cairo_pattern_add_color_stop_rgb (pattern, 0.0, top_shade.r, top_shade.g, top_shade.b);
        cairo_pattern_add_color_stop_rgb (pattern, shade_size, fill->r, fill->g, fill->b);
        cairo_pattern_add_color_stop_rgb (pattern, 1.0 - shade_size, middle_shade.r, middle_shade.g, middle_shade.b);
        cairo_pattern_add_color_stop_rgb (pattern, (height-(yoffset*2)-1)/height, bottom_shade.r, bottom_shade.g, bottom_shade.b);
        cairo_pattern_add_color_stop_rgba (pattern, (height-(yoffset*2)-1)/height, bottom_shade.r, bottom_shade.g, bottom_shade.b, 0.7);
        cairo_pattern_add_color_stop_rgba (pattern, 1.0, bottom_shade.r, bottom_shade.g, bottom_shade.b, 0.7);

        cairo_set_source (cr, pattern);
        cairo_fill (cr);
        cairo_pattern_destroy (pattern);
    }
    else
    {
        cairo_pattern_t *pattern;

        ge_cairo_set_color (cr, fill);
        cairo_fill_preserve (cr);

        pattern = cairo_pattern_create_linear (0, 0, 0, height);
        cairo_pattern_add_color_stop_rgba (pattern, 0.0, shadow.r, shadow.g, shadow.b, 0.0);
        cairo_pattern_add_color_stop_rgba (pattern, 0.4, shadow.r, shadow.g, shadow.b, 0.0);
        cairo_pattern_add_color_stop_rgba (pattern, 1.0, shadow.r, shadow.g, shadow.b, 0.2);
        cairo_set_source (cr, pattern);
        cairo_fill_preserve (cr);
        cairo_pattern_destroy (pattern);

        pattern = cairo_pattern_create_linear (0, yoffset+1, 0, 3+yoffset);
        //cairo_pattern_add_color_stop_rgba (pattern, 0.0, shadow.r, shadow.g, shadow.b, params->disabled ? 0.125 : 0.3);
        cairo_pattern_add_color_stop_rgba (pattern, 0.0, shadow.r, shadow.g, shadow.b, opt.tweak_disabled ? 0.125 : 0.3);
        cairo_pattern_add_color_stop_rgba (pattern, 1.0, shadow.r, shadow.g, shadow.b, 0.0);
        cairo_set_source (cr, pattern);
        cairo_fill_preserve (cr);
        cairo_pattern_destroy (pattern);

        pattern = cairo_pattern_create_linear (xoffset+1, 0, 3+xoffset, 0);
        //cairo_pattern_add_color_stop_rgba (pattern, 0.0, shadow.r, shadow.g, shadow.b, params->disabled ? 0.125 : 0.3);
        cairo_pattern_add_color_stop_rgba (pattern, 0.0, shadow.r, shadow.g, shadow.b, opt.tweak_disabled ? 0.125 : 0.3);
        cairo_pattern_add_color_stop_rgba (pattern, 1.0, shadow.r, shadow.g, shadow.b, 0.0);
        cairo_set_source (cr, pattern);
        cairo_fill (cr);
        cairo_pattern_destroy (pattern);
    }


    /* Drawing the border */
    //if (!params->active && params->is_default)
//     if ( !tweak_active && tweak_default )
//     {
//         const CairoColor *l = &colors->shade[4];
//         const CairoColor *d = &colors->shade[4];
//         ge_cairo_set_color (cr, l);
//         ge_cairo_stroke_rectangle (cr, 2.5, 2.5, width-5, height-5);
//
//         ge_cairo_set_color (cr, d);
//         ge_cairo_stroke_rectangle (cr, 3.5, 3.5, width-7, height-7);
//     }

    //ge_cairo_rounded_rectangle (cr, xoffset + 0.5, yoffset + 0.5, width-(xoffset*2)-1, height-(yoffset*2)-1, radius, params->corners);
    cairo_rectangle (cr, xoffset + 0.5, yoffset + 0.5, width-(xoffset*2)-1, height-(yoffset*2)-1);

    //if (params->disabled)
    if ( opt.tweak_disabled )
        ge_cairo_set_color (cr, border_disabled);
    else
        //if (!params->active)
        if ( !opt.tweak_active )
            clearlooks_set_border_gradient (cr, border_normal, 1.32, 0, height);
        else
            ge_cairo_set_color (cr, border_normal);

    cairo_stroke (cr);

    /* Draw the "shadow" */
    //if (!params->active)
    if ( !opt.tweak_active && opt.tweak_shadow )
    {
        cairo_translate (cr, 0.5, 0.5);
        /* Draw right shadow */
        cairo_move_to (cr, width - opt.my_xthickness, opt.my_ythickness - 1);
        cairo_line_to (cr, width - opt.my_xthickness, height - opt.my_ythickness - 1);
        cairo_set_source_rgba (cr, shadow.r, shadow.g, shadow.b, 0.1);
        cairo_stroke (cr);

        /* Draw topleft shadow */
        //clearlooks_draw_top_left_highlight (cr, fill, params, width, height, radius);
        clearlooks_draw_top_left_highlight (cr, fill, width, height, opt);
    }
    cairo_restore (cr);
}

bool DoDrawButton(Gtk::DrawingArea& da, GdkEventExpose* )//event)
{
    Point sz(WidgetSize(da));
    Cairo::RefPtr<Cairo::Context> cr = da.get_window()->create_cairo_context();

    cr->set_line_width(1.0);

    Rect rct(10, 10, 310, 50);
//     cr->rectangle(rct.lft+0.5, rct.top+0.5, rct.Width(), rct.Height());
//     cr->stroke();

    CR::Color fill(GetBGColor(da));
    CR::Color border_normal(GetBorderColor(da));
    CR::Color border_disabled(GetBorderColor(da));

    ButtonOpt opt;
    opt.my_xthickness = 3;
    opt.my_ythickness = 3;

    // 1
    clearlooks_draw_button (cr->cobj(), &fill, &border_normal, &border_disabled,
                            rct.lft, rct.top, rct.Width(), rct.Height(), opt);

//     // 2
//     rct += Point(0, 41);
//     opt.my_xthickness = 3;
//     opt.my_ythickness = 3;
//     clearlooks_draw_button (cr->cobj(), &fill, &border_normal, &border_disabled,
//                             rct.lft, rct.top, rct.Width(), rct.Height(), opt);

    // 3
    rct += Point(0, 41);
    opt.tweak_disabled = true;
    clearlooks_draw_button (cr->cobj(), &fill, &border_normal, &border_disabled,
                            rct.lft, rct.top, rct.Width(), rct.Height(), opt);

    // 4
    rct += Point(0, 41);
    opt.tweak_shadow = false;
    clearlooks_draw_button (cr->cobj(), &fill, &border_normal, &border_disabled,
                            rct.lft, rct.top, rct.Width(), rct.Height(), opt);

    // 5
    rct += Point(0, 41);
    opt.tweak_active = true;
    clearlooks_draw_button (cr->cobj(), &fill, &border_normal, &border_disabled,
                            rct.lft, rct.top, rct.Width(), rct.Height(), opt);


    return true;
}

BOOST_AUTO_TEST_CASE( DrawButtonTest )
{
    //TestExampleDA(DoDrawButton);
}

