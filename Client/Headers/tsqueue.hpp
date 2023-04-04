//Thread-safe deque for storing owned_message objects 

#pragma once 

#include <deque>
#include <mutex>
#include <condition_variable>

template <typename T>
class tsqueue {

    public:

        tsqueue() = default;
        tsqueue(const tsqueue<T>&) = delete;
        virtual ~tsqueue() { clear(); }

        const T& front() {
            std::scoped_lock lock(mutex); 
            return deque.front();
        }
    
        const T& back() {
            std::scoped_lock lock(mutex);
            return deque.back();
        }

        T pop_front() {
            std::scoped_lock lock(mutex);
            auto t = std::move(deque.front());
            deque.pop_front();
            return t;
        }

        T pop_back() {
            std::scoped_lock lock(mutex);
            auto t = std::move(deque.back());
            deque.pop_back();
            return t;
        }

        void push_back(const T& item) {
            std::scoped_lock lock(mutex);
            deque.emplace_back(std::move(item));
        
            cv.notify_all();
        }

        void push_front(const T& item) {
            std::scoped_lock lock(mutex);
            deque.emplace_front(std::move(item));

            cv.notify_all();
        }

        bool empty() {
            std::scoped_lock lock(mutex);
            return deque.empty();
        }

        size_t count() {
            std::scoped_lock lock(mutex);
            return deque.size();
        }

        void clear() {
            std::scoped_lock lock(mutex);
            deque.clear();
        }

        //Will be called in connection class to wait for incoming messages
        void wait() {
            while (empty()) {
                std::unique_lock<std::mutex> ul(mutex);
                cv.wait(ul);
            }
        }

    protected:
        std::mutex mutex;
        std::deque<T> deque;
        std::condition_variable cv;
};
