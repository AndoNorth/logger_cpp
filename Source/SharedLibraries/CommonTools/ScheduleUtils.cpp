
#include "stdafx.h"
#include "ScheduleUtils.h"

// ----------------------------
// --      Time Classes      --
// ----------------------------

TimeInterval::TimeInterval(std::chrono::seconds interval)
{
    _interval = interval;
    Set_time_to_now();
}

TimeInterval::TimeInterval(std::chrono::seconds interval, std::chrono::seconds offset, 
     std::chrono::system_clock::time_point time):
    _interval(interval), _offset(offset)
{
    Set_time(time);
}

void TimeInterval::Set_time(std::chrono::system_clock::time_point time) {
    _time = time;
}

void TimeInterval::Set_offset(std::chrono::minutes offset)
{
    if (offset >= _interval) {
        throw std::runtime_error("Offset must be less than the time interval.");
    }

    if (offset < std::chrono::minutes(0)) {
        throw std::runtime_error("Offset cannot be negative");
    }

    _offset = offset;
}

void TimeInterval::Set_time_to_now() {
    _time = std::chrono::system_clock::now();
}

std::chrono::system_clock::time_point TimeInterval::Get_current_timestamp() {

    // First, round down to the nearest interval 
    std::time_t t = std::chrono::system_clock::to_time_t(_time);
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(_interval);
    auto value = seconds.count();
    auto amo = t % value;
    auto timestamp = t - amo;

    // Then apply the offset
    auto offseted = timestamp + std::chrono::duration_cast<std::chrono::seconds>(_offset).count();
    if (offseted > t) {
        offseted -= value;
    }

    return std::chrono::system_clock::from_time_t(offseted);
}

std::chrono::system_clock::time_point TimeInterval::Get_next_timestamp() {
    auto previous = Get_current_timestamp();
    auto seconds_previous = std::chrono::system_clock::to_time_t(previous);

    // Simply add seconds to it
    auto interval_seconds = std::chrono::duration_cast<std::chrono::seconds>(_interval).count();
    auto next_seconds = seconds_previous + interval_seconds;

    return std::chrono::system_clock::time_point(std::chrono::seconds(next_seconds));
}

std::chrono::milliseconds TimeInterval::Get_milliseconds_until_next_timestamp() {
    auto next_timestamp = Get_next_timestamp();
    auto next_seconds = std::chrono::system_clock::to_time_t(next_timestamp);

    auto time_seconds = std::chrono::system_clock::to_time_t(_time);

    auto difference = next_seconds - time_seconds;

    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(difference));
}


// ------------------------------
// --     Scheduled Caller     --
// ------------------------------

ScheduledCaller::~ScheduledCaller()
{
    Stop();
}

std::chrono::system_clock::time_point ScheduledCaller::Get_previous_scheduled_time()
{
    return _interval.Get_current_timestamp();
}

std::chrono::system_clock::time_point ScheduledCaller::Get_next_scheduled_time()
{
    return _interval.Get_next_timestamp();
}

void ScheduledCaller::Stop_after_call()
{
    _thread.request_stop();
}

void ScheduledCaller::Request_call_early(std::chrono::milliseconds time)
{
    _optional_call_time = time;
}

std::chrono::milliseconds ScheduledCaller::Get_millis_until_next_call()
{
    return _interval.Get_milliseconds_until_next_timestamp();
}

TimeInterval ScheduledCaller::Get_time_interval() {
    return _interval;
}

void ScheduledCaller::Start()
{
    if (Is_running()) {
        return;
    }

    _is_running = true;
    
    _thread = std::jthread([this](std::stop_token stop_token) {
        auto condition = [&stop_token]() {return stop_token.stop_requested();};

        while (true) {

            std::unique_lock<std::mutex> lock(_mutex);

            _interval.Set_time_to_now();

            auto milliseconds = (_optional_call_time.has_value()) ?
                _optional_call_time.value() :
                Get_millis_until_next_call();
            

            if (_condition_variable.wait_for(lock, milliseconds, condition)) {
                // Stopping
                break;
            }
            _optional_call_time = std::nullopt;

            _func();
        }
    });
}

void ScheduledCaller::Stop()
{
    _thread.request_stop();
    _condition_variable.notify_all();
    if (_thread.joinable()) {
        _thread.join();
    }
    _is_running = false;
}

bool ScheduledCaller::Is_running()
{
    return _is_running;
}

void ScheduledCaller::Call_immediately()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _func();
}
