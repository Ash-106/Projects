#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class BlockingQueue {
public:
    // Add an item and wake one waiting thread
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            q.push(std::move(item));
        }
        cv.notify_one();
    }

    // Remove an item, block if empty
    T pop() {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait until queue is NOT empty
        cv.wait(lock, [&] { return !q.empty(); });

        T item = std::move(q.front());
        q.pop();
        return item;
    }

private:
    std::queue<T> q;
    std::mutex mtx;
    std::condition_variable cv;
};

#endif

