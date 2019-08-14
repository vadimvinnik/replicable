#pragma once

#include <memory>
#include <utility>

namespace replicable {

namespace wrap_traits {

  template <typename T>
  struct unique_ptr {
    using value_t = T;
    using ref_t = T&;
    using cref_t = T const&;
    using wrapped_t = std::unique_ptr<T>;

    static wrapped_t construct(value_t const& value) {
      return std::make_unique<value_t>(value);
    }

    template <typename ...Args>
    static wrapped_t construct(Args&& ...args) {
      return std::make_unique<value_t>(std::forward<Args>(args)...);
    }

    static void set(wrapped_t& lhs, value_t const& rhs) {
      lhs = std::make_unique<value_t>(rhs);
    }

    template <typename ...Args>
    static void set(wrapped_t& lhs, Args&& ...args) {
      lhs = std::make_unique<value_t>(std::forward<Args>(args)...);
    }

    static ref_t get(wrapped_t& wrapped) {
      return *wrapped;
    }

    static cref_t const_get(wrapped_t const& wrapped) {
      return *wrapped;
    }
  };

} // namespace wrap_traits

} // namespace replicable

