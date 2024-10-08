#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
  explicit ThreadPool(size_t num_threads) : m_stop(false) {
    // std::cout << "ThreadPool construct begin size = " << num_threads << "\n";
    // Initialize work threads
    for (size_t i = 0; i < num_threads; ++i) {
      m_workers.emplace_back(WorkerThread(this));
    }
    // std::cout << "ThreadPool construct end" << "\n";
  }

  ~ThreadPool() {
    // std::cout << "Thread pool destruct begin\n";
    Shutdown();
    // std::cout << "Thread pool destruct end\n";
  }

  // Shutdown thread pool
  void Shutdown() {
    {
      std::lock_guard<std::mutex> lock(m_queue_mtx);
      m_stop = true;
    }

    m_cv.notify_all(); // wake up all threads to finish remaining threads

    for (auto &thread : m_workers) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  template <typename F, typename... Args>
  auto AddTask(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
    // create a function with bounded parameters ready to execute
    auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

    // Encapsulate it into a shared ptr in order to be able to copy construct /
    // assign
    auto task_ptr =
        std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

    // wrap the task pointer into a valid lambda
    auto wrapper_func = [task_ptr]() { (*task_ptr)(); };

    {
      std::lock_guard<std::mutex> lock(m_queue_mtx);
      m_tasks.push(wrapper_func);
      // wake up one thread if it's waiting
      m_cv.notify_one();
      std::cout << "notify_one()" << std::endl;
    }

    // return future from promise
    return task_ptr->get_future();
  }

private:
  std::mutex m_queue_mtx;
  std::condition_variable m_cv;
  bool m_stop;

  std::vector<std::thread> m_workers;
  std::queue<std::function<void()>> m_tasks;

  class WorkerThread {
  public:
    explicit WorkerThread(ThreadPool *pool) : thread_pool(pool) {}

    void operator()() {
      while (!thread_pool->m_stop ||
             (thread_pool->m_stop && !thread_pool->m_tasks.empty())) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(thread_pool->m_queue_mtx);
          std::cout << "Thread[" << std::this_thread::get_id()
                    << "] waiting...()" << std::endl;
          this->thread_pool->m_cv.wait(lock, [this] {
            return this->thread_pool->m_stop ||
                   !this->thread_pool->m_tasks.empty();
          });
          std::cout << "Thread[" << std::this_thread::get_id() << "] awake()"
                    << std::endl;

          if (this->thread_pool->m_stop && this->thread_pool->m_tasks.empty()) {
            return;
          }
          task = std::move(this->thread_pool->m_tasks.front());
          thread_pool->m_tasks.pop();
        }

        std::cout << "Thread[" << std::this_thread::get_id()
                  << "] executing task begin" << std::endl;
        task();
        std::cout << "Thread[" << std::this_thread::get_id()
                  << "] executing task end" << std::endl;
      }
    }

  private:
    ThreadPool *thread_pool;
  };
};
