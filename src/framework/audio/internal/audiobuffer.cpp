/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
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
#include "audiobuffer.h"

#include "audiosanitizer.h"
#include "log.h"

using namespace muse::audio;

static constexpr size_t CAPACITY = 8192;
static constexpr size_t TARGET_BUFFER_SIZE = 1024;

//#define DEBUG_AUDIO
#ifdef DEBUG_AUDIO
#define LOG_AUDIO LOGD
#else
#define LOG_AUDIO LOGN
#endif

struct BaseBufferProfiler {
    size_t reservedFramesMax = 0;
    size_t reservedFramesMin = 0;
    double elapsedMax = 0.0;
    double elapsedSum = 0.0;
    double elapsedMin = 0.0;
    uint64_t callCount = 0;
    uint64_t maxCallCount = 0;
    std::string tag;

    BaseBufferProfiler(std::string&& profileTag, uint64_t profilerMaxCalls)
        : maxCallCount(profilerMaxCalls), tag(std::move(profileTag)) {}

    ~BaseBufferProfiler()
    {
        if (!stopped()) {
            return;
        }

        std::cout << "\n BUFFER PROFILE:    " << tag;
        std::cout << "\n reservedFramesMax: " << reservedFramesMax;
        std::cout << "\n reservedFramesMin: " << reservedFramesMin;
        std::cout << "\n elapsedMax:        " << elapsedMax;
        std::cout << "\n elapsedMin:        " << elapsedMin;
        std::cout << "\n elapsedAvg:        " << elapsedSum / callCount;
        std::cout << "\n total call count:  " << callCount;
        std::cout << "\n ===================\n";
    }

    void add(size_t reservedFrames, double elapsed)
    {
        if (maxCallCount != 0 && callCount > maxCallCount) {
            return;
        }

        callCount++;

        if (callCount == 1) {
            reservedFramesMin = reservedFrames;
            elapsedMin = elapsed;
        } else {
            reservedFramesMin = std::min(reservedFrames, reservedFramesMin);
            elapsedMin = std::min(elapsed, elapsedMin);
        }

        reservedFramesMax = std::max(reservedFrames, reservedFramesMax);
        elapsedMax = std::max(elapsed, elapsedMax);
        elapsedSum += elapsed;
    }

    bool stopped() const
    {
        return callCount >= maxCallCount && maxCallCount != 0;
    }

    void stop()
    {
        if (stopped()) {
            return;
        }

        maxCallCount = callCount - 1;
        LOGD() << "\n PROFILE STOP";
    }
};

static BaseBufferProfiler READ_PROFILE("READ_PROFILE", 3000);
static BaseBufferProfiler WRITE_PROFILE("WRITE_PROFILE", 0);

void AudioBuffer::init(const audioch_t audioChannelsCount, const samples_t renderStep)
{
    m_audioChannelsCount = audioChannelsCount;
    m_targetBufferSize = TARGET_BUFFER_SIZE * audioChannelsCount;
    m_renderStep = renderStep;

    m_data.resize(CAPACITY, 0.f);
}

void AudioBuffer::setSource(std::shared_ptr<IAudioSource> source)
{
    if (m_source == source) {
        return;
    }

    if (m_source) {
        reset();
    }

    m_source = source;
}

void AudioBuffer::forward()
{
    if (!m_source) {
        return;
    }

    size_t nextWriteIdx = m_writeIndex.load(std::memory_order_relaxed);
    const auto currentReadIdx = m_readIndex.load(std::memory_order_acquire);

    while (reservedFrames(nextWriteIdx, currentReadIdx) < m_targetBufferSize) {
        m_source->process(m_data.data() + nextWriteIdx, m_renderStep);
        nextWriteIdx += m_renderStep * m_audioChannelsCount;
        if (nextWriteIdx >= CAPACITY) {
            nextWriteIdx -= CAPACITY;
        }
    }

    m_writeIndex.store(nextWriteIdx, std::memory_order_release);
}

void AudioBuffer::pop(float* dest, size_t sampleCount)
{
    const auto currentReadIdx = m_readIndex.load(std::memory_order_relaxed);
    const auto currentWriteIdx = m_writeIndex.load(std::memory_order_acquire);
    if (currentReadIdx == currentWriteIdx) { // empty queue
        std::fill(dest, dest + sampleCount * m_audioChannelsCount, 0.f);
        return;
    }

    size_t newReadIdx = currentReadIdx;

    const size_t from = newReadIdx;
    const size_t to = std::min<size_t>(CAPACITY, from + sampleCount * m_audioChannelsCount);

    const auto right = to - from;
    std::memcpy(dest, m_data.data() + from, right * sizeof(float));
    newReadIdx += right;

    size_t left = sampleCount * m_audioChannelsCount - right;
    if (left > 0) {
        std::memcpy(dest + right, m_data.data(), left * sizeof(float));
        newReadIdx = left;
    }

    if (newReadIdx >= CAPACITY) {
        newReadIdx -= CAPACITY;
    }

    m_readIndex.store(newReadIdx, std::memory_order_release);
}

void AudioBuffer::setMinSamplesToReserve(size_t /*lag*/)
{
}

void AudioBuffer::reset()
{
    m_readIndex.store(0, std::memory_order_release);
    m_writeIndex.store(0, std::memory_order_release);

    m_data.resize(CAPACITY, 0.f);
}

size_t AudioBuffer::reservedFrames(const size_t writeIdx, const size_t readIdx) const
{
  return readIdx <= writeIdx ? writeIdx - readIdx
                             : writeIdx + CAPACITY - readIdx;
}
