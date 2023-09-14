// Copyright RedFox Studio 2022

#pragma once

#include "asserts.h"

#include <unordered_map>

namespace Fox
{

/*Simple wrapper to simplify map usage */
template<typename Key, class Value, typename HashFn, typename EqualFn>
class RICacheMap
{
  public:
    void Add(const Key& key, const Value& value)
    {
        check(Find(key) == nullptr); // element is already contained

        _map.emplace(key, value);
    }

    Value Find(const Key& key)
    {
        auto it = _map.find(key);
        if (it != _map.end())
            {
                return it->second;
            }
        return nullptr;
    }

    void EraseByValue(Value value)
    {
        for (auto it = _map.begin(); it != _map.end(); it++)
            {
                if (it->second == value)
                    {
                        _map.erase(it);
                        return;
                    }
            }
        check(0); // element is not contained
    }

    size_t Size() const { return _map.size(); }
    void   Clear() { return _map.clear(); }

  private:
    std::unordered_map<Key, Value, HashFn, EqualFn> _map;
};
}