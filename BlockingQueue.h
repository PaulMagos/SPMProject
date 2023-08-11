#include <list>
#include <mutex>

using namespace std;

template <typename T> class BlockingQueue
{
private:
    std::list<T> _queue;
    std::condition_variable _queue_cv;
    std::mutex _queue_mutex;

public:
    BlockingQueue() : _queue(){};
    void push(T item) {
        std::lock_guard<std::mutex> queue_lock(_queue_mutex);
        _queue.push_back(item);
        _queue_cv.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> queue_lock(_queue_mutex);
        _queue_cv.wait(queue_lock, [&] { return !_queue.empty(); });

        T item = _queue.front();
        _queue.pop_front();
        return item;
    }
    int size() {
        std::lock_guard<std::mutex> queue_lock(_queue_mutex);
        return _queue.size();
    }
};