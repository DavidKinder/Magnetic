/*
 * main.c - GTK+ 2.x interface for Magnetic 2.x
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

#include <string.h>
#include <gtk/gtk.h>

#include "defs.h"
#include "main.h"
#include "version.h"
#include "config.h"
#include "gui.h"
#include "text.h"
#include "graphics.h"
#include "hints.h"
#include "util.h"

gboolean application_exiting = FALSE;

/* Exit the application */

void close_application (GtkWidget *widget, gpointer user_data)
{
    write_config_file ();
    stop_recording (TRUE);
    stop_scripting (TRUE);
    stop_replaying (TRUE);
    gtk_main_quit ();
    application_exiting = TRUE;
}

/* ------------------------------------------------------------------------- *
 * Main Loop                                                                 *
 * ------------------------------------------------------------------------- */

static gint main_idle_handler = -1;

void start_main_loop ()
{
    if (main_idle_handler == -1)
	main_idle_handler = gtk_idle_add (main_loop, NULL);
}

void stop_main_loop ()
{
    if (main_idle_handler != -1)
    {
	gtk_idle_remove (main_idle_handler);
	main_idle_handler = -1;
    }
}

/*
 * This function will be called as an idle handler, i.e. it will be called
 * whenever GTK+ doesn't have anything more important to do.
 */

gboolean main_loop (gpointer data)
{
    /*
     * Make sure we turn off the idle handler if the game isn't running. This
     * shouldn't happen.
     */
    if (!ms_is_running ())
    {
	g_warning ("main_loop was called while the game wasn't running");
	main_idle_handler = -1;
	return FALSE;
    }

    /*
     * Execute one opcode. If this brings the game to a halt, it should mean
     * that the user voluntarily quit the game.
     */
    
    if (!ms_rungame ())
    {
	text_insert ("\n[End of session]\n");
	main_idle_handler = -1;
	ms_flush ();
	ms_stop ();
	ms_freemem ();

	gtk_text_view_scroll_mark_onscreen (
	    GTK_TEXT_VIEW (GUI.text_view),
	    gtk_text_buffer_get_insert (GUI.text_buffer));
	return FALSE;
    }

    return TRUE;
}

type8 ms_load_file (type8s *name, type8 *ptr, type16 size)
{
    gchar *filename;
    GIOChannel *file = NULL;
    gsize bytes_read;
    GError *error = NULL;

    if (name == NULL)
    {
	filename = file_selector ("Restore game...");
	if (filename == NULL)
	    return -1;
    } else
	filename = g_strdup (name);

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
	file = g_io_channel_new_file (filename, "r", &error);
	g_io_channel_set_encoding (file, NULL, &error);
	g_io_channel_read_chars (file, ptr, size, &bytes_read, &error);
	g_io_channel_unref (file);
    }
    g_free (filename);
    return (file != NULL) ? 0 : -1;
}

type8 ms_save_file (type8s *name, type8 *ptr, type16 size)
{
    gchar *filename;
    GIOChannel *file;
    gsize bytes_written;
    GError *error = NULL;

    if (name == NULL)
    {
	filename = file_selector ("Save game...");
	if (filename == NULL)
	    return -1;
    } else
	filename = g_strdup (name);

    if (!warn_if_file_exists (filename))
    {
	g_free (filename);
	return -1;
    }
    
    file = g_io_channel_new_file (filename, "w", &error);
    g_io_channel_set_encoding (file, NULL, &error);
    g_io_channel_write_chars (file, ptr, size, &bytes_written, &error);
    g_io_channel_unref (file);
    g_free (filename);
    return 0;
}

void ms_fatal (type8s *txt)
{
    /* We really should do something more sensible here */
    g_warning ("Magnetic Fatal Error: %s", txt);
}

void do_about ()
{
    GtkWidget *about;

    about = gtk_message_dialog_new (
	GTK_WINDOW (GUI.main_window),
	GTK_DIALOG_DESTROY_WITH_PARENT,
	GTK_MESSAGE_INFO,
	GTK_BUTTONS_OK,
"Magnetic 2.1 - Magnetic Scrolls Interpreter.\n"
"Copyright (C) 1997-2002  Niclas Karlsson\n\n"
"Development Team\n"
"\tNiclas Karlsson <nkarlsso@ra.abo.fi>\n"
"\tDavid Kinder <davidk@monis.co.uk>\n"
"\tStefan Meier <Stefan.Meier@if-legends.org>\n"
"\tPaul David Doherty <pdd@if-legends.org>\n\n"
"GTK+ 2.x interface v" MAGNETIC_VERSION_GUI "\n"
"\tTorbj\303\266rn Andersson <d91tan@Update.UU.SE>");
    gtk_dialog_run (GTK_DIALOG (about));
    gtk_widget_destroy (about);
}

static gchar *change_file_extension (gchar *filename, gchar *extension)
{
    gchar *new_filename;
    gchar *ptr;

    ptr = strrchr (filename, '.');
    if (ptr != NULL)
	*ptr = '\0';

    new_filename = g_strconcat (filename, ".", extension, NULL);

    if (ptr != NULL)
	*ptr = '.';
    
    if (!g_file_test (new_filename, G_FILE_TEST_EXISTS))
    {
	for (ptr = strrchr (new_filename, '.'); *ptr; ptr++)
	    *ptr = g_ascii_toupper (*ptr);
    }

    return new_filename;
}

gboolean start_new_game (gchar *game_filename, gchar *graphics_filename,
			 gchar *splash_filename, gchar *music_filename,
			 gchar *hints_filename)
{
    if (game_filename == NULL)
	game_filename = file_selector ("Open game file...");

    if (game_filename == NULL)
	return TRUE;

    stop_main_loop ();

    if (ms_is_running ())
    {
	ms_stop ();
	ms_freemem ();
    }

    stop_recording (TRUE);
    stop_scripting (TRUE);
    stop_replaying (TRUE);

    if (graphics_filename == NULL)
	graphics_filename = change_file_extension (game_filename, "gfx");

    if (splash_filename == NULL)
	splash_filename = change_file_extension (game_filename, "png");
    
    if (music_filename == NULL)
	music_filename = change_file_extension (game_filename, "mp3");

    if (hints_filename == NULL)
	hints_filename = change_file_extension (game_filename, "hnt");

    display_splash_screen (splash_filename, music_filename);

    text_clear ();
    graphics_clear ();
    hints_clear ();

    if (application_exiting)
	return FALSE;

    if (!ms_init (game_filename, graphics_filename, hints_filename))
    {
	GtkWidget *error;
	gchar *basename;
	
	basename = g_path_get_basename (game_filename);
	error = gtk_message_dialog_new (
	    GTK_WINDOW (GUI.main_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_ERROR,
	    GTK_BUTTONS_OK,
	    "Could not start the game! The most likely cause is\n"
	    "that '%s' is not a valid game file.",
	    basename);
	gtk_dialog_run (GTK_DIALOG (error));
	g_free (basename);
	gtk_widget_destroy (error);
    } else
	start_main_loop ();
    
    g_free (game_filename);
    g_free (graphics_filename);
    g_free (splash_filename);
    g_free (music_filename);
    g_free (hints_filename);
    
    gtk_widget_grab_focus (GUI.text_view);
    return TRUE;
}

int main (int argc, char *argv[])
{
    gchar *game_filename = NULL;
    gchar *graphics_filename = NULL;
    gchar *splash_filename = NULL;
    gchar *music_filename = NULL;
    gchar *hints_filename = NULL;

    gtk_init (&argc, &argv);

    GUI_init ();
    text_init ();
    graphics_init ();

    read_config_file ();

    if (argc >= 6)
	hints_filename = g_strdup (argv[5]);
    if (argc >= 5)
	music_filename = g_strdup (argv[4]);
    if (argc >= 4)
	splash_filename = g_strdup (argv[3]);
    if (argc >= 3)
	graphics_filename = g_strdup (argv[2]);
    if (argc >= 2)
	game_filename = g_strdup (argv[1]);

    if (start_new_game (game_filename,
			graphics_filename,
			splash_filename,
			music_filename,
			hints_filename))
    {
	gtk_widget_show_all (GUI.main_window);
	gtk_main ();
    }

    return 0;  
}
