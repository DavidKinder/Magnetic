#ifndef _MAIN_H
/*
 * main.h - GTK+ 2.x interface for Magnetic 2.x
 * Copyright (c) 2002 Torbj�rn Andersson <d91tan@Update.UU.SE>
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

#define _MAIN_H

extern gboolean application_exiting;

void start_main_loop (void);
void stop_main_loop (void);
gboolean main_loop (gpointer data);

gboolean start_new_game (gchar *game_filename, gchar *graphics_filename,
			 gchar *splash_filename, gchar *music_filename,
			 gchar *hints_filename);

void do_about (void);
void close_application (GtkWidget *widget, gpointer user_data);

#endif
