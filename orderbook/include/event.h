#ifndef EVENT_H
#define EVENT_H

#include "order.h"
#include <cstdint>

enum class EventType {
    ADD,
    CANCEL,
    SHUTDOWN
};

struct Event {
    EventType type;
    Order order;
    uint64_t cancel_id = 0;
};

#endif
