/*
 * gui.c - User interface
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

#include <gtk/gtk.h>

#include "main.h"
#include "config.h"
#include "gui.h"

struct GUI GUI;

/*
 * This should be automagically called every time the user changes the size
 * of the game window.
 */

static gboolean configure_window (GtkWidget *widget, GdkEventConfigure *event,
				  gpointer user_data)
{
    config.window_width = event->width;
    config.window_height = event->height;
    return FALSE;
}


void GUI_refresh ()
{
    gtk_window_set_default_size (
	GTK_WINDOW (GUI.main_window), config.window_width,
	config.window_height);
    gtk_paned_set_position (
	GTK_PANED (GUI.partition), config.window_split);
    gtk_widget_grab_focus (GUI.text_view);
}

enum
{
    MENU_OPEN = 1,
    MENU_QUIT,
    MENU_CONFIG,
    MENU_ABOUT
};

static void do_menu (gpointer callback_data, guint callback_action,
		     GtkWidget *widget)
{
    switch (callback_action)
    {
	case MENU_OPEN:
	    start_new_game (NULL, NULL, NULL, NULL, NULL);
	    break;
	    
	case MENU_QUIT:
	    close_application (NULL, NULL);
	    break;

	case MENU_CONFIG:
	    do_config ();
	    break;
	    
	case MENU_ABOUT:
	    do_about ();
	    break;

	default:
	    break;
    }
}

void GUI_init ()
{
    GtkItemFactoryEntry menu_items[] =
	{
	    { "/_File",                0, 0,       0,           "<Branch>",
	      0 },
	    { "/File/tearoff1",        0, 0,       0,           "<Tearoff>",
	      0 },
	    { "/File/_Open...",        0, do_menu, MENU_OPEN,   "<StockItem>",
	      GTK_STOCK_OPEN },
	    { "/File/_Preferences...", 0, do_menu, MENU_CONFIG, "<StockItem>",
	      GTK_STOCK_PREFERENCES },
	    { "/File/sep1",            0, 0,       0,           "<Separator>",
	      0 },
	    { "/File/_Quit",           0, do_menu, MENU_QUIT,   "<StockItem>",
	      GTK_STOCK_QUIT },

	    { "/_Help",                0, 0,       0,           "<LastBranch>",
	      0 },
	    { "/Help/tearoff1",        0, 0,       0,           "<Tearoff>",
	      0 },
	    { "/Help/_About",          0, do_menu, MENU_ABOUT,  "<Item>",
	      0 }
	};

    GtkAccelGroup *accel_group;
    GtkItemFactory *item_factory;
    GtkWidget *text_scroll;
    GtkWidget *box;

    GUI.main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (GUI.main_window), "GtkMagnetic");
    gtk_widget_set_size_request (GUI.main_window, MIN_WINDOW_WIDTH,
				 MIN_WINDOW_HEIGHT);

    g_signal_connect (G_OBJECT (GUI.main_window), "destroy",
		      G_CALLBACK (close_application), NULL);
    g_signal_connect (G_OBJECT (GUI.main_window), "configure_event",
		      G_CALLBACK (configure_window), NULL);

    /* The main "box" */

    GUI.main_box = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GUI.main_window), GUI.main_box);

    /* Menus */

    accel_group = gtk_accel_group_new ();
    item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
					 accel_group);
    g_object_set_data_full (G_OBJECT (GUI.main_window), "<main>",
			    item_factory, (GDestroyNotify) g_object_unref);
    gtk_window_add_accel_group (GTK_WINDOW (GUI.main_window), accel_group);
    gtk_item_factory_create_items (
	item_factory, G_N_ELEMENTS (menu_items), menu_items, NULL);
    gtk_box_pack_start (GTK_BOX (GUI.main_box),
			gtk_item_factory_get_widget (item_factory, "<main>"),
			FALSE, FALSE, 0);

    /* The status line */

    GUI.statusline.viewport = gtk_viewport_new (NULL, NULL);
    gtk_viewport_set_shadow_type (
	GTK_VIEWPORT (GUI.statusline.viewport), GTK_SHADOW_IN);
    gtk_box_pack_start (GTK_BOX (GUI.main_box), GUI.statusline.viewport,
			FALSE, TRUE, 0);
    
    box = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box), 3);
    gtk_container_add (GTK_CONTAINER (GUI.statusline.viewport), box);

    GUI.statusline.left = gtk_label_new (NULL);
    gtk_label_set_selectable (GTK_LABEL (GUI.statusline.left), TRUE);
    gtk_misc_set_alignment (GTK_MISC (GUI.statusline.left), 0.0, 0.0);
    gtk_box_pack_start (GTK_BOX (box), GUI.statusline.left, TRUE, TRUE, 0);

    GUI.statusline.right = gtk_label_new (NULL);
    gtk_label_set_selectable (GTK_LABEL (GUI.statusline.right), TRUE);
    gtk_misc_set_alignment (GTK_MISC (GUI.statusline.right), 1.0, 0.0);
    gtk_box_pack_start (GTK_BOX (box), GUI.statusline.right, FALSE, TRUE, 0);
    
    /* The game area; picture and text */
    
    GUI.partition = gtk_vpaned_new ();
    gtk_box_pack_start (GTK_BOX (GUI.main_box), GUI.partition, TRUE, TRUE, 0);

    GUI.picture_area = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (
	GTK_SCROLLED_WINDOW (GUI.picture_area), GTK_POLICY_AUTOMATIC,
	GTK_POLICY_AUTOMATIC);
    gtk_paned_add1 (GTK_PANED (GUI.partition), GUI.picture_area);

    text_scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (
	GTK_SCROLLED_WINDOW (text_scroll), GTK_POLICY_NEVER,
	GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (
	GTK_SCROLLED_WINDOW (text_scroll), GTK_SHADOW_IN);
    gtk_paned_add2 (GTK_PANED (GUI.partition), text_scroll);

    GUI.picture = gtk_image_new ();
    gtk_scrolled_window_add_with_viewport (
	GTK_SCROLLED_WINDOW (GUI.picture_area), GUI.picture);

    GUI.text_buffer = gtk_text_buffer_new (NULL);

    gtk_text_buffer_create_tag (
	GUI.text_buffer, "magnetic-input", "weight", PANGO_WEIGHT_BOLD,
	"editable", TRUE, NULL);
    gtk_text_buffer_create_tag (
	GUI.text_buffer, "magnetic-old-input", "weight", PANGO_WEIGHT_BOLD,
	"editable", FALSE, NULL);

    GUI.text_view = gtk_text_view_new_with_buffer (GUI.text_buffer);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (GUI.text_view), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (GUI.text_view), 3);
    gtk_text_view_set_right_margin (GTK_TEXT_VIEW (GUI.text_view), 3);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (GUI.text_view), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (GUI.text_view), TRUE);
    gtk_container_add (GTK_CONTAINER (text_scroll), GUI.text_view);
}
