#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <map>
#include <list>
#include <string>
#include "order.h"

#include "trade.h"
#include "blocking_queue.h"

struct Orderlocation{
	Side side;
	double price;
    std::list<Order>::iterator it;

};



// OrderBook for ONE symbol (e.g., AAPL)
class OrderBook {
public:
    // Add a new order to the book
    void add_order(const Order& order);

    // Print current state of the book
    void print() const;
    void cancel_order(uint64_t order_id);    
    void match(BlockingQueue<Trade>& trade_q);
    
private:
    // BUY side: highest price first
    std::string sym;
    std::map<double, std::list<Order>, std::greater<>> buy_orders;


    // SELL side: lowest price first
    std::map<double, std::list<Order>> sell_orders;
    std::unordered_map<uint64_t, Orderlocation> order_index;

};

#endif

