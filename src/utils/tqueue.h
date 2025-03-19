#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "utils/result.h"
#include "utils/utils.h"
#include "utils/consts.h"

using namespace std::chrono_literals;

namespace dev {

    template <typename T>
    class TQueue
    {
        std::queue<T> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_condition;
        size_t m_maxLegth;

    public:
        TQueue(size_t _maxLegth = 10) : m_maxLegth(_maxLegth) {}
        TQueue(const TQueue&) = delete;            // disable copying
        TQueue& operator=(const TQueue&) = delete; // disable assignment

        // _waitTime is in nanoseconds
        // _waitTime < 0 : wait until data received
        // _waitTime = 0  : no wait
        // _waitTime > 0  : wait until data received or time is over
        Result<T> pop(const std::chrono::duration<int64_t, std::nano> _waitTime = -1ns)
        {
            std::unique_lock<std::mutex> mlock(m_mutex);

            if (_waitTime == 0ns)  //-V550
            {    
                if (m_queue.empty()) return {};
            }
            else if (_waitTime < 0ns) {
                while (m_queue.empty())
                {
                    m_condition.wait(mlock);
                }
            }
            else if (m_queue.empty())
            {
                m_condition.wait_for(mlock, _waitTime);
                if (m_queue.empty()) return {};
            }
            auto value = m_queue.front();
            m_queue.pop();
            return { std::move(value) };
        }

        void push(const T& _item)
        {
            std::unique_lock<std::mutex> mlock(m_mutex);
            if (m_queue.size() > m_maxLegth) {
                m_queue.pop();
            }
            m_queue.push(_item);
            mlock.unlock();
            m_condition.notify_one();
        }

        void emplace(const T& _item)
        {
            std::unique_lock<std::mutex> mlock(m_mutex);
            if (m_queue.size() > m_maxLegth) {
                m_queue.pop();
            }
            m_queue.emplace(std::move(_item));
            mlock.unlock();
            m_condition.notify_one();
        }

        inline void clear()
        {
            std::unique_lock<std::mutex> mlock(m_mutex);
            m_queue = {};
        }

        inline bool empty()
        {
            std::unique_lock<std::mutex> mlock(m_mutex);
            return m_queue.empty();
        }
    };
}