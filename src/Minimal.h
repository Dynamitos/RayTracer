#pragma once

#define DEFINE_REF(x) typedef ::std::unique_ptr<x> P##x;

#define DECLARE_REF(x)                                                                                                                     \
  class x;                                                                                                                                 \
  typedef ::std::unique_ptr<x> P##x;

