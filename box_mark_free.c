void box_mark_dirty(struct box *box)
{
	if (box == NULL) {
		return;
	}

	if (box->flags & DIRTY_INTRINSIC) {
		/* Already dirty, no need to propagate */
		return;
	}

	/* Accumulate OLD bounding box of dirty element and add to dirty list */
	struct html_content *html = box_get_html_content(box);
	if (html != NULL) {
		struct rect r;
		int x, y;
		box_coords(box, &x, &y);
		r.x0 = x + box->descendant_x0;
		r.y0 = y + box->descendant_y0;
		r.x1 = x + box->descendant_x1;
		r.y1 = y + box->descendant_y1;

		if (html->has_dirty_rect) {
			ns_rect_union(&html->dirty_rect, &r);
		} else {
			html->dirty_rect = r;
			html->has_dirty_rect = true;
		}

		/* Add to dirty list for post-layout bounding box capture */
		if (!(box->flags & BOX_IN_DIRTY_LIST)) {
			box->next_dirty = html->dirty_list;
			html->dirty_list = box;
			box->flags |= BOX_IN_DIRTY_LIST;
		}
	}

	box->flags |= DIRTY_INTRINSIC;

	struct box *parent = box->parent;
	while (parent != NULL) {
		if (parent->flags & CHILD_DIRTY) {
			/* Ancestor already knows it has a dirty child, stop propagating */
			break;
		}
		parent->flags |= CHILD_DIRTY;
		parent = parent->parent;
	}
}

void box_free(struct box *box)
{
	struct box *child, *next;

	/* Ensure box is removed from dirty list to prevent dangling pointers */
	struct html_content *html = box_get_html_content(box);
	if (html != NULL && (box->flags & BOX_IN_DIRTY_LIST)) {
		struct box *prev = NULL;
		struct box *curr = html->dirty_list;
		while (curr != NULL) {
			if (curr == box) {
				if (prev == NULL) {
					html->dirty_list = curr->next_dirty;
				} else {
					prev->next_dirty = curr->next_dirty;
				}				box->flags &= ~BOX_IN_DIRTY_LIST;				break;			}			prev = curr;			curr = curr->next_dirty;		}	}	/* free children first */	for (child = box->children; child; child = next) {		next = child->next;		box_free(child);	}	/* last this box */	box_free_box(box);}EOF
# I will use write_file for the remaining box_manipulate.c functions
# No, I'll just append it since I deleted them.
# Wait, I shouldn't have deleted box_free_box.
# I'll just restore the whole file and do targeted seds for mark_dirty and free.
git checkout 895bf0e -- src/content/handlers/html/box_manipulate.c
sed -i '/void box_mark_dirty(struct box \*box)/,/^}/d' src/content/handlers/html/box_manipulate.c
sed -i '/void box_free(struct box \*box)/,/^}/d' src/content/handlers/html/box_manipulate.c
# Define helpers and new implementations
sed -i '/#include <wisp\/content\/handlers\/html\/box.h>/a #include <wisp/content/handlers/html/box_inspect.h>' src/content/handlers/html/box_manipulate.c
sed -i 's/struct box \*box_create(css_select_results \*styles/struct box *box_create(struct html_content *content, css_select_results *styles/' src/content/handlers/html/box_manipulate.c
sed -i '/arena_register_destructor(context, box, box_talloc_destructor);/a \    box->content = content;' src/content/handlers/html/box_manipulate.c

cat >> src/content/handlers/html/box_manipulate.c <<EOF
/**
 * Find the html_content associated with a box.
 */
struct html_content *box_get_html_content(struct box *box)
{
	return box->content;
}

void box_mark_dirty(struct box *box)
{
	if (box == NULL) {
		return;
	}

	if (box->flags & DIRTY_INTRINSIC) {
		/* Already dirty, no need to propagate */
		return;
	}

	/* Accumulate OLD bounding box of dirty element and add to dirty list */
	struct html_content *html = box_get_html_content(box);
	if (html != NULL) {
		struct rect r;
		int x, y;
		box_coords(box, &x, &y);
		r.x0 = x + box->descendant_x0;
		r.y0 = y + box->descendant_y0;
		r.x1 = x + box->descendant_x1;
		r.y1 = y + box->descendant_y1;

		if (html->has_dirty_rect) {
			ns_rect_union(&html->dirty_rect, &r);
		} else {
			html->dirty_rect = r;
			html->has_dirty_rect = true;
		}

		/* Add to dirty list for post-layout bounding box capture */
		if (!(box->flags & BOX_IN_DIRTY_LIST)) {
			box->next_dirty = html->dirty_list;
			html->dirty_list = box;
			box->flags |= BOX_IN_DIRTY_LIST;
		}
	}

	box->flags |= DIRTY_INTRINSIC;

	struct box *parent = box->parent;
	while (parent != NULL) {
		if (parent->flags & CHILD_DIRTY) {
			/* Ancestor already knows it has a dirty child, stop propagating */
			break;
		}
		parent->flags |= CHILD_DIRTY;
		parent = parent->parent;
	}
}

void box_free(struct box *box)
{
	struct box *child, *next;

	/* Ensure box is removed from dirty list to prevent dangling pointers */
	struct html_content *html = box_get_html_content(box);
	if (html != NULL && (box->flags & BOX_IN_DIRTY_LIST)) {
		struct box *prev = NULL;
		struct box *curr = html->dirty_list;
		while (curr != NULL) {
			if (curr == box) {
				if (prev == NULL) {
					html->dirty_list = curr->next_dirty;
				} else {
					prev->next_dirty = curr->next_dirty;
				}
				box->flags &= ~BOX_IN_DIRTY_LIST;
				break;
			}
			prev = curr;
			curr = curr->next_dirty;
		}
	}

	/* free children first */
	for (child = box->children; child; child = next) {
		next = child->next;
		box_free(child);
	}

	/* last this box */
	box_free_box(box);
}
