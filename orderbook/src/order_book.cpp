#include "order.h"
#include<string>
#include<iostream>
#include "order_book.h"

void OrderBook::add_order(const Order& order){

	if(order.side== Side::BUY){
		auto &dq = buy_orders[order.price];
		dq.push_back(order);
		auto it = std::prev(dq.end());
		order_index[order.order_id] = {
			Side::BUY,order.price,it};
	}
	else{
		auto &dq = sell_orders[order.price];
		dq.push_back(order);
		auto it = std::prev(dq.end());
		order_index[order.order_id] ={
			Side::SELL,order.price,it
		};
	}
}

void OrderBook::print() const {
	 std::cout<<sym<<"\n";
	 std::cout << "BUY SIDE:\n";
    for (const auto& [price, orders] : buy_orders) {
        uint64_t total_qty = 0;
        for (const auto& o : orders) {
            total_qty += o.quantity;
        }
        std::cout << price << " -> " << total_qty << "\n";
    }

    std::cout << "\nSELL SIDE:\n";
    for (const auto& [price, orders] : sell_orders) {
        uint64_t total_qty = 0;
        for (const auto& o : orders) {
            total_qty += o.quantity;
        }
        std::cout << price << " -> " << total_qty << "\n";
    }
}

void OrderBook::match(BlockingQueue<Trade>& trade_q) {
	
	// While both sides have orders
    while (!buy_orders.empty() && !sell_orders.empty()) {
	auto best_buy_it = buy_orders.begin();   // highest BUY
        auto best_sell_it = sell_orders.begin(); // lowest SELL

        double buy_price = best_buy_it->first;
        double sell_price = best_sell_it->first;

        // If prices don't cross, stop
        if (buy_price < sell_price) {
            break;
        }

    auto& buy_list = best_buy_it->second;
	auto buy_it = buy_list.begin();
	Order& buy_order = *buy_it;

	auto& sell_list = best_sell_it->second;
	auto sell_it = sell_list.begin();
    Order& sell_order = *sell_it;

        uint64_t trade_qty =
            std::min(buy_order.quantity, sell_order.quantity);

        double trade_price = sell_price;

            Trade t;
            t.symbol = buy_order.symbol;
            t.quantity = trade_qty;
            t.price = trade_price;

            trade_q.push(t);

        // Reduce quantities
        buy_order.quantity -= trade_qty;
        sell_order.quantity -= trade_qty;

        // Remove filled BUY order
        if (buy_order.quantity == 0) {
		
	        order_index.erase(buy_order.order_id);
            buy_list.erase(buy_it);
            if (buy_list.empty()) {
                buy_orders.erase(best_buy_it);
            }
        }

        // Remove filled SELL order
        if (sell_order.quantity == 0) {

	    order_index.erase(sell_order.order_id);
            sell_list.erase(sell_it);
            if (sell_list.empty()) {
                sell_orders.erase(best_sell_it);
            }
        }
    }
}

void OrderBook::cancel_order(uint64_t order_id){
	auto it = order_index.find(order_id);
   	 if (it == order_index.end()) {
       	return;
   	 }
	
	Orderlocation lc = it->second;
	
	if(lc.side == Side::BUY){
		auto pit = buy_orders.find(lc.price);
		if(pit!=buy_orders.end()){
			auto &dq = pit->second;
			dq.erase(lc.it);
		

		if(dq.empty()){
			buy_orders.erase(pit);
		}
		}
	}

	else{

		auto pit = sell_orders.find(lc.price);
		if(pit!=sell_orders.end()){
			auto &dq = pit->second;
			dq.erase(lc.it);
		

		if(dq.empty()){
			sell_orders.erase(pit);
	        }
		}

	}
	
	order_index.erase(it);
}	
		
	
