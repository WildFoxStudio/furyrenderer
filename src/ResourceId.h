// Copyright RedFox Studio 2022

#pragma once

#include <stdint.h>

namespace Fox
{

struct Id_T
{
    uint8_t  First;
    uint8_t  Second;
    uint16_t Value;
};

union IdType
{
    Id_T     Decompose;
    uint32_t Value;
};

class ResourceId
{
  public:
    ResourceId(uint32_t id) { _data.Value = id; };
    ResourceId(uint8_t first, uint8_t second, uint16_t value)
    {
        _data.Decompose.First  = first;
        _data.Decompose.Second = second;
        _data.Decompose.Value  = value;
    };

    uint16_t Value() const { return _data.Decompose.Value; };

    uint16_t First() const { return _data.Decompose.First; };
    uint16_t Second() const { return _data.Decompose.Second; };

    uint32_t operator*() const { return _data.Value; };

  private:
    IdType _data;
};
}