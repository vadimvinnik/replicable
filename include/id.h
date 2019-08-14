#pragma once

#include <utility>

namespace replicable {

namespace wrap_traits {

  template <typename T>
  struct id {
    using value_t = T;
    using ref_t = T&;
    using cref_t = T const&;
    using wrapped_t = T;

    static wrapped_t construct(value_t const& value) {
      return value;
    }

    template <typename ...Args>
    static wrapped_t construct(Args&& ...args) {
      return wrapped_t(std::forward<Args>(args)...);
    }

    static void set(wrapped_t& lhs, value_t const& rhs) {
      lhs = rhs;
    }

    template <typename ...Args>
    static void set(wrapped_t& lhs, Args&& ...args) {
      lhs = value_t(std::forward<Args>(args)...);
    }

    static ref_t get(wrapped_t& wrapped) {
      return wrapped;
    }

    static cref_t const_get(wrapped_t const& wrapped) {
      return wrapped;
    }
  };

} // namespace wrap_traits

} // namespace replicable

