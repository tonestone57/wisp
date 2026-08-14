1. **Enable editing on cookie fields:**
   In `src/desktop/cookie_manager.c`, we need to update the flags for `COOKIE_M_CONTENT` (value) and `COOKIE_M_EXPIRES` (expiry). We'll add `TREE_FLAG_ALLOW_EDIT` to `cm_ctx.fields[COOKIE_M_CONTENT].flags` and `cm_ctx.fields[COOKIE_M_EXPIRES].flags` within `cookie_manager_init_entry_fields`.

2. **Add `urldb_find_cookie_internal` helper in `urldb.c`:**
   In `src/content/urldb.c`, extract a static helper `urldb_find_cookie_internal` that takes `(domain, path, name)` and returns the `cookie_internal_data *` directly (also using it in `urldb_delete_cookie` if appropriate, but primarily for the new update function).

3. **Implement `urldb_update_cookie` in `urldb.c`:**
   Create the function:
   ```c
   nserror urldb_update_cookie(const char *domain, const char *path, const char *name, const char *new_value, const time_t *new_expires)
   ```
   If `new_value` is set, `strdup` it, free the old value, and assign the new. If `new_expires` is set, update `expires`. Return `NSERROR_OK` (or `NSERROR_NOT_FOUND` / `NSERROR_NOMEM`).

4. **Implement `TREE_MSG_NODE_EDIT` handling in `cookie_manager_tree_node_entry_cb`:**
   In `cookie_manager_tree_node_entry_cb` within `src/desktop/cookie_manager.c`:
   Check if the field is `COOKIE_M_CONTENT` or `COOKIE_M_EXPIRES`.
   The parameter `data` in this callback corresponds to the `struct cookie_manager_entry *e`. We can access the cookie domain/path/name via `e->data[COOKIE_M_DOMAIN].value`, `e->data[COOKIE_M_PATH].value`, and `e->data[COOKIE_M_NAME].value`.
   If `COOKIE_M_CONTENT`, just update `value`.
   If `COOKIE_M_EXPIRES`, parse with `curl_getdate` (if not "Session"). Reject if invalid (`curl_getdate` returns -1). Note that we will include `<curl/curl.h>`.
   Update URLDB by calling `urldb_update_cookie`.
   Update the local `struct cookie_manager_entry` data, and call `treeview_update_node_entry(cm_ctx.tree, e->entry, e->data, e)`.

5. **Expose `urldb_update_cookie`:**
   Add its prototype to `include/wisp/cookie_db.h`.

6. **Verify compilation:**
   Run `cmake --build build -j$(nproc)` to ensure there are no compilation errors.

7. **Complete pre-commit steps:**
   Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

8. **Submit the change.**
   Run `ctest --test-dir build` to verify tests, and then submit the change with a descriptive commit message.
