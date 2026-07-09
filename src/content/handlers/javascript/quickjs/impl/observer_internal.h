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
    bool attributeOldValue;
    bool characterDataOldValue;
} MutationObserverTarget;

typedef struct WispMutationObserver {
    struct WispMutationObserver *next;
    JSValue callback;
    struct MutationObserverTarget *targets;
    JSValue queue;
    JSContext *ctx;
    JSValue self;
    bool queued;
    uint32_t magic;
} WispMutationObserver;

typedef struct WispMutationRecord {
    char *type;
    struct dom_node *target;
    struct dom_node **addedNodes;
    int numAddedNodes;
    struct dom_node **removedNodes;
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
    double lastRatio;
} IntersectionObserverTarget;

typedef struct WispIntersectionObserver {
    struct WispIntersectionObserver *next;
    JSValue callback;
    struct IntersectionObserverTarget *targets;
    JSValue queue;
    JSContext *ctx;
    JSValue self;
    struct dom_node *root;
    char *root_margin;
    double *thresholds;
    int num_thresholds;
    uint32_t magic;
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
