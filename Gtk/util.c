/*
 * util.c - Utility functions
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

#include <stdarg.h>
#include <gtk/gtk.h>

#include "gui.h"
#include "util.h"

gchar *file_selector (gchar *title_fmt, ...)
{
    GtkWidget *dialog;
    va_list args;
    const gchar *filename = NULL;
    gchar *title;
    gchar *copied_filename = NULL;
    gboolean done = FALSE;

    va_start (args, title_fmt);
    title = g_strdup_vprintf (title_fmt, args);
    va_end (args);

    dialog = gtk_file_selection_new (title);
    g_free (title);

    gtk_window_set_transient_for (
	GTK_WINDOW (dialog), GTK_WINDOW (GUI.main_window));

    while (!done)
    {
	if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK)
	{
	    filename = NULL;
	    break;
	}

	filename =
	    gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

	/* We never want directories, so check for that right away */
	if (g_file_test (filename, G_FILE_TEST_IS_DIR))
	{
	    GtkWidget *error;
	    gchar *basename;

	    basename = g_path_get_basename (filename);
	    error = gtk_message_dialog_new (
		GTK_WINDOW (GUI.main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_OK,
		"'%s' is a directory!",
		basename);
	    gtk_dialog_run (GTK_DIALOG (error));
	    g_free (basename);
	    gtk_widget_destroy (error);
	} else
	    done = TRUE;
    }

    if (filename != NULL)
	copied_filename = g_strdup (filename);

    gtk_widget_destroy (dialog);
    return copied_filename;
}

gboolean warn_if_file_exists (gchar *filename)
{
    gboolean ok = TRUE;
    
    if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
	GtkWidget *warning;
	gchar *basename;

	basename = g_path_get_basename (filename);
	warning = gtk_message_dialog_new (
	    GTK_WINDOW (GUI.main_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_YES_NO,
	    "The file '%s' already exists.\nContinue anyway?",
	    basename);
	ok = (gtk_dialog_run (GTK_DIALOG (warning)) == GTK_RESPONSE_YES);
	g_free (basename);
	gtk_widget_destroy (warning);
    }

    return ok;
}
