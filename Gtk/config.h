#ifndef _CONFIG_H
/*
 * config.h - Storing to and retrieving from the configuration file
 * Copyright (c) 2002 Torbjörn Andersson <d91tan@Update.UU.SE>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 */

#define _CONFIG_H

/*
 * This is stupidly small, but on the off-chance that some user wants to be
 * able to play on a 320x240 screen...
 */

#define MIN_WINDOW_WIDTH 300
#define MIN_WINDOW_HEIGHT 200

struct configuration
{
    gint window_width;
    gint window_height;
    gint window_split;

    gint hints_width;
    gint hints_height;
    
    gchar *text_font;
    gchar *text_fg;
    gchar *text_bg;
    gchar *statusline_fg;
    gchar *statusline_bg;
    
    gboolean image_constant_height;
    gdouble image_scale;
    gint image_height;
    gint image_filter;     /* Should really be GdkInterpType, not gint */
    gdouble red_gamma;
    gdouble green_gamma;
    gdouble blue_gamma;
    gchar *graphics_bg;
    gboolean animate_images;
    gint animation_delay;
};

extern struct configuration config;

void read_config_file (void);
void write_config_file (void);
void do_config (void);

#endif
