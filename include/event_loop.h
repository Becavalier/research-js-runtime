#ifndef TINY_NODEJS_EVENT_LOOP_H
#define TINY_NODEJS_EVENT_LOOP_H

#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <map>
#include <vector>

// Forward declaration
class Runtime;

/**
 * @brief Event loop for handling asynchronous operations
 * 
 * The EventLoop class is responsible for:
 * - Running tasks asynchronously in a separate thread
 * - Scheduling delayed tasks (for setTimeout/setInterval)
 * - Managing the execution order of asynchronous operations
 * 
 * This is a simplified version of Node.js's event loop, which is based on libuv.
 */
class EventLoop {
public:
    /**
     * @brief Constructor for the EventLoop class
     * 
     * @param runtime Pointer to the Runtime instance that owns this event loop
     */
    EventLoop(Runtime* runtime);
    
    /**
     * @brief Destructor for the EventLoop class
     * 
     * Stops the event loop if it's still running and cleans up resources.
     */
    ~EventLoop();
    
    /**
     * @brief Start the event loop
     * 
     * Starts the event loop thread that processes tasks and delayed tasks.
     */
    void Start();
    
    /**
     * @brief Stop the event loop
     * 
     * Stops the event loop thread and waits for it to finish.
     */
    void Stop();
    
    /**
     * @brief Schedule a task to be executed on the event loop
     * 
     * The task will be executed as soon as possible on the event loop thread.
     * 
     * @param task Function to be executed
     */
    void ScheduleTask(std::function<void()> task);
    
    /**
     * @brief Schedule a task to be executed after a delay
     * 
     * The task will be executed after the specified delay on the event loop thread.
     * This is used to implement setTimeout in JavaScript.
     * 
     * @param task Function to be executed
     * @param delay_ms Delay in milliseconds
     * @return Task ID that can be used to cancel the task
     */
    uint64_t ScheduleDelayedTask(std::function<void()> task, uint64_t delay_ms);
    
    /**
     * @brief Cancel a previously scheduled delayed task
     * 
     * This is used to implement clearTimeout in JavaScript.
     * 
     * @param task_id ID of the task to cancel
     */
    void CancelDelayedTask(uint64_t task_id);
    
    /**
     * @brief Check if the event loop is running
     * 
     * @return true if the event loop is running, false otherwise
     */
    bool IsRunning() const;
    
private:
    /**
     * @brief Pointer to the Runtime instance that owns this event loop
     */
    Runtime* runtime_;
    
    /**
     * @brief Thread that runs the event loop
     */
    std::thread thread_;
    
    /**
     * @brief Queue of tasks to be executed
     */
    std::queue<std::function<void()>> task_queue_;
    
    /**
     * @brief Mutex for protecting access to the task queue
     */
    std::mutex queue_mutex_;
    
    /**
     * @brief Condition variable for signaling when tasks are added to the queue
     */
    std::condition_variable queue_cv_;
    
    /**
     * @brief Flag indicating whether the event loop is running
     */
    std::atomic<bool> running_;
    
    /**
     * @brief Counter for generating unique task IDs
     */
    uint64_t next_task_id_;
    
    /**
     * @brief Map of delayed tasks, indexed by task ID
     * 
     * Each entry contains the execution time and the task function.
     */
    std::map<uint64_t, std::pair<std::chrono::steady_clock::time_point, std::function<void()>>> delayed_tasks_;
    
    /**
     * @brief Mutex for protecting access to the delayed tasks map
     */
    std::mutex delayed_tasks_mutex_;
    
    /**
     * @brief Main event loop function that runs in a separate thread
     * 
     * This function processes tasks and delayed tasks until the event loop is stopped.
     */
    void Run();
    
    /**
     * @brief Process delayed tasks that are due for execution
     * 
     * Checks for delayed tasks that have reached their execution time and
     * schedules them for immediate execution.
     */
    void ProcessDelayedTasks();
};

#endif // TINY_NODEJS_EVENT_LOOP_H 