#pragma once

#define DEFINE_REF(x)                                                                                                                      \
    typedef ::Seele::RefPtr<x> P##x;                                                                                                       \
    typedef ::Seele::UniquePtr<x> UP##x;                                                                                                   \
    typedef ::Seele::OwningPtr<x> O##x;

#define DECLARE_REF(x)                                                                                                                     \
    class x;                                                                                                                               \
    typedef ::Seele::RefPtr<x> P##x;                                                                                                       \
    typedef ::Seele::UniquePtr<x> UP##x;                                                                                                   \
    typedef ::Seele::OwningPtr<x> O##x;