/**
 * \file semaphore.hpp
 * \author Graham Riches (graham.riches@live.com)
 * \brief C++17 Implementation of a semaphore until the toolchain is updated to C++20
 *        that conforms to the API of the C++20 semaphore so that it can easily be updated
 *        in the future.
 * \version 0.1
 * \date 2021-09-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#pragma once

/********************************** Includes *******************************************/
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <atomic>
#include <algorithm>

/********************************** Types *******************************************/
namespace std
{
template <std::ptrdiff_t LeastMaxValue>
class counting_semaphore {
  public:
    //!< Constructors and assignment operators    
    /**
     * \brief Constructs a counting semaphore with the internal counter initialized to desired
     * \param desired The initial counter value
     */
    constexpr explicit counting_semaphore(std::ptrdiff_t desired = 0)
        : m_count(desired) { }

    /**
    * \brief Destroy the counting semaphore object
    * \note UB if any thread is pending on this semaphore
    */
    ~counting_semaphore() = default;

    //!< Sempahore is not copyable or movable
    counting_semaphore(const counting_semaphore& other) = delete;
    counting_semaphore& operator=(const counting_semaphore& other) = delete;
    counting_semaphore(counting_semaphore&& other) = delete;
    counting_semaphore&& operator=(counting_semaphore&& other) = delete;

    
    //!< Member functions
    /**
     * \brief Atomically increments the internal counter by the value update
     * \note Any thread waiting (via acquire) on the counter value may be released by this
     * \param update The value to update the counter by
     */
    void release(std::ptrdiff_t update = 1) {
        std::unique_lock<std::mutex> lock(m_lock);
        m_count = std::clamp(m_count + update, std::ptrdiff_t{0}, LeastMaxValue);   
        m_cv.notify_all();
        lock.unlock();
    }

    /**
     * \brief Atomically decrements the internal counter value by 1 if it is greater than 0, otherwise blocks
     *        until the counter is greater than 0
     */
    void acquire() {
        std::unique_lock<std::mutex> lock(m_lock);        
        m_cv.wait(lock, [&] { return (m_count > 0); } );
        m_count = std::clamp(m_count - 1, std::ptrdiff_t{0}, LeastMaxValue);
        m_cv.notify_all();
        lock.unlock();
    }

    /**
     * \brief Tries to atomically decrement the counter by 1 if it's greater than 0, otherwise returns error
     * 
     * \retval true If counter was decremented
     * \retval false Otherwise
     */
    bool try_acquire() noexcept {
        std::unique_lock<std::mutex> lock(m_lock);
        if (lock.try_lock()) {
            m_count = std::clamp(m_count - 1, std::ptrdiff_t{0}, LeastMaxValue);
            lock.unlock();
            return true;
        }
        return false;
    }

    /**
     * \brief Tries to atomically decrement the internal counter by 1, otherwise blocks for a period of time
     *      
     * \param rel_time The time period to wait for
     * \retval true If counter decremented by 1 within the time period
     * \retval false Otherwise 
     */
    template <class Rep, class Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time) {
        std::unique_lock<std::mutex> lock(m_lock);
        if (m_cv.wait_for(lock, rel_time, [&]{ return (m_count > 0); })) {
            m_count = std::clamp(m_count - 1, std::ptrdiff_t{0}, LeastMaxValue);
            return true;
        } else {
            return false;
        }                
    }

    /**
     * \brief Tries to atomically decrement the internal counter by 1, otherwise blocks until a set point in time
     *      
     * \param abs_time The time point to wait until
     * \retval true if counter decremented before time period passes
     * \retval false otherwise
     */
    template<class Clock, class Duration>
    bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time) {        
        std::unique_lock<std::mutex> lock(m_lock);
        if (m_cv.wait_until(lock, abs_time, [&]{ return (m_count > 0); })) {
            m_count = std::clamp(m_count - 1, std::ptrdiff_t{0}, LeastMaxValue);
            return true;
        } else {
            return false;
        }
    }

    //!< Constants
    /**
     * \brief Returns the internal counter's maximum possible value, which is greater than
     *        or equal to LeastMaxValue. For specialization binary_semaphore, LeastMaxValue is equal to 1.
     * 
     * \retval std::ptrdiff_t value
     */
    constexpr std::ptrdiff_t max() noexcept {
        return LeastMaxValue;
    }

  private:
    std::ptrdiff_t m_count;
    std::condition_variable m_cv;
    std::mutex m_lock;
};

using binary_semaphore = counting_semaphore<1>;

};  // namespace std