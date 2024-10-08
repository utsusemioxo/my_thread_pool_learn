#include <iostream>
#include <chrono>
#include <thread>
#include "thread_pool.h"

void SimpleTask(int id) {
  // std::cout << "Task " << id << " is starting." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(3));
  // std::cout << "Task " << id << " is finished." << std::endl;
}

int main() {
  ThreadPool pool(4);

  std::vector<std::future<void>> futures;
  for (int i = 1; i <= 16; ++i) {
    // std::cout << "push future " << i << " begin\n";
    futures.push_back(pool.AddTask(SimpleTask, i));
    // std::cout << "push future " << i << " end\n";
  }

  for (auto &future : futures) {
    future.get();
  }

  pool.Shutdown();

  std::cout << "All tasks completed and thread pool shutdown.\n";

  return 0;
}