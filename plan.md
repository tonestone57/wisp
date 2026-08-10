1. **Analyze `nsgtk_download_tree_view_row_activated`**: The function handles row activation on the download tree view in the GTK frontend (`frontends/gtk/download.c`). Currently, it indiscriminately calls `nsgtk_download_do(nsgtk_download_store_clear_item);`, clearing the item.
2. **Implement Context Menu**: The task specifies replacing this with context actions (`pause`, `start/resume`, `cancel`, `clear`).
3. **Menu Creation**: I will replace the hardcoded action with a newly instantiated `GtkMenu`.
4. **Menu Items Setup**:
    - Add a "Pause" item using `gtk_image_menu_item_new_from_stock("gtk-media-pause", NULL)`.
    - Add a "Play/Resume" item using `gtk_image_menu_item_new_from_stock("gtk-media-play", NULL)`.
    - Add a "Cancel" item using `gtk_image_menu_item_new_from_stock("gtk-cancel", NULL)`.
    - Add a "Clear" item using `gtk_image_menu_item_new_from_stock("gtk-clear", NULL)`.
5. **State Binding and Action Routing**:
    - Since Wisp uses `GtkButton` structures (`dl_ctx.pause`, `dl_ctx.resume`, `dl_ctx.cancel`, `dl_ctx.clear`) in its `dl_ctx` struct to track state (`gtk_widget_is_sensitive`) and handle actions, I can bind the activation of these menu items directly to `gtk_button_clicked` on their respective backing buttons.
    - Use `gtk_widget_set_sensitive` to sync each item's sensitivity with its backing button.
6. **Pop-up Launch**: Display the menu at the user's cursor using the compatibility wrapper `nsgtk_menu_popup_at_pointer(menu, NULL);`.
7. **Complete pre-commit checks**: Follow standard pre-commit validation.
