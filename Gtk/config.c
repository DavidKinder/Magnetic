/*
 * config.c - Storing to and retrieving from the configuration file
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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "config.h"
#include "gui.h"
#include "text.h"
#include "graphics.h"

#define CONFIG_VERSION_MAJOR  1
#define CONFIG_VERSION_MINOR  1

static GtkWidget *config_dialog;

/* ------------------------------------------------------------------------- *
 * The configuration data structure. There is no guarantee that all of these *
 * settings will be up-to-date at all times. In particular, I haven't found  *
 * any way of detecting when the window partition changes.                   *
 * ------------------------------------------------------------------------- */

struct configuration config =
{
    2 * MIN_WINDOW_WIDTH,   /* window width                         */
    2 * MIN_WINDOW_HEIGHT,  /* window height                        */
    150,                    /* partition                            */
    MIN_WINDOW_WIDTH,       /* hints window width                   */
    MIN_WINDOW_HEIGHT,      /* hints window height                  */
    NULL,                   /* text font                            */
    NULL,                   /* text foreground colour               */
    NULL,                   /* text background colour               */
    NULL,                   /* statusline foreground colour         */
    NULL,                   /* statusline background colour         */
    FALSE,                  /* scale image to constant height?      */
    1.0,                    /* image scaling                        */
    300,                    /* constant image height                */
    GDK_INTERP_BILINEAR,    /* interpolation mode for image scaling */
    1.0,                    /* red gamma                            */
    1.0,                    /* green gamma                          */
    1.0,                    /* blue gamma                           */
    NULL,                   /* graphics background colour           */
    TRUE,                   /* animate images                       */
    100                     /* animation delay (ms)                 */
};

/* ------------------------------------------------------------------------- *
 * Utility functions                                                         *
 * ------------------------------------------------------------------------- */

static gchar *get_config_filename ()
{
    return g_strjoin ("/", g_get_home_dir (), ".gtkmagnetic", NULL);
}

/* ------------------------------------------------------------------------- *
 * Configuration file writer.                                                *
 * ------------------------------------------------------------------------- */

void write_config_file ()
{
    gchar *filename;
    GIOChannel *file;
    GError *error = NULL;

    /*
     * HACK: As mentioned before, I don't know how to detect when the window
     * partition changes, so this is the only opportunity to make sure the
     * setting is up to date.
     */
    config.window_split = gtk_paned_get_position (GTK_PANED (GUI.partition));
    
    filename = get_config_filename ();
    file = g_io_channel_new_file (filename, "w", &error);
    g_free (filename);

    if (file != NULL)
    {
	gchar *buf;
	gsize bytes_written;

	buf = g_strdup_printf (
	    "<?xml version=\"1.0\"?>\n\n"
	    "<configuration version=\"1.1\">\n"
	    "  <layout>\n"
	    "    <main_window>\n"
	    "      <width>%d</width>\n"
	    "      <height>%d</height>\n"
	    "      <split>%d</split>\n"
	    "    </main_window>\n\n"
	    
	    "    <hints_window>\n"
	    "      <width>%d</width>\n"
	    "      <height>%d</height>\n"
	    "    </hints_window>\n"
	    "  </layout>\n\n"
	    
	    "  <text>\n"
	    "    <font>%s</font>\n"
	    "    <foreground>%s</foreground>\n"
	    "    <background>%s</background>\n"
	    "  </text>\n\n"
	    
	    "  <statusline>\n"
	    "    <foreground>%s</foreground>\n"
	    "    <background>%s</background>\n"
	    "  </statusline>\n\n"
	    
	    "  <graphics>\n"
	    "    <constant_height>%s</constant_height>\n"
	    "    <scale>%f</scale>\n"
	    "    <height>%d</height>\n"
	    "    <filter>%d</filter>\n"
	    "    <gamma>\n"
	    "      <red>%f</red>\n"
	    "      <green>%f</green>\n"
	    "      <blue>%f</blue>\n"
	    "    </gamma>\n"
	    "    <background>%s</background>\n"
	    "    <animate>%s</animate>\n"
	    "    <delay>%d</delay>\n"
	    "  </graphics>\n"
	    "</configuration>\n",
	    config.window_width,
	    config.window_height,
	    config.window_split,
	    config.hints_width,
	    config.hints_height,
	    (config.text_font != NULL) ? config.text_font : "",
	    (config.text_fg != NULL) ? config.text_fg : "",
	    (config.text_bg != NULL) ? config.text_bg : "",
	    (config.statusline_fg != NULL) ? config.statusline_fg : "",
	    (config.statusline_bg != NULL) ? config.statusline_bg : "",
	    config.image_constant_height ? "TRUE" : "FALSE",
	    config.image_scale,
	    config.image_height,
	    config.image_filter,
	    config.red_gamma,
	    config.green_gamma,
	    config.blue_gamma,
	    (config.graphics_bg != NULL) ? config.graphics_bg : "",
	    config.animate_images ? "TRUE" : "FALSE",
	    config.animation_delay);

	g_io_channel_write_chars (file, buf, -1, &bytes_written, &error);
	g_free (buf);
	g_io_channel_unref (file);
    }
}

/* ------------------------------------------------------------------------- *
 * Configuration file reader/parser. The parser_state variable decides which *
 * tags are currently expected, and one of the parser_float, parser_int and  *
 * parser_char variables is set to point at the location where the parsed    *
 * value should be stored. Unknown tags are silently ignored.                *
 *                                                                           *
 * This makes it very simple to add new tags or even to change the expected  *
 * layout of the configuration file. I'm quite pleased with the way this     *
 * code turned out.                                                          *
 * ------------------------------------------------------------------------- */

static enum
{
    CONFIG_PARSE_TOPLEVEL,
    CONFIG_PARSE_CONFIGURATION,
    CONFIG_PARSE_LAYOUT,
    CONFIG_PARSE_LAYOUT_MAIN_WINDOW,
    CONFIG_PARSE_LAYOUT_HINTS_WINDOW,
    CONFIG_PARSE_TEXT,
    CONFIG_PARSE_STATUSLINE,
    CONFIG_PARSE_GRAPHICS,
    CONFIG_PARSE_GRAPHICS_GAMMA
} parser_state = CONFIG_PARSE_TOPLEVEL;

static gboolean *parser_bool = NULL;
static gint *parser_int = NULL;
static gdouble *parser_float = NULL;
static gchar **parser_char = NULL;

static void config_parse_reset ()
{
    parser_bool = NULL;
    parser_int = NULL;
    parser_float = NULL;
    parser_char = NULL;
}

/* Parse opening tag and attributes */

static void config_parse_start_element (GMarkupParseContext *context,
					const gchar *element_name,
					const gchar **attribute_names,
					const gchar **attribute_values,
					gpointer user_data,
					GError **error)
{
    config_parse_reset ();
    
    switch (parser_state)
    {
	case CONFIG_PARSE_TOPLEVEL:
	    if (strcmp (element_name, "configuration") == 0)
	    {
		gint version_major = 0;
		gint version_minor = 0;
		gint i;
		
		parser_state = CONFIG_PARSE_CONFIGURATION;

		for (i = 0; attribute_names[i] != NULL; i++)
		{
		    if (strcmp (attribute_names[i], "version") == 0)
			sscanf (attribute_values[i], "%d.%d", &version_major,
				&version_minor);
		}

		if (version_major != CONFIG_VERSION_MAJOR ||
		    version_minor != CONFIG_VERSION_MINOR)
		{
		    g_message (
			"Config version mismatch. Expected %d.%d, found %d.%d",
			CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR,
			version_major, version_minor);
		    if (version_major == CONFIG_VERSION_MAJOR &&
			version_minor <= CONFIG_VERSION_MINOR)
			g_message ("This should be quite harmless.");
		    else
			g_message (
			    "Some of your settings have probably been lost.");
		}
	    }
	    break;
	    
	case CONFIG_PARSE_CONFIGURATION:
	    if (strcmp (element_name, "layout") == 0)
		parser_state = CONFIG_PARSE_LAYOUT;
	    else if (strcmp (element_name, "text") == 0)
		parser_state = CONFIG_PARSE_TEXT;
	    else if (strcmp (element_name, "statusline") == 0)
		parser_state = CONFIG_PARSE_STATUSLINE;
	    else if (strcmp (element_name, "graphics") == 0)
		parser_state = CONFIG_PARSE_GRAPHICS;
	    break;

	case CONFIG_PARSE_LAYOUT:
	    if (strcmp (element_name, "main_window") == 0)
		parser_state = CONFIG_PARSE_LAYOUT_MAIN_WINDOW;
	    else if (strcmp (element_name, "hints_window") == 0)
		parser_state = CONFIG_PARSE_LAYOUT_HINTS_WINDOW;
	    break;

	case CONFIG_PARSE_LAYOUT_MAIN_WINDOW:
	    if (strcmp (element_name, "width") == 0)
		parser_int = &(config.window_width);
	    else if (strcmp (element_name, "height") == 0)
		parser_int = &(config.window_height);
	    else if (strcmp (element_name, "split") == 0)
		parser_int = &(config.window_split);
	    break;

	case CONFIG_PARSE_LAYOUT_HINTS_WINDOW:
	    if (strcmp (element_name, "width") == 0)
		parser_int = &(config.hints_width);
	    else if (strcmp (element_name, "height") == 0)
		parser_int = &(config.hints_height);
	    break;

	case CONFIG_PARSE_TEXT:
	    if (strcmp (element_name, "font") == 0)
		parser_char = &(config.text_font);
	    else if (strcmp (element_name, "background") == 0)
		parser_char = &(config.text_bg);
	    else if (strcmp (element_name, "foreground") == 0)
		parser_char = &(config.text_fg);
	    break;

	case CONFIG_PARSE_STATUSLINE:
	    if (strcmp (element_name, "foreground") == 0)
		parser_char = &(config.statusline_fg);
	    else if (strcmp (element_name, "background") == 0)
		parser_char = &(config.statusline_bg);
	    break;
	    
	case CONFIG_PARSE_GRAPHICS:
	    if (strcmp (element_name, "constant_height") == 0)
		parser_bool = &(config.image_constant_height);
	    else if (strcmp (element_name, "scale") == 0)
		parser_float = &(config.image_scale);
	    else if (strcmp (element_name, "height") == 0)
		parser_int = &(config.image_height);
	    else if (strcmp (element_name, "filter") == 0)
		parser_int = &(config.image_filter);
	    else if (strcmp (element_name, "gamma") == 0)
		parser_state = CONFIG_PARSE_GRAPHICS_GAMMA;
	    else if (strcmp (element_name, "background") == 0)
		parser_char = &(config.graphics_bg);
	    else if (strcmp (element_name, "animate") == 0)
		parser_bool = &(config.animate_images);
	    else if (strcmp (element_name, "delay") == 0)
		parser_int = &(config.animation_delay);
	    break;

	case CONFIG_PARSE_GRAPHICS_GAMMA:
	    if (strcmp (element_name, "red") == 0)
		parser_float = &(config.red_gamma);
	    else if (strcmp (element_name, "green") == 0)
		parser_float = &(config.green_gamma);
	    else if (strcmp (element_name, "blue") == 0)
		parser_float = &(config.blue_gamma);
	    break;

	default:
	    break;
    }
}

/* Parse closing tag */

static void config_parse_end_element (GMarkupParseContext *context,
				      const gchar *element_name,
				      gpointer user_data,
				      GError **error)
{
    config_parse_reset ();
    
    switch (parser_state)
    {
	case CONFIG_PARSE_CONFIGURATION:
	    if (strcmp (element_name, "configuration") == 0)
		parser_state = CONFIG_PARSE_TOPLEVEL;
	    break;

	case CONFIG_PARSE_LAYOUT:
	    if (strcmp (element_name, "layout") == 0)
		parser_state = CONFIG_PARSE_CONFIGURATION;
	    break;

	case CONFIG_PARSE_LAYOUT_MAIN_WINDOW:
	    if (strcmp (element_name, "main_window") == 0)
		parser_state = CONFIG_PARSE_LAYOUT;
	    break;

	case CONFIG_PARSE_LAYOUT_HINTS_WINDOW:
	    if (strcmp (element_name, "hints_window") == 0)
		parser_state = CONFIG_PARSE_LAYOUT;
	    break;
	    
	case CONFIG_PARSE_TEXT:
	    if (strcmp (element_name, "text") == 0)
		parser_state = CONFIG_PARSE_CONFIGURATION;
	    break;
	    
	case CONFIG_PARSE_GRAPHICS:
	    if (strcmp (element_name, "graphics") == 0)
		parser_state = CONFIG_PARSE_CONFIGURATION;
	    break;

	case CONFIG_PARSE_GRAPHICS_GAMMA:
	    if (strcmp (element_name, "gamma") == 0)
		parser_state = CONFIG_PARSE_GRAPHICS;
	    break;

	case CONFIG_PARSE_STATUSLINE:
	    if (strcmp (element_name, "statusline") == 0)
		parser_state = CONFIG_PARSE_CONFIGURATION;
	    break;

	default:
	    break;
    }
}

/* Parse text between tags */

static void config_parse_text (GMarkupParseContext *context,
			       const gchar *text,
			       gsize text_len,
			       gpointer user_data,
			       GError **error)
{
    if (parser_bool != NULL)
	*parser_bool = (strcmp (text, "TRUE") == 0) ? TRUE : FALSE;
    else if (parser_int != NULL)
	*parser_int = (gint) g_ascii_strtod (text, NULL);
    else if (parser_float != NULL)
	*parser_float = g_ascii_strtod (text, NULL);
    else if (parser_char != NULL)
    {
	if (*parser_char != NULL)
	    g_free (*parser_char);
	if (strlen (text) > 0)
	    *parser_char = g_strdup (text);
    }
}

/*
 * Error handling. We really ought to do something more useful here, but it
 * shouldn't ever happen...
 */

static void config_parse_error (GMarkupParseContext *context,
				GError *error,
				gpointer user_data)
{
    g_warning ("Config file parser error: %s", error->message);
}

void read_config_file ()
{
    GMarkupParseContext *context;
    GMarkupParser parser =
	{
	    config_parse_start_element,
	    config_parse_end_element,
	    config_parse_text,
	    NULL,
	    config_parse_error
	};
    gchar *filename;
    gchar *text;
    gsize length;
    GError *error = NULL;

    filename = get_config_filename ();
    g_file_get_contents (filename, &text, &length, &error);
    g_free (filename);

    if (text != NULL)
    {
	context = g_markup_parse_context_new (&parser, 0, NULL, NULL);
	g_markup_parse_context_parse (context, text, -1, &error);
	g_markup_parse_context_end_parse (context, &error);
	g_markup_parse_context_free (context);

	if (parser_state != CONFIG_PARSE_TOPLEVEL)
	    g_warning ("Unexpected parser state at end of config file");
    }

    /* Apply the settings from the configuration file */

    text_refresh ();
    graphics_refresh ();
    GUI_refresh ();
}

/* ------------------------------------------------------------------------- *
 * Configuration window.                                                     *
 * ------------------------------------------------------------------------- */

static GtkWidget *add_scale (GtkWidget *tab, gchar *text, gdouble min,
			     gdouble max, gdouble step, gdouble value)
{
    GtkWidget *scale;
    GtkWidget *label;

    label = gtk_label_new (text);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (tab), label, TRUE, TRUE, 0);
    
    scale = gtk_hscale_new_with_range (min, max, step);
    gtk_scale_set_digits (GTK_SCALE (scale), 2);
    gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
    gtk_range_set_value (GTK_RANGE (scale), value );
    gtk_box_pack_start (GTK_BOX (tab), scale, TRUE, TRUE, 0);

    return scale;
}

struct colour_setting
{
    GtkWidget *display_widget;
    gchar *colour_spec;
    GdkColor colour;
    gboolean changed;
};

static void update_colour_setting (char **spec_ptr, GdkColor *colour)
{
    if (*spec_ptr != NULL)
	g_free (*spec_ptr);

    /*
     * This won't always give the exact same colour string as the one
     * displayed in the dialog window. I'm not sure why, but the
     * difference is small enough that it shouldn't make a difference.
     */
    *spec_ptr = gtk_color_selection_palette_to_string (colour, 1);
}

static void select_colour (GtkButton *button, gpointer user_data)
{
    struct colour_setting *setting = (struct colour_setting *) user_data;
    GtkWidget *dialog;
    GdkColor colour;

    dialog = gtk_color_selection_dialog_new ("Select colour");
    gtk_window_set_transient_for (GTK_WINDOW (dialog),
				  GTK_WINDOW (config_dialog));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

    if (setting->colour_spec != NULL)
	gtk_color_selection_set_current_color (
	    GTK_COLOR_SELECTION (
		GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel),
	    &(setting->colour));
    
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
	setting->changed = TRUE;
	gtk_color_selection_get_current_color (
	    GTK_COLOR_SELECTION (
		GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel), &colour);

	memcpy (&(setting->colour), &colour, sizeof (GdkColor));
	gtk_widget_modify_base (
	    setting->display_widget, GTK_STATE_INSENSITIVE, &colour);
    }

    gtk_widget_destroy (dialog);
}

static GtkWidget *add_colour_setting (GtkWidget *tab, gchar *text,
				      struct colour_setting *setting)
{
    GtkWidget *colour_area;
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *button;

    setting->changed = FALSE;
    
    label = gtk_label_new (text);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (tab), label, TRUE, TRUE, 0);

    box = gtk_hbox_new (FALSE, 10);
    gtk_box_pack_start (GTK_BOX (tab), box, TRUE, TRUE, 0);
    
    /*
     * I can't find any good widget for a filled rectangle, so I'll use a
     * insensitive entry field instead.
     */
    colour_area = gtk_entry_new ();
    gtk_widget_set_sensitive (colour_area, FALSE);
    gtk_box_pack_start (GTK_BOX (box), colour_area, TRUE, TRUE, 0);

    button = gtk_button_new_with_label ("Change...");
    gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

    g_signal_connect (
	G_OBJECT (button), "clicked", G_CALLBACK (select_colour), setting);

    if (setting->colour_spec != NULL
	&& gdk_color_parse (setting->colour_spec, &(setting->colour)))
	gtk_widget_modify_base (colour_area, GTK_STATE_INSENSITIVE,
				&(setting->colour));
    else
	gtk_entry_set_text (GTK_ENTRY (colour_area), "- theme default -");
    
    return colour_area;
}

static gulong sig_scale_changed = 0;

static GtkWidget *image_scale_label;
static GtkWidget *image_scale;

static gint tmp_image_height;
static gfloat tmp_image_scale;

static void toggle_constant_height (GtkToggleButton *togglebutton,
				    gpointer user_data)
{
    /*
     * Apparently changing the image scale the way we do below will cause
     * the "value_changed" signal to be emitted, which will screw things up
     * quite badly. So we block that signal temporarily.
     */
    
    g_signal_handler_block (G_OBJECT (image_scale), sig_scale_changed);
    
    if (gtk_toggle_button_get_active (togglebutton))
    {
	gtk_label_set_text (GTK_LABEL (image_scale_label), "Image height:");
	gtk_scale_set_digits (GTK_SCALE (image_scale), 0);
	gtk_range_set_range (GTK_RANGE (image_scale), 50, 1000);
	gtk_range_set_increments (GTK_RANGE (image_scale), 1.0, 50.0);
	gtk_range_set_value (GTK_RANGE (image_scale), tmp_image_height);
    } else
    {
	gtk_label_set_text (GTK_LABEL (image_scale_label), "Scale factor:");
	gtk_scale_set_digits (GTK_SCALE (image_scale), 2);
	gtk_range_set_range (GTK_RANGE (image_scale), 0.1, 5.0);
	gtk_range_set_increments (GTK_RANGE (image_scale), 0.01, 0.1);
	gtk_range_set_value (GTK_RANGE (image_scale), tmp_image_scale);
    }

    g_signal_handler_unblock (G_OBJECT (image_scale), sig_scale_changed);
}

static void change_image_scale (GtkRange *range, gpointer user_data)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (user_data)))
	tmp_image_height = (gint) gtk_range_get_value (range);
    else
	tmp_image_scale = gtk_range_get_value (range);
}

struct menu_item
{
    GtkWidget *widget;
    gchar *text;
    gint value;
};

void do_config ()
{
    static struct colour_setting text_fg;
    static struct colour_setting text_bg;
    static struct colour_setting statusline_fg;
    static struct colour_setting statusline_bg;
    static struct colour_setting graphics_bg;

    GtkWidget *dummy;
    GtkWidget *tabs;
    GtkWidget *font_tab;
    GtkWidget *graphics_tab;
    GtkWidget *colour_tab;
    GtkWidget *font_selection;
    GtkWidget *constant_height;
    GtkWidget *red_gamma;
    GtkWidget *green_gamma;
    GtkWidget *blue_gamma;
    GtkWidget *animate_images;
    GtkWidget *animation_delay;
    GtkWidget *image_filter;
    GtkWidget *menu;
    struct menu_item menu_items[] =
    {
	/*
	 * In reality, the value and index are probably the same. But even
	 * so, I don't want to lock myself into that assumption.
	 */
	{ NULL, "Nearest neighbor", GDK_INTERP_NEAREST  },
	{ NULL, "Tiles",            GDK_INTERP_TILES    },
	{ NULL, "Bilinear",         GDK_INTERP_BILINEAR },
	{ NULL, "Hyperbolic",       GDK_INTERP_HYPER    }
    };
    GSList *menu_group = NULL;
    gint filter_idx = 0;
    gint i;

    /*
     * Some settings, such as the partition, may have changed since they were
     * last restored. Save them now so the config dialog won't accidentally
     * restore the old values.
     */
    write_config_file ();
    
    config_dialog = gtk_dialog_new_with_buttons (
	"Preferences",
	GTK_WINDOW (GUI.main_window),
	GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	GTK_STOCK_OK,
	GTK_RESPONSE_ACCEPT,
	GTK_STOCK_CANCEL,
	GTK_RESPONSE_REJECT,
	NULL);

    /* Top level */

    tabs = gtk_notebook_new ();
    gtk_box_pack_start (
	GTK_BOX (GTK_DIALOG (config_dialog)->vbox), tabs, TRUE, TRUE, 0);

    font_tab = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (font_tab), 5);
    gtk_notebook_append_page (GTK_NOTEBOOK (tabs), font_tab,
			      gtk_label_new ("Text"));

    graphics_tab = gtk_vbox_new (FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (graphics_tab), 5);
    gtk_notebook_append_page (GTK_NOTEBOOK (tabs), graphics_tab,
			      gtk_label_new ("Graphics"));

    colour_tab = gtk_vbox_new (FALSE, 3);
    gtk_container_set_border_width (GTK_CONTAINER (colour_tab), 5);
    gtk_notebook_append_page (GTK_NOTEBOOK (tabs), colour_tab,
			      gtk_label_new ("Colour"));

    /* Text settings */

    font_selection = gtk_font_selection_new ();

    if (config.text_font == NULL)
    {
	GtkStyle *style;
	gchar *font_name;

	style = gtk_widget_get_style (GUI.text_view);
	font_name = pango_font_description_to_string (style->font_desc);
	gtk_font_selection_set_font_name (
	    GTK_FONT_SELECTION (font_selection), font_name);
	g_free (font_name);
    } else
	gtk_font_selection_set_font_name (
	    GTK_FONT_SELECTION (font_selection), config.text_font);

    gtk_box_pack_start (GTK_BOX (font_tab), font_selection, TRUE, TRUE, 0);
    
    /* Picture settings */

    constant_height = gtk_check_button_new_with_label (
	"Scale image to constant height.");
    gtk_toggle_button_set_active (
	GTK_TOGGLE_BUTTON (constant_height), config.image_constant_height);
    gtk_box_pack_start (
	GTK_BOX (graphics_tab), constant_height, TRUE, TRUE, 0);

    g_signal_connect (
	G_OBJECT (constant_height), "toggled",
	G_CALLBACK (toggle_constant_height), NULL);

    image_scale_label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (image_scale_label), 0.0, 0.5);
    gtk_box_pack_start (
	GTK_BOX (graphics_tab), image_scale_label, TRUE, TRUE, 0);

    image_scale = gtk_hscale_new (NULL);
    gtk_scale_set_value_pos (GTK_SCALE (image_scale), GTK_POS_RIGHT);
    gtk_box_pack_start (GTK_BOX (graphics_tab), image_scale, TRUE, TRUE, 0);

    sig_scale_changed = g_signal_connect (
	G_OBJECT (image_scale), "value_changed",
	G_CALLBACK (change_image_scale), constant_height);
    
    tmp_image_scale = config.image_scale;
    tmp_image_height = config.image_height;
    
    toggle_constant_height (GTK_TOGGLE_BUTTON (constant_height), NULL);

    dummy = gtk_label_new ("Interpolation mode:");
    gtk_misc_set_alignment (GTK_MISC (dummy), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (graphics_tab), dummy, TRUE, TRUE, 0);

    image_filter = gtk_option_menu_new ();
    gtk_box_pack_start (GTK_BOX (graphics_tab), image_filter, TRUE, TRUE, 0);

    menu = gtk_menu_new ();

    for (i = 0; i < sizeof (menu_items) / sizeof (struct menu_item); i++)
    {
	menu_items[i].widget = gtk_radio_menu_item_new_with_label (
	    menu_group, menu_items[i].text);
	menu_group = gtk_radio_menu_item_get_group (
	    GTK_RADIO_MENU_ITEM (menu_items[i].widget));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_items[i].widget);

	if (config.image_filter == menu_items[i].value)
	    filter_idx = i;
    }

    gtk_option_menu_set_menu (GTK_OPTION_MENU (image_filter), menu);

    gtk_check_menu_item_set_active (
	GTK_CHECK_MENU_ITEM (menu_items[filter_idx].widget), TRUE);
    gtk_option_menu_set_history (GTK_OPTION_MENU (image_filter), filter_idx);

    dummy = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (graphics_tab), dummy, TRUE, TRUE, 8);

    red_gamma = add_scale (
	graphics_tab, "Red gamma:", 0.1, 5.0, 0.1, config.red_gamma);
    green_gamma = add_scale (
	graphics_tab, "Green gamma:", 0.1, 5.0, 0.1, config.green_gamma);
    blue_gamma = add_scale (
	graphics_tab, "Blue gamma:", 0.1, 5.0, 0.1, config.blue_gamma);

    dummy = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (graphics_tab), dummy, TRUE, TRUE, 8);

    animate_images = gtk_check_button_new_with_label ("Animate images.");
    gtk_box_pack_start (GTK_BOX (graphics_tab), animate_images, TRUE, TRUE, 0);
    gtk_toggle_button_set_active (
	GTK_TOGGLE_BUTTON (animate_images), config.animate_images);
    
    animation_delay = add_scale (
	graphics_tab, "Animation delay (ms):", 50.0, 500.0, 10.0,
	config.animation_delay);
    gtk_scale_set_digits (GTK_SCALE (animation_delay), 0);
    
    /* Colour settings */

    text_fg.colour_spec = config.text_fg;
    text_bg.colour_spec = config.text_bg;
    statusline_fg.colour_spec = config.statusline_fg;
    statusline_bg.colour_spec = config.statusline_bg;
    graphics_bg.colour_spec = config.graphics_bg;
    
    text_fg.display_widget = add_colour_setting (
	colour_tab, "Text foreground:", &text_fg);
    text_bg.display_widget = add_colour_setting (
	colour_tab, "Text background:", &text_bg);
    statusline_fg.display_widget = add_colour_setting (
	colour_tab, "Status foreground:", &statusline_fg);
    statusline_bg.display_widget = add_colour_setting (
	colour_tab, "Status background:", &statusline_bg);
    graphics_bg.display_widget = add_colour_setting (
	colour_tab, "Picture background:", &graphics_bg);
    
    /* Run the dialog */

    gtk_widget_show_all (GTK_WIDGET (GTK_DIALOG (config_dialog)->vbox));

    if (gtk_dialog_run (GTK_DIALOG (config_dialog)) == GTK_RESPONSE_ACCEPT)
    {
	if (config.text_font != NULL)
	    g_free (config.text_font);
	config.text_font = gtk_font_selection_get_font_name (
	    GTK_FONT_SELECTION (font_selection));

	config.image_constant_height =
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (constant_height));
	config.image_scale = tmp_image_scale;
	config.image_height = tmp_image_height;
	config.red_gamma = gtk_range_get_value (GTK_RANGE (red_gamma));
	config.green_gamma = gtk_range_get_value (GTK_RANGE (green_gamma));
	config.blue_gamma = gtk_range_get_value (GTK_RANGE (blue_gamma));

	config.animate_images =
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (animate_images));
	config.animation_delay =
	    (gint) gtk_range_get_value (GTK_RANGE (animation_delay));

	filter_idx =
	    gtk_option_menu_get_history (GTK_OPTION_MENU (image_filter));
	config.image_filter = menu_items[filter_idx].value;
	
	write_config_file ();

	if (text_fg.changed)
	    update_colour_setting (
		&(config.text_fg), &(text_fg.colour));
	if (text_bg.changed)
	    update_colour_setting (
		&(config.text_bg), &(text_bg.colour));
	if (statusline_fg.changed)
	    update_colour_setting (
		&(config.statusline_fg), &(statusline_fg.colour));
	if (statusline_bg.changed)
	    update_colour_setting (
		&(config.statusline_bg), &(statusline_bg.colour));
	if (graphics_bg.changed)
	    update_colour_setting (
		&(config.graphics_bg), &(graphics_bg.colour));

	/* Apply settings */

	text_refresh ();
	graphics_refresh ();
	GUI_refresh ();
    }
    
    gtk_widget_destroy (config_dialog);
}

