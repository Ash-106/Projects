#ifndef TRADE_H
#define TRADE_H

#include <string>
#include <cstdint>

struct Trade {
    std::string symbol;
    uint64_t quantity;
    double price;
};
#endif

