#include "event_queue.h"

void evtq_init(EventQueue* eq)
{
    if (eq == NULL) {
        return;
    }
    eq->head = 0;
    eq->tail = 0;
    eq->count = 0;
}

bool evtq_is_empty(EventQueue* eq)
{
    return (eq != NULL) ? (eq->count == 0) : true;
}

bool evtq_is_full(EventQueue* eq)
{
    return (eq != NULL) ? (eq->count == EVT_QUEUE_MAX) : false;
}

RetStatus evtq_put(EventQueue* eq, const Event* evt)
{
    if (eq == NULL || evt == NULL) {
        return RET_INVALID_PARAM;
    }
    
    if (evtq_is_full(eq)) {
        return RET_NO_MEMORY;
    }
    
    eq->events[eq->head] = *evt;
    eq->head = (eq->head + 1) % EVT_QUEUE_MAX;
    eq->count++;
    return RET_OK;
}

RetStatus evtq_get(EventQueue* eq, Event* evt)
{
    if (eq == NULL || evt == NULL) {
        return RET_INVALID_PARAM;
    }
    
    if (evtq_is_empty(eq)) {
        return RET_ERR;
    }
    
    *evt = eq->events[eq->tail];
    eq->tail = (eq->tail + 1) % EVT_QUEUE_MAX;
    eq->count--;
    return RET_OK;
}

u16 evtq_count(EventQueue* eq)
{
    return (eq != NULL) ? eq->count : 0;
}
