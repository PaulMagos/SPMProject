#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

using namespace std;

class ThreadPool
{
    public:
        void Start(int numThreads);
        void QueueJob(const std::function<void()>& job);
        void Stop();
        bool busy();
        bool debug = false;                       // Tells threads to stop looking for jobs

    private:
        void ThreadLoop(uint32_t thread_id = 0);

        uint32_t num_threads;
        bool should_terminate = false;           // Tells threads to stop looking for jobs
        vector<bool> thread_busy;                // Tells threads to stop looking for jobs
        mutex queue_mutex;                  // Prevents data races to the job queue
        condition_variable mutex_condition; // Allows threads to wait on new jobs or termination
        vector<thread> threads;
        queue<function<void()>> jobs;
};

void ThreadPool::Start(int numThreads) {
    int max =  (int)thread::hardware_concurrency();  // Max # of threads the system supports
    num_threads = (numThreads > max) ? max : numThreads;
    thread_busy = vector<bool>(num_threads, false);
    for (uint32_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(&ThreadPool::ThreadLoop, this, i);
    }
}

void ThreadPool::ThreadLoop(uint32_t thread_id) {
    while (true) {
        std::function<void()> job;
        thread_busy[thread_id] = false;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if(debug) cout << "Thread " << this_thread::get_id() << " waiting for job" << endl;
            mutex_condition.wait(lock, [this] {
                return !jobs.empty() || should_terminate;
            });
            if (should_terminate) {
                return;
            }
            if(debug) cout << "Thread " << this_thread::get_id() << " found job" << endl;
            job = jobs.front();
            jobs.pop();
        }
        thread_busy[thread_id] = true;
        if(debug) cout << "Thread " << this_thread::get_id() << " executing job" << endl;
        job();
    }
}

void ThreadPool::QueueJob(const std::function<void()>& job) {
    {
        lock_guard<mutex> lock(queue_mutex);
        jobs.push(job);
    }
    mutex_condition.notify_one();
}

void ThreadPool::Stop() {
    {
        lock_guard<mutex> lock(queue_mutex);
        should_terminate = true;
    }
    mutex_condition.notify_all();
    for (auto& thread : threads) {
        thread.join();
    }
}

bool ThreadPool::busy() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    for (int i = 0; i < num_threads; i++) {
        if (thread_busy[i]) return true;
    }
    return !jobs.empty();
}