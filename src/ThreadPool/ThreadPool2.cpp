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
        bool should_terminate = true;           // Tells threads to stop looking for jobs
        vector<bool> thread_busy;                // Tells threads to stop looking for jobs
        mutex queue_mutex;                  // Prevents data races to the job queue
        vector<mutex> queue_mutexes;                  // Prevents data races to the job queue
        condition_variable mutex_condition; // Allows threads to wait on new jobs or termination
        vector<thread> threads;
        queue<function<void()>> jobs;
        vector<queue<function<void()>>> tJobs;
};

void ThreadPool::Start(int numThreads) {
    int max =  (int)thread::hardware_concurrency();  // Max # of threads the system supports
    num_threads = (numThreads > max) ? max : numThreads;
    thread_busy = vector<bool>(num_threads, false);
    tJobs = vector<queue<function<void()>>>(num_threads);
    queue_mutexes = vector<mutex>(num_threads);
    for (uint32_t i = 0; i < 5; ++i) {
        threads.emplace_back(&ThreadPool::ThreadLoop, this, i);
    }
}

void ThreadPool::ThreadLoop(uint32_t thread_id) {
    while (true) {
        std::function<void()> job;
        thread_busy[thread_id] = false;
        if(debug) cout << "Thread " << this_thread::get_id() << " waiting for lock" << endl;
        {
            std::unique_lock<std::mutex> lock(queue_mutexes[thread_id]);
            if(debug) cout << "Thread " << this_thread::get_id() << " waiting for job" << endl;
            mutex_condition.wait(lock, [this, &thread_id] {
                return !tJobs[thread_id].empty() || should_terminate;
            });
            if (should_terminate) {
                return;
            }
            if(debug) cout << "Thread " << this_thread::get_id() << " found job" << endl;
            job = tJobs[thread_id].front();
            tJobs[thread_id].pop();
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
        if (jobs.size()>1) {
                for (int i = threads.size(); i < jobs.size() || i<num_threads; ++i)
                    threads.emplace_back(&ThreadPool::ThreadLoop, this, i);
                int numJobs = jobs.size();
                int numThreads = threads.size();
                int jobsPerThread = numJobs / numThreads;
                int jobsLeft = numJobs % numThreads;
                for (int i = 0; i < numThreads; i++) {
                    {
                        unique_lock<mutex> lock2(queue_mutexes[i]);
                        for (int j = 0; j < jobsPerThread; j++) {
                            tJobs[i].push(jobs.front());
                            jobs.pop();
                        }
                        if (jobsLeft > 0) {
                            tJobs[i].push(jobs.front());
                            jobs.pop();
                            jobsLeft--;
                        }
                        cout<<"Thread "<<i<<" has "<<tJobs[i].size()<<" jobs"<<endl;
                    }
                }
        }
    }
    mutex_condition.notify_all();
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