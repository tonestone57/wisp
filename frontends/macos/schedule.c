#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "wisp/utils/errors.h"
#include "wisp/utils/log.h"
#include "macos/schedule.h"

struct nscallback {
    struct nscallback *next;
    uint64_t tv; /* Absolute time in microseconds */
    void (*callback)(void *p);
    void *p;
};

static struct nscallback *schedule_list = NULL;

static uint64_t get_monotonic_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
}

static nserror schedule_remove(void (*callback)(void *p), void *p)
{
    struct nscallback *cur_nscb;
    struct nscallback *prev_nscb;
    struct nscallback *unlnk_nscb;

    if (schedule_list == NULL) {
        return NSERROR_OK;
    }

    cur_nscb = schedule_list;
    prev_nscb = NULL;

    while (cur_nscb != NULL) {
        if ((cur_nscb->callback == callback) && (cur_nscb->p == p)) {
            unlnk_nscb = cur_nscb;
            cur_nscb = unlnk_nscb->next;

            if (prev_nscb == NULL) {
                schedule_list = cur_nscb;
            } else {
                prev_nscb->next = cur_nscb;
            }
            free(unlnk_nscb);
        } else {
            prev_nscb = cur_nscb;
            cur_nscb = prev_nscb->next;
        }
    }
    return NSERROR_OK;
}

nserror macos_schedule(int ival, void (*callback)(void *p), void *p)
{
    struct nscallback *nscb;
    struct nscallback *cur_nscb;
    struct nscallback *prev_nscb;
    nserror ret;

    ret = schedule_remove(callback, p);
    if ((ival < 0) || (ret != NSERROR_OK)) {
        return ret;
    }

    nscb = calloc(1, sizeof(struct nscallback));
    if (nscb == NULL) {
        return NSERROR_NOMEM;
    }

    nscb->tv = get_monotonic_time_us() + ((uint64_t)ival * 1000);
    nscb->callback = callback;
    nscb->p = p;

    if (schedule_list == NULL || nscb->tv < schedule_list->tv) {
        nscb->next = schedule_list;
        schedule_list = nscb;
    } else {
        cur_nscb = schedule_list;
        prev_nscb = NULL;
        while (cur_nscb != NULL && cur_nscb->tv <= nscb->tv) {
            prev_nscb = cur_nscb;
            cur_nscb = cur_nscb->next;
        }
        nscb->next = cur_nscb;
        prev_nscb->next = nscb;
    }

    return NSERROR_OK;
}

int schedule_run(void)
{
    uint64_t now;
    struct nscallback *unlnk_nscb;

    if (schedule_list == NULL)
        return -1;

    now = get_monotonic_time_us();

    while (schedule_list != NULL && schedule_list->tv <= now) {
        unlnk_nscb = schedule_list;
        schedule_list = unlnk_nscb->next;

        unlnk_nscb->callback(unlnk_nscb->p);

        free(unlnk_nscb);
    }

    if (schedule_list == NULL) {
        return -1;
    }

    uint64_t nexttime = schedule_list->tv;
    int64_t diff = nexttime - now;
    if (diff < 0) {
        diff = 0;
    }

    return (int)((diff + 999) / 1000);
}
