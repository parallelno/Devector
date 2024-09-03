#pragma once

#include "Utils/result.h"
#include "Utils/utils.h"
#include "Utils/consts.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

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

        // _waitTime = -1.0 : wait until data received
        // _waitTime = 0.0  : no wait
        // _waitTime > 0.0  : wait until data received or time is over
        Result<T> pop(const double _waitTime = -1.0)
        {
            std::unique_lock<std::mutex> mlock(m_mutex);

            if (_waitTime == 0.0)  //-V550
            {    
                if (m_queue.empty()) return {};
            }
            else if (_waitTime < 0) {
                while (m_queue.empty())
                {
                    m_condition.wait(mlock);
                }
            }
            else if (m_queue.empty())
            {
                m_condition.wait_for(mlock, (_waitTime * 1000.0) * 1ms);
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
            m_queue.emplace(_item);
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