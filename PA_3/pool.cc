#include "pool.h"
#include <mutex>
#include <iostream>

Task::Task() = default;
Task::~Task() = default;

ThreadPool::ThreadPool(int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(new std::thread(&ThreadPool::run_thread, this));
    }
}

ThreadPool::~ThreadPool() {
    for (std::thread *t: threads) {
        delete t;
    }
    threads.clear();

    for (Task *q: queue) {
        delete q;
    }
    queue.clear();
}

void ThreadPool::SubmitTask(const std::string &name, Task *task) {
    //TODO: Add task to queue, make sure to lock the queue
    if (done){
        printf("Cannot added task to queue\n");
        return;
    }
    mtx.lock();
    task->name = name;
    queue.push_back(task);
    printf("Added task\n");
    mtx.unlock();
}

void ThreadPool::run_thread() {
    while (true) {
        num_tasks_unserviced = queue.size();
        //TODO1: if done and no tasks left, break
        if(done && num_tasks_unserviced <= 0){break;}

        //TODO2: if no tasks left, continue
        if(num_tasks_unserviced <= 0){continue;}
       
        //TODO3: get task from queue, remove it from queue, and run it
        Task* task = nullptr;
        for (Task* t : queue){
            if (!t->is_running()){
                task = t;
                task->running = true;
                remove_task(task);
                printf("Started task\n");
                task->Run();
                printf("Finished task\n");
                break;
            }
        }

        //TODO4: delete task
        mtx.lock();
        if(task != nullptr) {
            delete task;
        }
        mtx.unlock();
    }
}

// Remove Task t from queue if it's there
void ThreadPool::remove_task(Task *t) {
    mtx.lock();
    for (auto it = queue.begin(); it != queue.end();) {
        if (*it == t) {
            queue.erase(it);
            mtx.unlock();
            return;
        }
        ++it;
    }
    mtx.unlock();
}

void ThreadPool::Stop() {
    //TODO: Delete threads, but remember to wait for them to finish first
    printf("Called Stop()\n");
    done = true;
    for (std::thread* thread : threads){
        printf("Stopping thread\n");
        thread->join();
        delete thread;
    }
    threads.clear();
}
