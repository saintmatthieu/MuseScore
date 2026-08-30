/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

namespace muse::audio {
//! Orchestrion's runtime timing probe (see Orchestrion's
//! OrchestrionCommon/PerfTrace.h, whose line format and ORCHESTRION_PERF_LOG
//! switch this mirrors so the audio side lands in the same file, on the same
//! steady-clock axis). Unset, a probe is one branch on a static flag. Set, a
//! probe appends to a memory buffer that a background thread writes out: the
//! probing thread never touches the filesystem (a blocking write there would
//! stall the audio thread it is meant to observe).
class PerfTraceProbe
{
public:
    static bool enabled()
    {
        static const bool on = std::getenv("ORCHESTRION_PERF_LOG") != nullptr;
        return on;
    }

    static long long nowUs()
    {
        using namespace std::chrono;
        return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    }

    static void event(const char* tag, const char* name, long long valueUs, const char* extra = "")
    {
        if (!enabled()) {
            return;
        }
        char line[256];
        const int n = std::snprintf(line, sizeof line, "%lld %s %s %lld %s\n", nowUs(), tag, name, valueUs, extra);
        if (n > 0) {
            Writer::append(line, static_cast<size_t>(n < int(sizeof line) ? n : int(sizeof line) - 1));
        }
    }

private:
    class Writer
    {
    public:
        static void append(const char* line, size_t n)
        {
            Writer* writer = instance();
            if (!writer) {
                return;
            }
            std::lock_guard lock{ writer->m_mutex };
            writer->m_buffer.append(line, n);
        }

    private:
        Writer()
            : m_fd{::open(std::getenv("ORCHESTRION_PERF_LOG"), O_WRONLY | O_APPEND | O_CREAT, 0644) },
            m_thread{ [this] { run(); } }
        {
        }

        ~Writer()
        {
            {
                std::lock_guard lock{ m_mutex };
                m_stop = true;
            }
            m_cv.notify_one();
            m_thread.join();
            if (m_fd >= 0) {
                ::close(m_fd);
            }
        }

        static Writer* instance()
        {
            static Writer* const writer = new Writer;
            static const struct Guard {
                ~Guard() { delete writer; destroyed() = true; }
            } guard;
            return destroyed() ? nullptr : writer;
        }

        static bool& destroyed()
        {
            static bool flag = false;
            return flag;
        }

        void run()
        {
            std::string out;
            while (true) {
                bool stop = false;
                {
                    std::unique_lock lock{ m_mutex };
                    m_cv.wait_for(lock, std::chrono::milliseconds{ 100 }, [this] { return m_stop; });
                    out.swap(m_buffer);
                    stop = m_stop;
                }
                const char* p = out.data();
                size_t left = out.size();
                while (m_fd >= 0 && left > 0) {
                    const ssize_t n = ::write(m_fd, p, left);
                    if (n <= 0) {
                        break;
                    }
                    p += n;
                    left -= static_cast<size_t>(n);
                }
                out.clear();
                if (stop) {
                    return;
                }
            }
        }

        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::string m_buffer;
        bool m_stop = false;
        int m_fd = -1;
        std::thread m_thread;
    };
};
}
