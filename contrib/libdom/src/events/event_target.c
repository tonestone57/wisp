/*
 * This file is part of libdom.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 Bo Yang <struggleyb.nku@gmail.com>
 */

#include <assert.h>
#include <stdlib.h>

#include "events/event.h"
#include "events/event_listener.h"
#include "events/event_target.h"

#include "core/document.h"
#include "core/node.h"
#include "core/string.h"

#include "utils/utils.h"
#include "utils/validate.h"

static void event_target_destroy_listener(struct listener_entry *e)
{
    list_del(&e->list);
    dom_event_listener_unref(e->listener);
    dom_string_unref(e->type);
    free(e);
}
static void event_target_destroy_listeners(struct listener_entry *list)
{
    struct listener_entry *next;

    while (list != (struct listener_entry *)list->list.next) {
        next = (struct listener_entry *)list->list.next;
        event_target_destroy_listener(list);
        list = next;
    }

    event_target_destroy_listener(list);
}

/* Initialise this EventTarget */
dom_exception _dom_event_target_internal_initialise(dom_event_target_internal *eti)
{
    eti->listeners = NULL;

    return DOM_NO_ERR;
}

/* Finalise this EventTarget */
void _dom_event_target_internal_finalise(dom_event_target_internal *eti)
{
    if (eti->listeners != NULL) {
        event_target_destroy_listeners(eti->listeners);
        eti->listeners = NULL;
    }
}

/*-------------------------------------------------------------------------*/
/* The public API */

/**
 * Add an EventListener to the EventTarget
 *
 * \param et        The EventTarget object
 * \param type      The event type which this event listener listens for
 * \param listener  The event listener object
 * \param capture   Whether add this listener in the capturing phase
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 */
dom_exception _dom_event_target_add_event_listener(
    dom_event_target_internal *eti, dom_string *type, struct dom_event_listener *listener, bool capture)
{
    struct listener_entry *le = NULL;

    le = malloc(sizeof(struct listener_entry));
    if (le == NULL)
        return DOM_NO_MEM_ERR;

    /* Initialise the listener_entry */
    list_init(&le->list);
    le->type = dom_string_ref(type);
    le->listener = listener;
    dom_event_listener_ref(listener);
    le->capture = capture;

    if (eti->listeners == NULL) {
        eti->listeners = le;
    } else {
        list_append(&eti->listeners->list, &le->list);
    }

    return DOM_NO_ERR;
}

/**
 * Remove an EventListener from the EventTarget
 *
 * (LibDOM extension: If type is NULL, remove all listener registrations
 * regardless of type and cature)
 *
 * \param et        The EventTarget object
 * \param type      The event type this listener is registered for
 * \param listener  The listener object
 * \param capture   Whether the listener is registered at the capturing phase
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 */
dom_exception _dom_event_target_remove_event_listener(
    dom_event_target_internal *eti, dom_string *type, struct dom_event_listener *listener, bool capture)
{
    if (eti->listeners != NULL) {
        struct listener_entry *le = eti->listeners;

        do {
            bool matches;
            if (type == NULL) {
                matches = (le->listener == listener);
            } else {
                matches = dom_string_isequal(le->type, type) && (le->listener == listener) && (le->capture == capture);
            }
            if (matches) {
                if (le->list.next == &le->list) {
                    eti->listeners = NULL;
                } else {
                    eti->listeners = (struct listener_entry *)le->list.next;
                }
                list_del(&le->list);
                dom_event_listener_unref(le->listener);
                dom_string_unref(le->type);
                free(le);
                break;
            }

            le = (struct listener_entry *)le->list.next;
        } while (eti->listeners != NULL && le != eti->listeners);
    }

    return DOM_NO_ERR;
}

/**
 * Add an EventListener
 *
 * \param et         The EventTarget object
 * \param namespace  The namespace of this listener
 * \param type       The event type which this event listener listens for
 * \param listener   The event listener object
 * \param capture    Whether add this listener in the capturing phase
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 *
 * We don't support this API now, so it always return DOM_NOT_SUPPORTED_ERR.
 */
dom_exception _dom_event_target_add_event_listener_ns(dom_event_target_internal *eti, dom_string *namespace,
    dom_string *type, struct dom_event_listener *listener, bool capture)
{
    UNUSED(eti);
    UNUSED(namespace);
    UNUSED(type);
    UNUSED(listener);
    UNUSED(capture);

    return DOM_NOT_SUPPORTED_ERR;
}

/**
 * Remove an EventListener
 *
 * \param et         The EventTarget object
 * \param namespace  The namespace of this listener
 * \param type       The event type which this event listener listens for
 * \param listener   The event listener object
 * \param capture    Whether add this listener in the capturing phase
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 *
 * We don't support this API now, so it always return DOM_NOT_SUPPORTED_ERR.
 */
dom_exception _dom_event_target_remove_event_listener_ns(dom_event_target_internal *eti, dom_string *namespace,
    dom_string *type, struct dom_event_listener *listener, bool capture)
{
    UNUSED(eti);
    UNUSED(namespace);
    UNUSED(type);
    UNUSED(listener);
    UNUSED(capture);

    return DOM_NOT_SUPPORTED_ERR;
}

/*-------------------------------------------------------------------------*/

/**
 * Dispatch an event on certain EventTarget
 *
 * \param et       The EventTarget object
 * \param eti      Internal EventTarget object
 * \param evt      The event object
 * \param success  Indicates whether any of the listeners which handled the
 *                 event called Event.preventDefault(). If
 *                 Event.preventDefault() was called the returned value is
 *                 false, else it is true.
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 */
dom_exception _dom_event_target_dispatch(dom_event_target *et, dom_event_target_internal *eti, struct dom_event *evt,
    dom_event_flow_phase phase, bool *success)
{
    if (eti->listeners != NULL) {
        int count = 0;
        struct listener_entry *le = eti->listeners;
        do {
            count++;
            le = (struct listener_entry *)le->list.next;
        } while (le != eti->listeners);

        struct listener_copy {
            struct dom_event_listener *listener;
            dom_string *type;
            bool capture;
        } *copied_listeners = malloc(count * sizeof(struct listener_copy));

        if (copied_listeners != NULL) {
            int idx = 0;
            le = eti->listeners;
            do {
                copied_listeners[idx].listener = le->listener;
                dom_event_listener_ref(le->listener);
                copied_listeners[idx].type = le->type;
                dom_string_ref(le->type);
                copied_listeners[idx].capture = le->capture;
                idx++;
                le = (struct listener_entry *)le->list.next;
            } while (le != eti->listeners);

            evt->current = et;

            for (int i = 0; i < count; i++) {
                struct listener_copy curr = copied_listeners[i];

                /* Verify that the listener is still registered on the target */
                bool still_registered = false;
                if (eti->listeners != NULL) {
                    struct listener_entry *check = eti->listeners;
                    do {
                        if (check->listener == curr.listener &&
                            dom_string_isequal(check->type, curr.type) &&
                            check->capture == curr.capture) {
                            still_registered = true;
                            break;
                        }
                        check = (struct listener_entry *)check->list.next;
                    } while (check != eti->listeners);
                }

                if (still_registered) {
                    if (dom_string_isequal(curr.type, evt->type)) {
                        assert(curr.listener->handler != NULL);

                        if ((curr.capture && phase == DOM_CAPTURING_PHASE) ||
                            (!curr.capture && phase == DOM_BUBBLING_PHASE) ||
                            (evt->target == evt->current && phase == DOM_AT_TARGET)) {
                            curr.listener->handler(evt, curr.listener->pw);
                            if (evt->stop_now == true)
                                break;
                        }
                    }
                }
            }

            /* Cleanup refs and free */
            for (int i = 0; i < count; i++) {
                dom_event_listener_unref(copied_listeners[i].listener);
                dom_string_unref(copied_listeners[i].type);
            }
            free(copied_listeners);
        }
    }

    if (evt->prevent_default == true)
        *success = false;

    return DOM_NO_ERR;
}
