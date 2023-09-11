// Copyright RedFox Studio 2022

#pragma once

#include "asserts.h"

#include <atomic>

namespace Fox
{
class CAtomicCounter
{
  public:
    inline void IncreaseCounter() { _counter.store(_counter.load() + 1); };
    inline void DecreaseCounter()
    {
        check(_counter.load() > 0); // count can't be negative
        _counter.store(_counter.load() - 1);
    };
    inline uint32_t Count() const { return _counter.load(); };

  protected:
    // Move constructor
    CAtomicCounter(CAtomicCounter&& other) noexcept : _counter(other._counter.load())
    {
        other._counter.store(0); // Reset the moved counter
    }
    CAtomicCounter() = default;

  private:
    std::atomic<uint32_t> _counter{ 0 };
};
}