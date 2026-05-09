#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "common_types.h"

#define EVT_QUEUE_MAX 32

typedef struct {
    Event events[EVT_QUEUE_MAX];
    u16 head;
    u16 tail;
    u16 count;
} EventQueue;

void evtq_init(EventQueue* eq);
bool evtq_is_empty(EventQueue* eq);
bool evtq_is_full(EventQueue* eq);
RetStatus evtq_put(EventQueue* eq, const Event* evt);
RetStatus evtq_get(EventQueue* eq, Event* evt);
u16 evtq_count(EventQueue* eq);

#endif /* EVENT_QUEUE_H */
