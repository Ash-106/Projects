#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <cstdint>

enum class Side {
    BUY,
    SELL
};

struct Order {
    uint64_t order_id;
    Side side;
    std::string symbol;
    uint64_t quantity;
    double price;
};

#endif


