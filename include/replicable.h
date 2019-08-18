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
  using source_t = source_base<T, Traits>;
  using wrap_traits_t = Traits<T>;
  using value_t = typename wrap_traits_t::value_t;
  using wrapped_t = typename wrap_traits_t::wrapped_t;
  using version_t = unsigned int;

  class replica {
      replica(
        std::lock_guard<std::mutex> lock,
        source_t const& source)
      : source_ { source }
      , wrapped_ { wrap_traits_t::construct(
          wrap_traits_t::const_get(
            source.wrapped_)) }
      , version_ { source.version_ }
      {}

  public:
      explicit replica(source_t const& source) :
        replica {
          std::lock_guard<std::mutex> { source.mutex_ },
          source
        }
      {}

      replica(replica const&) = delete;
      replica& operator=(replica const&) = delete;

      void ensure_up_to_date() {
        auto const source_version =
          source_.version_.load(std::memory_order_consume);
        if (version_ != source_version) {
          std::lock_guard<std::mutex> lock { source_.mutex_ };
          wrap_traits_t::set(
              wrapped_,
            wrap_traits_t::const_get(source_.wrapped_));
          version_ = source_version;
        }
      }

      value_t const& const_get() const {
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
    version_.fetch_add(1, std::memory_order_release);
    wrap_traits_t::set(wrapped_, value);
  }

  template <typename ...Args>
  void set(Args&& ...args) {
    std::lock_guard<std::mutex> lock { mutex_ };
    version_.fetch_add(1, std::memory_order_release);
    wrap_traits_t::set(wrapped_, std::forward<Args>(args)...);
  }

  template <typename Func>
  void modify(Func func) {
    std::lock_guard<std::mutex> lock { mutex_ };
    version_.fetch_add(1, std::memory_order_release);
    func(wrap_traits_t::get(wrapped_));
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

