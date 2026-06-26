#ifndef WISP_OBSERVER_INTERNAL_H
#define WISP_OBSERVER_INTERNAL_H

#include "quickjs.h"
#include "utils/libdom.h"

typedef struct MutationObserverTarget {
    struct MutationObserverTarget *next;
    struct dom_node *node;
    bool childList;
    bool attributes;
    bool characterData;
    bool subtree;
} MutationObserverTarget;

typedef struct WispMutationObserver {
    struct WispMutationObserver *next;
    JSValue callback;
    struct MutationObserverTarget *targets;
    JSValue queue;
    JSContext *ctx;
    JSValue self;
    bool queued;
} WispMutationObserver;

typedef struct WispMutationRecord {
    char *type;
    struct dom_node *target;
    struct dom_node *addedNodes[1];
    int numAddedNodes;
    struct dom_node *removedNodes[1];
    int numRemovedNodes;
    struct dom_node *previousSibling;
    struct dom_node *nextSibling;
    char *attributeName;
    char *attributeNamespace;
    char *oldValue;
} WispMutationRecord;

typedef struct IntersectionObserverTarget {
    struct IntersectionObserverTarget *next;
    struct dom_node *node;
    bool wasIntersecting;
} IntersectionObserverTarget;

typedef struct WispIntersectionObserver {
    struct WispIntersectionObserver *next;
    JSValue callback;
    struct IntersectionObserverTarget *targets;
    JSValue queue;
    JSContext *ctx;
} WispIntersectionObserver;

typedef struct WispIntersectionObserverEntry {
    double time;
    struct dom_node *target;
    double intersectionRatio;
    bool isIntersecting;
    double targetX, targetY, targetWidth, targetHeight;
    double intersectX, intersectY, intersectWidth, intersectHeight;
    double rootWidth, rootHeight;
} WispIntersectionObserverEntry;

#endif /* WISP_OBSERVER_INTERNAL_H */
