#include "event_loop.h"
#include "runtime.h"
#include <iostream>

// Constructor
EventLoop::EventLoop(Runtime* runtime)
    : runtime_(runtime), running_(false), next_task_id_(1) {
}

// Destructor
EventLoop::~EventLoop() {
    Stop();
}

// Start the event loop
void EventLoop::Start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    thread_ = std::thread(&EventLoop::Run, this);
}

// Stop the event loop
void EventLoop::Stop() {
    if (!running_) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        running_ = false;
        queue_cv_.notify_one();
    }
    
    if (thread_.joinable()) {
        thread_.join();
    }
}

// Schedule a task to be executed on the event loop
void EventLoop::ScheduleTask(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push(task);
    queue_cv_.notify_one();
}

// Schedule a task to be executed after a delay (in milliseconds)
uint64_t EventLoop::ScheduleDelayedTask(std::function<void()> task, uint64_t delay_ms) {
    std::lock_guard<std::mutex> lock(delayed_tasks_mutex_);
    uint64_t task_id = next_task_id_++;
    auto execution_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
    delayed_tasks_[task_id] = std::make_pair(execution_time, task);
    return task_id;
}

// Cancel a delayed task
void EventLoop::CancelDelayedTask(uint64_t task_id) {
    std::lock_guard<std::mutex> lock(delayed_tasks_mutex_);
    delayed_tasks_.erase(task_id);
}

// Check if the event loop is running
bool EventLoop::IsRunning() const {
    return running_;
}

// Event loop thread function
void EventLoop::Run() {
    while (running_) {
        // Process delayed tasks
        ProcessDelayedTasks();
        
        // Process regular tasks
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (task_queue_.empty()) {
                // Wait for a task or timeout after 10ms to check delayed tasks
                queue_cv_.wait_for(lock, std::chrono::milliseconds(10), [this] {
                    return !task_queue_.empty() || !running_;
                });
                
                if (!running_) {
                    break;
                }
                
                if (task_queue_.empty()) {
                    continue;
                }
            }
            
            task = task_queue_.front();
            task_queue_.pop();
        }
        
        // Execute the task
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "Exception in event loop task: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in event loop task" << std::endl;
        }
    }
}

// Process delayed tasks
void EventLoop::ProcessDelayedTasks() {
    std::vector<std::function<void()>> tasks_to_run;
    
    {
        std::lock_guard<std::mutex> lock(delayed_tasks_mutex_);
        auto now = std::chrono::steady_clock::now();
        
        auto it = delayed_tasks_.begin();
        while (it != delayed_tasks_.end()) {
            if (it->second.first <= now) {
                tasks_to_run.push_back(it->second.second);
                it = delayed_tasks_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Execute tasks that are due
    for (const auto& task : tasks_to_run) {
        ScheduleTask(task);
    }
} 