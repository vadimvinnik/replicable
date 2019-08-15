#pragma once

#include "id.h"
#include "unique_ptr.h"

#include <memory>
#include <mutex>
#include <atomic>

namespace replicable {

template <typename T, template<typename> typename Traits = wrap_traits::id>
class source_base {
public:
  using wrap_traits_t = Traits<T>;
  using source_t = source_base<T, Traits>;
  using wrapped_t = typename wrap_traits_t::wrapped_t;
  using cref_t = typename wrap_traits_t::cref_t;
  using version_t = unsigned long;

private:
  static wrapped_t copy(wrapped_t const& other) {
    return wrap_traits_t::construct(wrap_traits_t::const_get(other));
  }

  static void assign(wrapped_t& lhs, wrapped_t const& rhs) {
    wrap_traits_t::set(lhs, wrap_traits_t::const_get(rhs));
  }

  class locked_source {
  public:
      explicit locked_source(source_t const& source)
        : source { source }
        , lock_ { source.mutex_ }
      {}

      source_t const& source;

  private:
      std::lock_guard<std::mutex> lock_;
  };

public:
  using value_t = typename wrap_traits_t::value_t;

  class replica {
      explicit replica(locked_source const& lock)
        : source_ { lock.source }
        , wrapped_ { copy(lock.source.wrapped_) }
        , version_ { lock.source.version_ }
      {}

  public:
      explicit replica(source_t const& source) :
        replica { locked_source(source) }
      {}

      replica(replica const&) = delete;
      replica& operator=(replica const&) = delete;

      version_t version() const noexcept {
        return version_;
      }

      bool is_up_to_date() const noexcept {
        return version_ == source_.version_;
      }

      version_t ensure_up_to_date() {
        if (version_ != source_.version_) {
          locked_source lock { source_ };
          assign(wrapped_, lock.source.wrapped_);
          version_ = lock.source.version_;
        }

        return version_;
      }

      cref_t const_get() const
      {
        return wrap_traits_t::const_get(wrapped_);
      }

  private:
      source_t const& source_;
      wrapped_t wrapped_;
      version_t version_;
  };

  template <typename ...Args>
  explicit source_base(Args&& ...args)
    : wrapped_ { wrap_traits_t::construct(std::forward<Args>(args)...) }
    , version_ { 0 }
  {}

  source_base(source_t const&) = delete;
  source_t& operator=(source_t const&) = delete;

  version_t version() const noexcept {
    return version_;
  }

  void set(value_t const& value) {
    std::lock_guard<std::mutex> lock { mutex_ };
    ++version_;
    wrap_traits_t::set(wrapped_, value);
  }

  template <typename Func>
  void modify(Func func) {
    std::lock_guard<std::mutex> lock { mutex_ };
    ++version_;
    func(wrap_traits_t::get(wrapped_));
  }

  template <typename ...Args>
  void set(Args&& ...args) {
    std::lock_guard<std::mutex> lock { mutex_ };
    ++version_;
    wrap_traits_t::set(wrapped_, std::forward<Args>(args)...);
  }

  void replace(wrapped_t&& wrapped) {
    std::lock_guard<std::mutex> lock { mutex_ };
    ++version_;
    wrapped_ = wrapped;
  }

private:
  mutable std::mutex mutex_;
  wrapped_t wrapped_;
  std::atomic<version_t> version_;
};

template <typename T>
using source_assigning = source_base<T, wrap_traits::id>;

template <typename T>
using source_replacing = source_base<T, wrap_traits::unique_ptr>;

} // namespace replicable

