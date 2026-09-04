/*
 * Copyright 2019 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * find in page gtk frontend implementation
 */

#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdlib.h>

#include <wisp/desktop/search.h>
#include <wisp/search.h>
#include <wisp/utils/nsoption.h>

#include "gtk/compat.h"
#include "gtk/find.h"
#include "gtk/toolbar_items.h"
#include "gtk/window.h"


struct gtk_find {
    GtkToolbar *bar;
    GtkEntry *entry;
    GtkToolButton *back;
    GtkToolButton *forward;
    GtkToolButton *close;
    GtkCheckButton *checkAll;
    GtkCheckButton *caseSens;

    struct browser_window *bw;
};

/**
 * activate find forwards button in gui.
 *
 * \param active activate/inactivate
 * \param find the gtk find context
 */
static void nsgtk_find_set_forward_state(bool active, struct gtk_find *find)
{
    gtk_widget_set_sensitive(GTK_WIDGET(find->forward), active);
}


/**
 * activate find back button in gui.
 *
 * \param active activate/inactivate
 * \param find the gtk find context
 */
static void nsgtk_find_set_back_state(bool active, struct gtk_find *find)
{
    gtk_widget_set_sensitive(GTK_WIDGET(find->back), active);
}


/**
 * connected to the find forward button
 */
static gboolean nsgtk_find_forward_button_clicked(GtkWidget *widget, gpointer data)
{
    struct gtk_find *find;
    search_flags_t flags;

    find = (struct gtk_find *)data;

    flags = SEARCH_FLAG_FORWARDS;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find->caseSens))) {
        flags |= SEARCH_FLAG_CASE_SENSITIVE;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find->checkAll))) {
        flags |= SEARCH_FLAG_SHOWALL;
    }

    browser_window_search(find->bw, find, flags, gtk_entry_get_text(find->entry));

    return TRUE;
}

/**
 * connected to the find back button
 */
static gboolean nsgtk_find_back_button_clicked(GtkWidget *widget, gpointer data)
{
    struct gtk_find *find;
    search_flags_t flags;

    find = (struct gtk_find *)data;

    flags = 0;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find->caseSens))) {
        flags |= SEARCH_FLAG_CASE_SENSITIVE;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find->checkAll))) {
        flags |= SEARCH_FLAG_SHOWALL;
    }

    browser_window_search(find->bw, find, flags, gtk_entry_get_text(find->entry));

    return TRUE;
}

/**
 * connected to the find close button
 */
static gboolean nsgtk_find_close_button_clicked(GtkWidget *widget, gpointer data)
{
    struct gtk_find *find;

    find = (struct gtk_find *)data;

    nsgtk_find_toggle_visibility(find);

    return TRUE;
}


/**
 * connected to the find entry [typing]
 */
static gboolean nsgtk_find_entry_changed(GtkWidget *widget, gpointer data)
{
    struct gtk_find *find;
    search_flags_t flags;

    find = (struct gtk_find *)data;

    flags = 0;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find->caseSens))) {
        flags |= SEARCH_FLAG_CASE_SENSITIVE;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find->checkAll))) {
        flags |= SEARCH_FLAG_SHOWALL;
    }

    browser_window_search(find->bw, find, flags, gtk_entry_get_text(find->entry));

    return TRUE;
}

/**
 * connected to the find entry [return key]
 */
static gboolean nsgtk_find_entry_activate(GtkWidget *widget, gpointer data)
{
    struct gtk_find *find;
    search_flags_t flags;

    find = (struct gtk_find *)data;

    flags = SEARCH_FLAG_FORWARDS;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find->caseSens))) {
        flags |= SEARCH_FLAG_CASE_SENSITIVE;
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find->checkAll))) {
        flags |= SEARCH_FLAG_SHOWALL;
    }

    browser_window_search(find->bw, find, flags, gtk_entry_get_text(find->entry));

    return FALSE;
}

/**
 * allows escape key to close find bar too
 */
static gboolean nsgtk_find_entry_key(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_KEY(Escape)) {
        struct gtk_find *find;
        find = (struct gtk_find *)data;

        nsgtk_find_toggle_visibility(find);
    }
    return FALSE;
}


static struct gui_search_table find_table = {
    .forward_state = (void *)nsgtk_find_set_forward_state,
    .back_state = (void *)nsgtk_find_set_back_state,
};

struct gui_search_table *nsgtk_find_table = &find_table;


/* exported interface documented in gtk/find.h */
nserror nsgtk_find_toggle_visibility(struct gtk_find *find)
{
    gboolean vis;

    browser_window_search_clear(find->bw);

    g_object_get(G_OBJECT(find->bar), "visible", &vis, NULL);
    if (vis) {
        gtk_widget_hide(GTK_WIDGET(find->bar));
    } else {
        gtk_widget_show(GTK_WIDGET(find->bar));
        gtk_widget_grab_focus(GTK_WIDGET(find->entry));
        nsgtk_find_entry_changed(GTK_WIDGET(find->entry), find);
    }

    return NSERROR_OK;
}


/* exported interface documented in gtk/find.h */
nserror nsgtk_find_restyle(struct gtk_find *find)
{
    switch (nsoption_int(button_type)) {

    case 1: /* Small icons */
        gtk_toolbar_set_style(GTK_TOOLBAR(find->bar), GTK_TOOLBAR_ICONS);
        gtk_toolbar_set_icon_size(GTK_TOOLBAR(find->bar), GTK_ICON_SIZE_SMALL_TOOLBAR);
        break;

    case 2: /* Large icons */
        gtk_toolbar_set_style(GTK_TOOLBAR(find->bar), GTK_TOOLBAR_ICONS);
        gtk_toolbar_set_icon_size(GTK_TOOLBAR(find->bar), GTK_ICON_SIZE_LARGE_TOOLBAR);
        break;

    case 3: /* Large icons with text */
        gtk_toolbar_set_style(GTK_TOOLBAR(find->bar), GTK_TOOLBAR_BOTH);
        gtk_toolbar_set_icon_size(GTK_TOOLBAR(find->bar), GTK_ICON_SIZE_LARGE_TOOLBAR);
        break;

    case 4: /* Text icons only */
        gtk_toolbar_set_style(GTK_TOOLBAR(find->bar), GTK_TOOLBAR_TEXT);
        break;

    default:
        break;
    }
    return NSERROR_OK;
}


/* exported interface documented in gtk/find.h */
nserror nsgtk_find_create(GtkBuilder *builder, struct browser_window *bw, struct gtk_find **find_out)
{
    struct gtk_find *find;

    find = malloc(sizeof(struct gtk_find));
    if (find == NULL) {
        return NSERROR_NOMEM;
    }

    find->bw = bw;

    find->bar = GTK_TOOLBAR(gtk_builder_get_object(builder, "findbar"));
    find->entry = GTK_ENTRY(gtk_builder_get_object(builder, "Find"));
    find->back = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "FindBack"));
    find->forward = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "FindForward"));
    find->close = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "FindClose"));
    find->checkAll = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "FindHighlightAll"));
    find->caseSens = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "FindMatchCase"));

    g_signal_connect(find->forward, "clicked", G_CALLBACK(nsgtk_find_forward_button_clicked), find);

    g_signal_connect(find->back, "clicked", G_CALLBACK(nsgtk_find_back_button_clicked), find);

    g_signal_connect(find->entry, "changed", G_CALLBACK(nsgtk_find_entry_changed), find);

    g_signal_connect(find->entry, "activate", G_CALLBACK(nsgtk_find_entry_activate), find);

    g_signal_connect(find->entry, "key-press-event", G_CALLBACK(nsgtk_find_entry_key), find);

    g_signal_connect(find->close, "clicked", G_CALLBACK(nsgtk_find_close_button_clicked), find);

    g_signal_connect(find->caseSens, "toggled", G_CALLBACK(nsgtk_find_entry_changed), find);

    g_signal_connect(find->checkAll, "toggled", G_CALLBACK(nsgtk_find_entry_changed), find);

    nsgtk_find_restyle(find);


    *find_out = find;

    return NSERROR_OK;
}
