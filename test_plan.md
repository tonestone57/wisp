Okay, there are no specific messages for "Pause", "Play".
But I can just use `gtk_image_menu_item_new_from_stock("gtk-media-pause", NULL)` and GTK will automatically use the localized label for the stock item.
In GTK+ 3, stock items are deprecated but they still work and will show the localized "Pause", "Cancel", "Clear" strings, with the icons. Or we can just use `gtk_menu_item_new_with_label` but it won't be translated unless Wisp provides translations. Given Wisp has no translations for Pause/Play, using stock is probably best because GTK handles the string.

```c
static void
nsgtk_download_tree_view_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
    GtkMenu *menu;
    GtkWidget *item;
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model(tree);

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        menu = GTK_MENU(gtk_menu_new());

        /* Pause */
        item = gtk_image_menu_item_new_from_stock("gtk-media-pause", NULL);
        gtk_widget_set_sensitive(item, gtk_widget_is_sensitive(GTK_WIDGET(dl_ctx.pause)));
        g_signal_connect_swapped(item, "activate", G_CALLBACK(gtk_button_clicked), dl_ctx.pause);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        /* Start/Resume */
        item = gtk_image_menu_item_new_from_stock("gtk-media-play", NULL);
        gtk_widget_set_sensitive(item, gtk_widget_is_sensitive(GTK_WIDGET(dl_ctx.resume)));
        g_signal_connect_swapped(item, "activate", G_CALLBACK(gtk_button_clicked), dl_ctx.resume);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        /* Cancel */
        item = gtk_image_menu_item_new_from_stock("gtk-cancel", NULL);
        gtk_widget_set_sensitive(item, gtk_widget_is_sensitive(GTK_WIDGET(dl_ctx.cancel)));
        g_signal_connect_swapped(item, "activate", G_CALLBACK(gtk_button_clicked), dl_ctx.cancel);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        /* Clear */
        item = gtk_image_menu_item_new_from_stock("gtk-clear", NULL);
        gtk_widget_set_sensitive(item, gtk_widget_is_sensitive(GTK_WIDGET(dl_ctx.clear)));
        g_signal_connect_swapped(item, "activate", G_CALLBACK(gtk_button_clicked), dl_ctx.clear);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        gtk_widget_show_all(GTK_WIDGET(menu));
        nsgtk_menu_popup_at_pointer(menu, NULL);
    }
}
```

Wait, `nsgtk_menu_popup_at_pointer` expects `(GtkMenu *menu, const GdkEvent *trigger_event)`.
I can pass `NULL` for `trigger_event` because `row-activated` does not provide an event directly. But wait, `nsgtk_menu_popup_at_pointer` is implemented as:
```c
void nsgtk_menu_popup_at_pointer(GtkMenu *menu, const GdkEvent *trigger_event)
{
#if GTK_CHECK_VERSION(3,22,0)
    gtk_menu_popup_at_pointer(menu, trigger_event);
#else
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
}
```
If we pass `NULL`, it pops up at the pointer.

Let's test this in the codebase.
