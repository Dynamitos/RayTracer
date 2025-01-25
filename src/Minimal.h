#pragma once
#include <coroutine>
#include <memory>

#define DEFINE_REF(x) typedef ::std::unique_ptr<x> P##x;

#define DECLARE_REF(x)                                                                                                                     \
  class x;                                                                                                                                 \
  typedef ::std::unique_ptr<x> P##x;

struct Task
{
  struct promise_type
  {
    Task get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
  std::coroutine_handle<promise_type> handle;
};
