#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <optional>
#include <thread>

#ifndef _WINDOWS
#define COMMONTOOLS_EXPORT
#endif

/**
 * Helps to create timestamps at a specific interval, such as every 5 minutes.
 * 
 * When you set_time() to the current time, it will round down to the interval
 * when calling get_current_data_timestamp(). For example, if we set_time() to 
 * 9:47pm, then get_current_data_timestamp() will return 9:45pm. 
 */
class COMMONTOOLS_EXPORT TimeInterval {
private:        
    /**
     * The time point to base the next scheduled time on. Call 
     * set_time() or set_time_to_now() to modify
     */
    std::chrono::system_clock::time_point _time;
public:
    /**
     * How often the data is requested.
     */
    std::chrono::seconds _interval; 

    /**
     * How many minutes should we wait to start? For example, does something happen every 30 minutes, but start
     * on the 15th minute? So it would occur at 1:15, 1:45, 2:15, 2:45 and so forth?
     */
    std::chrono::seconds _offset = std::chrono::seconds(0);

    /**
     * Creates a TimeSequence given how often something should occur, in minutes.
     * Then sets the time to the current time.
     * 
     * @param interval How often should something occur, in minutes?
     */
    TimeInterval(std::chrono::seconds interval);

    /**
     * Used to call something repeatedly at a scheduled time, such as every hour, or
     * every 3 hours offsetted by 10 minutes (3:10, 6:10, 9:10, etc.)
     *  
     * @param interval How often should something occur?
     * @param offset Should all times be offsetted by a few minutes or hours?
     * @param time What time should the next schedule be based on. Defaults to the current time.
     */
    TimeInterval(std::chrono::seconds interval, std::chrono::seconds offset, 
        std::chrono::system_clock::time_point time = std::chrono::system_clock::now());

    ///**
    // * Creates a TimeSequence
    // * 
    // * @param interval How often should something occur in minutes?
    // * @param time The time to create intervals on. Useful for setting to a time in the past or future.
    // */
    //TimeInterval(std::chrono::seconds interval, std::chrono::system_clock::time_point time);

    virtual ~TimeInterval() = default;
    TimeInterval(const TimeInterval&) = default;
    TimeInterval(TimeInterval&&) = default;
    TimeInterval& operator=(const TimeInterval&) = default;
    TimeInterval& operator=(TimeInterval&&) = default;

    /**
     * Sets the time.
     */
    void Set_time(std::chrono::system_clock::time_point time);

    /**
     * Sets the offset. The offset must be smaller than the interval, and non-negative.
     */
    void Set_offset(std::chrono::minutes offset);

    /**
     * Sets the time to the current time.
     */
    void Set_time_to_now();

    /**
     * Gets the last scheduled time something is scheduled to occur. If the 
     * _interval is 5 minutes, and you set_time() to 9:47pm, this will return 
     * 9:45pm. 
     * 
     * @return The time_point which something was scheduled to occur last.
     */
    std::chrono::system_clock::time_point Get_current_timestamp(); 

    /**
     * Gets the next time for the interval in the future. If the _interval is
     * 5 minutes, and you set_time() to 9:47pm, this will return 9:50pm
     * 
     * @return The time point in the interval when something is scheduled next.
     */
    std::chrono::system_clock::time_point Get_next_timestamp();

    /**
     * Gets the number of milliseconds between _time and the next timestamp.
     * For the current time, call set_time(), then call this.
     * 
     * @return Milliseconds until the next timestamp
     */
    std::chrono::milliseconds Get_milliseconds_until_next_timestamp();
};


/**
 * Runs a function over and over at scheduled times.
 */
class COMMONTOOLS_EXPORT ScheduledCaller {
private:

    /**
     * How often, and when should this function run? 
     */
    TimeInterval _interval;

    /**
     * If this has a value, it will use this number of milliseconds instead
     * of the scheduled time. This is useful if after your hourly API request,
     * it fails, and you want to try again in 30 seconds. 
     */
    std::optional<std::chrono::milliseconds> _optional_call_time;

    /**
     * The function to call at scheduled intervals.
     * 
     * If you need the function to be called again shortly, return a time.
     * Otherwise, for regular scheduled calling, return std::nullopt.
     * 
     * For example, if a network request was unsuccessful, and you want to wait
     * for a minute and try again, return std::chrono::minutes(1). But if it was
     * successful, return std::nullopt to be called on the next scheduled interval.
     */
    std::function<void()> _func;

    /**
     * Runs the thread at a specific time, but also allows
     * for early wakeup in case we need to stop any 
     * future calls.
     */
    std::condition_variable _condition_variable;

    /**
     * Protects data in this class between the threads.
     */
    std::mutex _mutex;

    /**
     * The thread which calls the function.
     */
    std::jthread _thread;

    /**
    * Are we currently calling the function?
    */
    std::atomic<bool> _is_running;
public:
    /**
     * Constructs a new ScheduledCaller, which periodically calls a function
     * on a scheduled time interval
     * 
     * @param func The std::function to call 
     * @param interval When to call the function. Every 5 minutes? Every hour on
     * the 15 minute mark?
     * 
     */
    ScheduledCaller(std::function<void()> func,
        TimeInterval _interval) : 
        _interval(_interval), 
        _func(func) {
    }

    /**
     * Destructor. Will try to stop the caller if it wasn't stopped already.
     */
    virtual ~ScheduledCaller();

    // Copy and Assignment does not copy the thread, only copies the interval and func.
    ScheduledCaller(const ScheduledCaller& other) 
        : _interval(other._interval),
          _func(other._func) {}
    ScheduledCaller operator=(const ScheduledCaller& other) noexcept {
        return ScheduledCaller(other);
    }

    /**
     * Get the timestamp for the next scheduled time to call
     * the function
     * 
     * @return Returns a time_point when something is scheduled for.
     */
    std::chrono::system_clock::time_point Get_next_scheduled_time();

    /**
     * Get the timestamp for the previous scheduled time to call 
     * the function, even if it did not occur at that time.
     * 
     * @return Returns a time_point when something was last scheduled.
     */
    std::chrono::system_clock::time_point Get_previous_scheduled_time();

    /**
     * Indicates that we should stop after the next call. Useful for 
     * stopping this caller inside func()
     * 
     */
    void Stop_after_call();

    /**
    * Calls the function in a specified amount of time next, instead
    * of the scheduled time one time.
     */
    void Request_call_early(std::chrono::milliseconds time);

    /**
     * Gets a copy of the time interval.
     * 
     * @return A copy of the current time interval.
     */
    TimeInterval Get_time_interval();

    /**
     * Gets the number of milliseconds to the next call.
     * 
     * @return A time duration from now to the next call.
     */
    std::chrono::milliseconds Get_millis_until_next_call();

    /**
     * Continually call a function at the set interval.
     */
    void Start();

    /**
     * Stops calling the function
     */
    void Stop();

    /**
     * Returns true if it is running. This function is thread-safe.
     * 
     * @return 
     */
    bool Is_running();

    /**
     * When you need to call the function immediately. Resets the time.
     */
    void Call_immediately();
};

