#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>
#include <future>

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
        vector<mutex> queue_mutex;                  // Prevents data races to the job queue
        vector<condition_variable> mutex_condition; // Allows threads to wait on new jobs or termination
        vector<thread> threads;
        vector<queue<function<void()>>> jobs;
};

void ThreadPool::Start(int numThreads) {
    int max =  (int)thread::hardware_concurrency();  // Max # of threads the system supports
    num_threads = (numThreads > max) ? max : numThreads;
    thread_busy = vector<bool>(num_threads, false);
    queue_mutex = vector<mutex>(num_threads);
    mutex_condition = vector<condition_variable>(num_threads);
    jobs = vector<queue<function<void()>>>(num_threads);
    for (uint32_t i = 0; i < numThreads; ++i) {
        threads.emplace_back( &ThreadPool::ThreadLoop, this, i);
    }
}

void ThreadPool::ThreadLoop(uint32_t thread_id) {
    while (true) {
        std::function<void()> job;
        thread_busy[thread_id] = false;
        if(debug) cout << "Thread " << thread_id << " waiting for lock" << endl;
        {
            std::unique_lock<std::mutex> lock(queue_mutex[thread_id]);
            if(debug) cout << "Thread " << thread_id << " waiting for job" << endl;
            mutex_condition[thread_id].wait(lock, [this, &thread_id] {
                return !jobs[thread_id].empty() || should_terminate;
            });
            if (should_terminate) {
                return;
            }
            if(debug) cout << "Thread " << thread_id << " found job" << endl;
            job = jobs[thread_id].front();
            jobs[thread_id].pop();
        }
        thread_busy[thread_id] = true;
        if(debug) cout << "Thread " << thread_id << " executing job" << endl;
        job();
    }
}

void ThreadPool::QueueJob(const std::function<void()>& job) {
    int pos = 0;
    for (int i = 0; i < num_threads; i++) {
        if (jobs[i].size() < jobs[pos].size()) {
            pos = i;
        }
    }
    {
        lock_guard<mutex> lock(queue_mutex[pos]);
        jobs[pos].push(job);
        mutex_condition[pos].notify_one();
    }
}

void ThreadPool::Stop() {
    should_terminate = true;
    for (int i = 0; i < num_threads; i++) {
        mutex_condition[i].notify_one();
        threads[i].join();
    }
}

bool ThreadPool::busy() {
    bool busy = false;
    for (int i = 0; i < num_threads; i++) {
        std::lock_guard<std::mutex> lock(queue_mutex[i]);
        if (thread_busy[i]) return true;
        if (!jobs[i].empty()) busy = true;
    }
    return busy;
}