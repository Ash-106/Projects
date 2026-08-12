#include <iostream>
#include <string>
#include "order.h"
#include "order_book.h"
#include <thread>
#include "event.h"
#include "blocking_queue.h"
#include <chrono>

void input_thread(BlockingQueue<Event>& q) {
    std::string action;

    while (std::cin >> action) {
        if (action == "ADD") {
            std::string side, symbol;
            uint64_t qty;
            double price;

            std::cin >> side >> qty >> symbol >> price;

            Event e;
            e.type = EventType::ADD;
            e.order.symbol = symbol;
            e.order.quantity = qty;
            e.order.price = price;
            e.order.side = (side == "BUY") ? Side::BUY : Side::SELL;

            q.push(e);
        }
        else if (action == "CANCEL") {
            uint64_t id;
            std::cin >> id;

            Event e;
            e.type = EventType::CANCEL;
            e.cancel_id = id;

            q.push(e);
        }
    }

    Event e;
    e.type = EventType::SHUTDOWN;
    q.push(e);
}

void matching_thread(BlockingQueue<Event>& event_q,
                     BlockingQueue<Trade>& trade_q) {

    std::unordered_map<std::string, OrderBook> books;

    while (true) {
        Event e = event_q.pop();

        if (e.type == EventType::ADD) {
            books[e.order.symbol].add_order(e.order);
            books[e.order.symbol].match(trade_q);
        }
        else if (e.type == EventType::CANCEL) {
            for (auto& [_, book] : books) {
                book.cancel_order(e.cancel_id);
            }
        }
        else if (e.type == EventType::SHUTDOWN) {
            break;
        }
    }

    // Signal downstream that matching is done
    Trade shutdown;
    shutdown.quantity = 0;   // sentinel
    trade_q.push(shutdown);
}

void logger_thread(BlockingQueue<Trade>& trade_q) {
    while (true) {
        Trade t = trade_q.pop();

        if (t.quantity == 0) {
            break;  // shutdown signal
        }

        std::cout << "TRADE "
                  << t.symbol << " "
                  << t.quantity << " @ "
                  << t.price << "\n";
    }
}

int main() {

    auto start = std::chrono::high_resolution_clock::now();

    BlockingQueue<Event> event_q;
    BlockingQueue<Trade> trade_q;

    std::thread producer(input_thread, std::ref(event_q));
    std::thread matcher(matching_thread,
                        std::ref(event_q),
                        std::ref(trade_q));
    std::thread logger(logger_thread, std::ref(trade_q));

   

    producer.join();
    matcher.join();
    logger.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cerr << "REACHED END OF MAIN\n";

    std::cerr << "TOTAL TIME: " << ms << " ms\n";
    return 0;
}