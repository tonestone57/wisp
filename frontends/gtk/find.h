/*
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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
 * free text page find for gtk interface
 */

#ifndef WISP_GTK_FIND_H_
#define WISP_GTK_FIND_H_

extern struct gui_search_table *nsgtk_find_table;

struct gtk_find;

/**
 * create text find context
 *
 * \param builder the gtk builder containing the find toolbar
 * \param bw The browsing context to run the find operations against
 * \param find_out find context result
 * \return NSERROR_OK and find_out updated
 */
nserror nsgtk_find_create(GtkBuilder *builder, struct browser_window *bw, struct gtk_find **find_out);

/**
 * update find toolbar size and style
 */
nserror nsgtk_find_restyle(struct gtk_find *find);

/**
 * toggle find bar visibility
 */
nserror nsgtk_find_toggle_visibility(struct gtk_find *find);

#endif
