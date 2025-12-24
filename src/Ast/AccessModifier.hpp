#pragma once

#include <cstdint>
namespace Fig
{
    enum class AccessModifier : uint8_t
    {
        Normal,
        Const,
        Public,
        PublicConst,
    };

    inline bool isAccessPublic(AccessModifier am)
    {
        return am == AccessModifier::Public || am == AccessModifier::PublicConst;
    }

    inline bool isAccessConst(AccessModifier am)
    {
        return am == AccessModifier::Const || am == AccessModifier::PublicConst;
    }
};