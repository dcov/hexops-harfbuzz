/*
 * Copyright © 2022  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HELPER_CAIRO_USER_HH
#define HELPER_CAIRO_USER_HH

#include "config.h"

#include "font-options.hh"

#include <cairo.h>
#include <hb.h>

#include "hb-blob.hh"
#include "hb-cairo-utils.h"

static const cairo_user_data_key_t _hb_font_cairo_user_data_key = {0};

static void
move_to (hb_draw_funcs_t *dfuncs,
	 cairo_t *cr,
	 hb_draw_state_t *st,
	 float to_x, float to_y,
	 void *)
{
  cairo_move_to (cr,
		 (double) to_x, (double) to_y);
}

static void
line_to (hb_draw_funcs_t *dfuncs,
	 cairo_t *cr,
	 hb_draw_state_t *st,
	 float to_x, float to_y,
	 void *)
{
  cairo_line_to (cr,
		 (double) to_x, (double) to_y);
}

static void
cubic_to (hb_draw_funcs_t *dfuncs,
	  cairo_t *cr,
	  hb_draw_state_t *st,
	  float control1_x, float control1_y,
	  float control2_x, float control2_y,
	  float to_x, float to_y,
	  void *)
{
  cairo_curve_to (cr,
		  (double) control1_x, (double) control1_y,
		  (double) control2_x, (double) control2_y,
		  (double) to_x, (double) to_y);
}

static void
close_path (hb_draw_funcs_t *dfuncs,
	    cairo_t *cr,
	    hb_draw_state_t *st,
	    void *)
{
  cairo_close_path (cr);
}


static hb_draw_funcs_t *
get_cairo_draw_funcs (void)
{
  static hb_draw_funcs_t *funcs;

  if (!funcs)
  {
    funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) line_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) close_path, nullptr, nullptr);
  }

  return funcs;
}

#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC

static void
push_transform (hb_paint_funcs_t *funcs,
                void *paint_data,
                float xx, float yx,
                float xy, float yy,
                float dx, float dy,
                const hb_paint_context_t *ctx,
                void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;
  cairo_matrix_t m;

  cairo_save (cr);
  cairo_matrix_init (&m, (double)xx, (double)yx,
                         (double)xy, (double)yy,
                         (double)dx, (double)dy);
  cairo_transform (cr, &m);
}

static void
pop_transform (hb_paint_funcs_t *funcs,
               void *paint_data,
               const hb_paint_context_t *ctx,
               void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  cairo_restore (cr);
}

static void
push_clip_glyph (hb_paint_funcs_t *funcs,
                 void *paint_data,
                 hb_codepoint_t glyph,
                 const hb_paint_context_t *ctx,
                 void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  cairo_save (cr);
  cairo_new_path (cr);
  hb_font_draw_glyph (ctx->font, glyph, get_cairo_draw_funcs (), cr);
  cairo_close_path (cr);
  cairo_clip (cr);
}

static void
push_clip_rectangle (hb_paint_funcs_t *funcs,
                     void *paint_data,
                     float xmin, float ymin, float xmax, float ymax,
                     const hb_paint_context_t *ctx,
                     void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  cairo_save (cr);
  cairo_rectangle (cr,
                   (double)xmin, (double)ymin,
                   (double)(xmax - xmin), (double)(ymax - ymin));
  cairo_clip (cr);
}

static void
pop_clip (hb_paint_funcs_t *funcs,
          void *paint_data,
          const hb_paint_context_t *ctx,
          void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  cairo_restore (cr);
}

static void
push_group (hb_paint_funcs_t *funcs,
            void *paint_data,
            const hb_paint_context_t *ctx,
            void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  cairo_save (cr);
  cairo_push_group (cr);
}

static void
pop_group (hb_paint_funcs_t *funcs,
           void *paint_data,
           hb_paint_composite_mode_t mode,
           const hb_paint_context_t *ctx,
           void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  cairo_pop_group_to_source (cr);
  cairo_set_operator (cr, hb_paint_composite_mode_to_cairo (mode));
  cairo_paint (cr);

  cairo_restore (cr);
}

static void
paint_color (hb_paint_funcs_t *funcs,
             void *paint_data,
             hb_color_t color,
             const hb_paint_context_t *ctx,
             void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  cairo_set_source_rgba (cr,
                         hb_color_get_red (color) / 255.,
                         hb_color_get_green (color) / 255.,
                         hb_color_get_blue (color) / 255.,
                         hb_color_get_alpha (color) / 255.);
  cairo_paint (cr);
}

static void
paint_image (hb_paint_funcs_t *funcs,
             void *paint_data,
             hb_blob_t *blob,
             hb_tag_t format,
             hb_glyph_extents_t *extents,
             const hb_paint_context_t *ctx,
             void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  hb_cairo_paint_glyph_image (cr, ctx, blob, format, extents);
}

static void
paint_linear_gradient (hb_paint_funcs_t *funcs,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0,
                       float x1, float y1,
                       float x2, float y2,
                       const hb_paint_context_t *ctx,
                       void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  hb_cairo_paint_linear_gradient (cr, ctx, color_line, x0, y0, x1, y1, x2, y2);
}

static void
paint_radial_gradient (hb_paint_funcs_t *funcs,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0, float r0,
                       float x1, float y1, float r1,
                       const hb_paint_context_t *ctx,
                       void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  hb_cairo_paint_radial_gradient (cr, ctx, color_line, x0, y0, r0, x1, y1, r1);
}

static void
paint_sweep_gradient (hb_paint_funcs_t *funcs,
                      void *paint_data,
                      hb_color_line_t *color_line,
                      float x0, float y0,
                      float start_angle, float end_angle,
                      const hb_paint_context_t *ctx,
                      void *user_data)
{
  cairo_t *cr = (cairo_t *)paint_data;

  hb_cairo_paint_sweep_gradient (cr, ctx, color_line, x0, y0, start_angle, end_angle);
}

static hb_paint_funcs_t *
get_cairo_paint_funcs ()
{
  static hb_paint_funcs_t *funcs;

  if (!funcs)
  {
    funcs = hb_paint_funcs_create ();

    hb_paint_funcs_set_push_transform_func (funcs, push_transform, nullptr, nullptr);
    hb_paint_funcs_set_pop_transform_func (funcs, pop_transform, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_rectangle_func (funcs, push_clip_rectangle, nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func (funcs, pop_clip, nullptr, nullptr);
    hb_paint_funcs_set_push_group_func (funcs, push_group, nullptr, nullptr);
    hb_paint_funcs_set_pop_group_func (funcs, pop_group, nullptr, nullptr);
    hb_paint_funcs_set_color_func (funcs, paint_color, nullptr, nullptr);
    hb_paint_funcs_set_image_func (funcs, paint_image, nullptr, nullptr);
    hb_paint_funcs_set_linear_gradient_func (funcs, paint_linear_gradient, nullptr, nullptr);
    hb_paint_funcs_set_radial_gradient_func (funcs, paint_radial_gradient, nullptr, nullptr);
    hb_paint_funcs_set_sweep_gradient_func (funcs, paint_sweep_gradient, nullptr, nullptr);
  }

  return funcs;
}

#endif

#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC
static cairo_status_t
render_color_glyph (cairo_scaled_font_t  *scaled_font,
		    unsigned long         glyph,
		    cairo_t              *cr,
		    cairo_text_extents_t *extents);
#endif

static cairo_status_t
render_glyph (cairo_scaled_font_t  *scaled_font,
	      unsigned long         glyph,
	      cairo_t              *cr,
	      cairo_text_extents_t *extents)
{
#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC
  static char *p = getenv ("HB_DRAW");
  if (p && atoi (p) >= 2)
    return render_color_glyph (scaled_font, glyph, cr, extents);
#endif

  hb_font_t *font = (hb_font_t *) (cairo_font_face_get_user_data (cairo_scaled_font_get_font_face (scaled_font),
								  &_hb_font_cairo_user_data_key));

  hb_position_t x_scale, y_scale;
  hb_font_get_scale (font, &x_scale, &y_scale);
  cairo_scale (cr, +1./x_scale, -1./y_scale);

  hb_font_draw_glyph (font, glyph, get_cairo_draw_funcs (), cr);
  cairo_fill (cr);

  return CAIRO_STATUS_SUCCESS;
}

#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC

static cairo_status_t
render_color_glyph (cairo_scaled_font_t  *scaled_font,
		    unsigned long         glyph,
		    cairo_t              *cr,
		    cairo_text_extents_t *extents)
{
  hb_font_t *font = (hb_font_t *) (cairo_font_face_get_user_data (cairo_scaled_font_get_font_face (scaled_font),
								  &_hb_font_cairo_user_data_key));
  unsigned int palette = 0;
#ifdef CAIRO_COLOR_PALETTE_DEFAULT
  cairo_font_options_t *options = cairo_font_options_create ();
  cairo_scaled_font_get_font_options (scaled_font, options);
  palette = cairo_font_options_get_color_palette (options);
  cairo_font_options_destroy (options);
#endif

  hb_color_t color = HB_COLOR (0, 0, 0, 255);
  cairo_pattern_t *pattern = cairo_get_source (cr);

  if (cairo_pattern_get_type (pattern) == CAIRO_PATTERN_TYPE_SOLID)
  {
    double r, g, b, a;
    cairo_pattern_get_rgba (pattern, &r, &g, &b, &a);
    color = HB_COLOR ((int)(b * 255.), (int)(g * 255.), (int) (r * 255.), (int)(a * 255.));
  }

  hb_position_t x_scale, y_scale;
  hb_font_get_scale (font, &x_scale, &y_scale);
  cairo_scale (cr, +1./x_scale, -1./y_scale);

  hb_font_paint_glyph (font, glyph, get_cairo_paint_funcs (), cr, palette, color);

  hb_glyph_extents_t hb_extents;
  hb_font_get_glyph_extents (font, glyph, &hb_extents);
  extents->x_bearing = (double) hb_extents.x_bearing / x_scale;
  extents->y_bearing = (double)-hb_extents.y_bearing / y_scale;
  extents->width     = (double) hb_extents.width     / x_scale;
  extents->height    = (double)-hb_extents.height    / y_scale;

  return CAIRO_STATUS_SUCCESS;
}

#endif

static inline cairo_font_face_t *
helper_cairo_create_user_font_face (const font_options_t *font_opts)
{
  cairo_font_face_t *cairo_face = cairo_user_font_face_create ();

  cairo_font_face_set_user_data (cairo_face,
				 &_hb_font_cairo_user_data_key,
				 hb_font_reference (font_opts->font),
				 (cairo_destroy_func_t) hb_font_destroy);

  cairo_user_font_face_set_render_glyph_func (cairo_face, render_glyph);

#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC
  hb_face_t *face = hb_font_get_face (font_opts->font);
  if (hb_ot_color_has_png (face) || hb_ot_color_has_layers (face) || hb_ot_color_has_paint (face))
  {
    cairo_user_font_face_set_render_color_glyph_func (cairo_face, render_color_glyph);
  }
#endif

  return cairo_face;
}

static inline bool
helper_cairo_user_font_face_has_data (cairo_font_face_t *font_face)
{
  return cairo_font_face_get_user_data (font_face, &_hb_font_cairo_user_data_key);
}

static inline bool
helper_cairo_user_scaled_font_has_color (cairo_scaled_font_t *scaled_font)
{
  /* Ignoring SVG for now, since we cannot render it. */
  hb_font_t *font = (hb_font_t *) (cairo_font_face_get_user_data (cairo_scaled_font_get_font_face (scaled_font),
								  &_hb_font_cairo_user_data_key));
  hb_face_t *face = hb_font_get_face (font);
  return hb_ot_color_has_png (face) || hb_ot_color_has_layers (face) || hb_ot_color_has_paint (face);
}

#endif
