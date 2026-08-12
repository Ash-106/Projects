# Multithreaded Limit Order Book & Matching Engine

A deterministic, event-driven limit order book and matching engine implemented in C++17.

The project simulates the core order-processing workflow of a trading system: market participants submit buy/sell orders, orders are maintained in separate books for each instrument, compatible orders are matched according to price-time priority, and resulting trades are sent through a multithreaded processing pipeline.

---

## Features

- Multi-symbol limit order books
- Buy and sell order management
- Price-time priority matching
- Partial order fills
- Order cancellation
- Trade generation
- Event-driven architecture
- Multithreaded input, matching, and logging pipeline
- Thread-safe producer-consumer queues
- Blocking synchronization using `std::mutex` and `std::condition_variable`
- No busy-waiting
- Deterministic matching
- Graceful thread shutdown
- Synthetic high-load testing

---

## What is a Limit Order Book?

A limit order book maintains outstanding buy and sell orders for a particular financial instrument.

For example:
AAPL

BUY SIDE
151.00 -> 50
150.50 -> 100

SELL SIDE
151.50 -> 40
152.00 -> 80