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

GuiWidgets Gui;

/*
 * This should be automagically called every time the user changes the size
 * of the game window.
 */

static gboolean configure_window (GtkWidget *widget, GdkEventConfigure *event,
				  gpointer user_data)
{
    Config.window_width = event->width;
    Config.window_height = event->height;
    return FALSE;
}

#ifdef GTK3
void gui_refresh ()
{
    gtk_window_set_default_size (
    GTK_WINDOW (Gui.main_window), Config.window_width,
    Config.window_height);

    GtkWidget *child1 = gtk_paned_get_child1 (GTK_PANED (Gui.partition));
    GtkWidget *child2 = gtk_paned_get_child2 (GTK_PANED (Gui.partition));

    if (child1) g_object_ref (child1);
    if (child2) g_object_ref (child2);

    if (child1) gtk_container_remove (GTK_CONTAINER (Gui.partition), child1);
    if (child2) gtk_container_remove (GTK_CONTAINER (Gui.partition), child2);

    GtkWidget *new_partition = gtk_paned_new (
    Config.horizontal_split ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);

    if (Config.horizontal_split) {
    gtk_widget_set_size_request (new_partition, MIN_WINDOW_WIDTH, -1);
    if (child1) gtk_widget_set_size_request (child1, MIN_WINDOW_WIDTH/3, -1);
    if (child2) gtk_widget_set_size_request (child2, MIN_WINDOW_WIDTH/3, -1);
    } else {
    gtk_widget_set_size_request (new_partition, -1, MIN_WINDOW_HEIGHT);
    if (child1) gtk_widget_set_size_request (child1, -1, MIN_WINDOW_HEIGHT/3);
    if (child2) gtk_widget_set_size_request (child2, -1, MIN_WINDOW_HEIGHT/3);
    }

    gtk_container_remove (GTK_CONTAINER (Gui.main_box), Gui.partition);
    Gui.partition = new_partition;
    gtk_box_pack_start (GTK_BOX (Gui.main_box), Gui.partition, TRUE, TRUE, 0);

    if (child1) {
    gtk_paned_add1 (GTK_PANED(Gui.partition), child1);
    g_object_unref (child1);
    }
    if (child2) {
    gtk_paned_add2 (GTK_PANED (Gui.partition), child2);
    g_object_unref (child2);
    }

    gtk_paned_set_position (GTK_PANED (Gui.partition), Config.window_split);
    gtk_widget_show_all (Gui.partition);

    GtkWidget *scrolled_window = gtk_widget_get_parent (Gui.text_view);
    gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scrolled_window),
    GTK_POLICY_AUTOMATIC,
    GTK_POLICY_AUTOMATIC
    );

    gtk_widget_grab_focus(Gui.text_view);
}
#else
void gui_refresh ()
{
    gtk_window_set_default_size (
	GTK_WINDOW (Gui.main_window), Config.window_width,
	Config.window_height);
    gtk_paned_set_position (
	GTK_PANED (Gui.partition), Config.window_split);
    gtk_widget_grab_focus (Gui.text_view);
}
#endif

static void do_open ()
{
    start_new_game (NULL, NULL, NULL, NULL, NULL, NULL);
}

static void do_quit ()
{
    close_application (NULL, NULL);
}

#ifdef GTK3
static void create_menus (GtkWidget *main_box)
{
    GtkWidget *menubar;
    GtkWidget *menu;
    GtkWidget *menuitem;
    GtkWidget *submenu;

    menubar = gtk_menu_bar_new ();
    gtk_box_pack_start (GTK_BOX (main_box), menubar, FALSE, FALSE, 0);

    menu = gtk_menu_new ();
    menuitem = gtk_menu_item_new_with_mnemonic ("_File");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menuitem);

    submenu = gtk_menu_item_new_with_mnemonic ("_Open");
    g_signal_connect (G_OBJECT (submenu), "activate", G_CALLBACK (do_open), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

    submenu = gtk_menu_item_new_with_mnemonic ("_Preferences...");
    g_signal_connect (G_OBJECT (submenu), "activate", G_CALLBACK (do_config), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

    submenu = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

    submenu = gtk_menu_item_new_with_mnemonic ("_Quit");
    g_signal_connect (G_OBJECT (submenu), "activate", G_CALLBACK (do_quit), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

    menu = gtk_menu_new ();
    menuitem = gtk_menu_item_new_with_mnemonic ("_Help");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menuitem);

    submenu = gtk_menu_item_new_with_mnemonic ("_About");
    g_signal_connect (G_OBJECT (submenu), "activate", G_CALLBACK (do_about), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
}
#else
static GtkActionEntry menuEntries[] =
{
    { "FileMenu", NULL, "_File" },
    { "Open", GTK_STOCK_OPEN, "_Open", "<control>O", NULL, G_CALLBACK (do_open) },
    { "Prefs", GTK_STOCK_PREFERENCES, "_Preferences...", NULL, NULL, G_CALLBACK (do_config) },
    { "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", NULL, G_CALLBACK (do_quit) },
    { "HelpMenu", NULL, "_Help" },
    { "About", GTK_STOCK_ABOUT, "_About", NULL, NULL, G_CALLBACK (do_about) },
};

static const char *uiDescr =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem name='Open...' action='Open'/>"
"      <menuitem name='Preferences...' action='Prefs'/>"
"      <separator/>"
"      <menuitem name='Quit' action='Quit'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem name='About' action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";
#endif

void gui_init ()
{
#ifdef GTK2
    GtkUIManager *ui_manager;
    GtkActionGroup *action_group;
    GtkAccelGroup *accel_group;
    GError *error;

#endif
    GtkWidget *text_scroll;
    GtkWidget *box;
#ifdef GTK3
    GdkPixbuf *icon;
#endif

    Gui.main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (Gui.main_window), "GtkMagnetic");
#ifdef GTK3

    icon = gdk_pixbuf_new_from_file  ("gtkmagnetic.png", NULL);
    if (icon) {
        gtk_window_set_icon (GTK_WINDOW (Gui.main_window), icon);
        gtk_window_set_default_icon (icon);
        g_object_unref (icon);
    }
#endif
    gtk_widget_set_size_request (Gui.main_window, MIN_WINDOW_WIDTH,
				 MIN_WINDOW_HEIGHT);

    g_signal_connect (G_OBJECT (Gui.main_window), "destroy",
		      G_CALLBACK (close_application), NULL);
    g_signal_connect (G_OBJECT (Gui.main_window), "configure-event",
		      G_CALLBACK (configure_window), NULL);

    /* The main "box" */

#ifdef GTK3
    Gui.main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#else
    Gui.main_box = gtk_vbox_new (FALSE, 0);
#endif
    gtk_container_add (GTK_CONTAINER (Gui.main_window), Gui.main_box);

    /* Menus */
#ifdef GTK3
    create_menus (Gui.main_box);
#else
    action_group = gtk_action_group_new ("MenuActions");

    gtk_action_group_add_actions (
	action_group, menuEntries, G_N_ELEMENTS (menuEntries),
	Gui.main_window);

    ui_manager = gtk_ui_manager_new ();

    gtk_ui_manager_set_add_tearoffs (ui_manager, TRUE);

    accel_group = gtk_ui_manager_get_accel_group (ui_manager);
    gtk_window_add_accel_group (GTK_WINDOW (Gui.main_window), accel_group);
    gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

    error = NULL;
    if (!gtk_ui_manager_add_ui_from_string (ui_manager, uiDescr, -1, &error))
    {
	/* This is bad, but not catastrophic. Keep running. */
	g_message ("Building menus failed: %s", error->message);
	g_error_free (error);
    }

    gtk_box_pack_start (
	GTK_BOX (Gui.main_box),
	gtk_ui_manager_get_widget (ui_manager, "/MenuBar"),
	FALSE, FALSE, 0);
#endif

    /* The status line */

    Gui.statusline.viewport = gtk_viewport_new (NULL, NULL);
    gtk_viewport_set_shadow_type (
	GTK_VIEWPORT (Gui.statusline.viewport), GTK_SHADOW_IN);
    gtk_box_pack_start (GTK_BOX (Gui.main_box), Gui.statusline.viewport,
			FALSE, TRUE, 0);
    
#ifdef GTK3
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#else
    box = gtk_hbox_new (FALSE, 0);
#endif
    gtk_container_set_border_width (GTK_CONTAINER (box), 3);
    gtk_container_add (GTK_CONTAINER (Gui.statusline.viewport), box);

    Gui.statusline.left = gtk_label_new (NULL);
    gtk_label_set_selectable (GTK_LABEL (Gui.statusline.left), TRUE);
#ifdef GTK3
    gtk_label_set_xalign (GTK_LABEL (Gui.statusline.left), 0.0);
    gtk_label_set_yalign (GTK_LABEL (Gui.statusline.left), 0.0);
#else
    gtk_misc_set_alignment (GTK_MISC (Gui.statusline.left), 0.0, 0.0);
#endif
    gtk_box_pack_start (GTK_BOX (box), Gui.statusline.left, TRUE, TRUE, 0);

    Gui.statusline.right = gtk_label_new (NULL);
    gtk_label_set_selectable (GTK_LABEL (Gui.statusline.right), TRUE);
#ifdef GTK3
    gtk_label_set_xalign (GTK_LABEL (Gui.statusline.right), 1.0);
    gtk_label_set_yalign (GTK_LABEL (Gui.statusline.right), 0.0);
#else
    gtk_misc_set_alignment (GTK_MISC (Gui.statusline.right), 1.0, 0.0);
#endif
    gtk_box_pack_start (GTK_BOX (box), Gui.statusline.right, FALSE, TRUE, 0);
    
    /* The game area; picture and text */
    
#ifdef GTK3
    Gui.partition = gtk_paned_new (Config.horizontal_split ?
                                 GTK_ORIENTATION_HORIZONTAL :
                                 GTK_ORIENTATION_VERTICAL);
#else
    Gui.partition = gtk_vpaned_new ();
#endif
    gtk_box_pack_start (GTK_BOX (Gui.main_box), Gui.partition, TRUE, TRUE, 0);

    Gui.picture_area = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (
	GTK_SCROLLED_WINDOW (Gui.picture_area), GTK_POLICY_AUTOMATIC,
	GTK_POLICY_AUTOMATIC);
    gtk_paned_add1 (GTK_PANED (Gui.partition), Gui.picture_area);

    text_scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (
	GTK_SCROLLED_WINDOW (text_scroll), GTK_POLICY_NEVER,
	GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (
	GTK_SCROLLED_WINDOW (text_scroll), GTK_SHADOW_IN);
    gtk_paned_add2 (GTK_PANED (Gui.partition), text_scroll);

    Gui.picture = gtk_image_new ();
#ifdef GTK3
    gtk_container_add (GTK_CONTAINER (Gui.picture_area), Gui.picture);
#else
    gtk_scrolled_window_add_with_viewport (
	GTK_SCROLLED_WINDOW (Gui.picture_area), Gui.picture);
#endif

    Gui.text_buffer = gtk_text_buffer_new (NULL);

    gtk_text_buffer_create_tag (
	Gui.text_buffer, "magnetic-input", "weight", PANGO_WEIGHT_BOLD,
	"editable", TRUE, NULL);
    gtk_text_buffer_create_tag (
	Gui.text_buffer, "magnetic-old-input", "weight", PANGO_WEIGHT_BOLD,
	"editable", FALSE, NULL);
#ifdef GTK3
    gtk_text_buffer_create_tag (
    Gui.text_buffer, "magnetic-input-padding",
    "weight", PANGO_WEIGHT_BOLD, "editable", TRUE, NULL);
#else
    gtk_text_buffer_create_tag (
	Gui.text_buffer, "magnetic-input-padding",
#ifdef USE_INVISIBLE_TEXT
	"invisible", TRUE,
#endif
	"weight", PANGO_WEIGHT_BOLD, "editable", TRUE, NULL);
#endif

    Gui.text_view = gtk_text_view_new_with_buffer (Gui.text_buffer);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (Gui.text_view), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (Gui.text_view), 3);
    gtk_text_view_set_right_margin (GTK_TEXT_VIEW (Gui.text_view), 3);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (Gui.text_view), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (Gui.text_view), TRUE);
    gtk_container_add (GTK_CONTAINER (text_scroll), Gui.text_view);
}
