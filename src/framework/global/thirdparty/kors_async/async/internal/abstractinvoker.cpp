/*
MIT License

Copyright (c) 2020 Igor Korsukov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "abstractinvoker.h"

#include <cassert>

#include "queuedinvoker.h"

using namespace kors::async;

AbstractInvoker::AbstractInvoker()
{
}

AbstractInvoker::~AbstractInvoker()
{
    std::lock_guard<std::mutex> lock(m_qInvokersMutex);
    for (QInvoker* qi : m_qInvokers) {
        qi->invalidate();
    }

    m_qInvokers.clear();
}

void AbstractInvoker::invoke(int type)
{
    invoke(type, NotifyData());
}

void AbstractInvoker::invoke(int type, const NotifyData& data)
{
    //! NOTE: Copy callbacks under lock so that concurrent add/remove on
    //! another thread doesn't race with our iteration. Each CallBack carries
    //! its own shared_ptr to the call, so the call object stays alive for
    //! the duration of this invocation even if it is removed from
    //! m_callbacks concurrently.
    CallBacks callbacks;
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        auto it = m_callbacks.find(type);
        if (it == m_callbacks.end()) {
            return;
        }
        callbacks = it->second;
    }

    std::thread::id threadID = std::this_thread::get_id();

    for (const CallBack& c : callbacks) {
        if (c.threadID == threadID) {
            invokeCallback(type, c, data);
        } else {
            QInvoker* qi = new QInvoker(this, type, c.receiver, c.call, data);
            QueuedInvoker::instance()->invoke(c.threadID, [qi]() {
                qi->invoke();
                delete qi;
            });
        }
    }
}

void AbstractInvoker::invokeCallback(int type, const CallBack& c, const NotifyData& data)
{
    assert(c.threadID == std::this_thread::get_id());

    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (!containsReceiver(c.receiver)) {
            return;
        }
    }

    if (c.receiver && !c.receiver->isConnectedAsync()) {
        return;
    }

    doInvoke(type, c.call.get(), data);
}

void AbstractInvoker::invokeQueuedCallback(int type, Asyncable* receiver, void* call, const NotifyData& data)
{
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (!containsReceiver(receiver)) {
            return;
        }
    }

    if (receiver && !receiver->isConnectedAsync()) {
        return;
    }

    doInvoke(type, call, data);
}

void AbstractInvoker::processEvents()
{
    QueuedInvoker::instance()->processEvents();
}

void AbstractInvoker::onMainThreadInvoke(const std::function<void(const std::function<void()>&, bool)>& f)
{
    QueuedInvoker::instance()->onMainThreadInvoke(f);
}

bool AbstractInvoker::isConnected() const
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    for (auto it = m_callbacks.cbegin(); it != m_callbacks.cend(); ++it) {
        const CallBacks& cs = it->second;
        if (cs.size() > 0) {
            return true;
        }
    }
    return false;
}

int AbstractInvoker::CallBacks::receiverIndexOf(Asyncable* receiver) const
{
    for (size_t i = 0; i < size(); ++i) {
        if (at(i).receiver == receiver) {
            return int(i);
        }
    }
    return -1;
}

bool AbstractInvoker::CallBacks::containsReceiver(Asyncable* receiver) const
{
    return receiverIndexOf(receiver) > -1;
}

void AbstractInvoker::removeCallBack(int type, Asyncable* receiver)
{
    std::shared_ptr<void> keepAlive; // drop refcount after releasing the lock
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        auto it = m_callbacks.find(type);
        if (it == m_callbacks.end()) {
            return;
        }

        CallBacks& callbacks = it->second;
        int index = callbacks.receiverIndexOf(receiver);
        if (index < 0) {
            return;
        }

        CallBack c = callbacks.at(index);
        if (c.receiver) {
            c.receiver->disconnectAsync(this);
        }
        keepAlive = std::move(c.call);
        callbacks.erase(callbacks.begin() + index);
    }
    //! NOTE: keepAlive (the strong ref to the call) is released here, outside
    //! the lock. Any pending QInvoker that still holds a weak_ptr will see
    //! it expired on its next invoke() and skip.
}

void AbstractInvoker::removeAllCallBacks()
{
    std::map<int, CallBacks> taken;
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        taken.swap(m_callbacks);
    }

    for (auto it = taken.begin(); it != taken.end(); ++it) {
        for (CallBack& c : it->second) {
            if (c.receiver) {
                c.receiver->disconnectAsync(this);
            }
        }
    }
    //! NOTE: 'taken' goes out of scope here, dropping every call's strong ref
    //! outside the lock. Pending QInvokers observe the weak_ptr expire.
}

void AbstractInvoker::addCallBack(int type, Asyncable* receiver, std::shared_ptr<void> call, Asyncable::AsyncMode mode)
{
    bool needRemoveFirst = false;
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        const CallBacks& callbacks = m_callbacks[type];
        if (callbacks.containsReceiver(receiver)) {
            switch (mode) {
            case Asyncable::AsyncMode::AsyncSetOnce:
                // Drop the new call (lock released on scope exit)
                return;
            case Asyncable::AsyncMode::AsyncSetRepeat:
                needRemoveFirst = true;
                break;
            }
        }
    }

    if (needRemoveFirst) {
        removeCallBack(type, receiver);
    }

    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        CallBack c(std::this_thread::get_id(), type, receiver, std::move(call));
        m_callbacks[type].push_back(c);
        if (c.receiver) {
            c.receiver->connectAsync(this);
        }
    }
}

void AbstractInvoker::disconnectAsync(Asyncable* receiver)
{
    std::vector<int> types;
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        for (auto it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
            for (CallBack& c : it->second) {
                if (c.receiver == receiver) {
                    types.push_back(c.type);
                }
            }
        }
    }

    for (int type : types) {
        removeCallBack(type, receiver);
    }
}

void AbstractInvoker::addQInvoker(QInvoker* qi)
{
    std::lock_guard<std::mutex> lock(m_qInvokersMutex);
    m_qInvokers.push_back(qi);
}

void AbstractInvoker::removeQInvoker(QInvoker* qi)
{
    std::lock_guard<std::mutex> lock(m_qInvokersMutex);
    m_qInvokers.remove(qi);
}

bool AbstractInvoker::containsReceiver(Asyncable* receiver) const
{
    for (auto it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
        for (const CallBack& c : it->second) {
            if (c.receiver == receiver) {
                return true;
            }
        }
    }

    return false;
}
