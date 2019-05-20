#pragma once

#include <memory>
#include <mutex>
#include <atomic>

namespace wrap_traits {
    template <typename T>
    struct id
    {
        using value_t = T;
        using ref_t = T&;
        using cref_t = T const&;
        using wrapped_t = T;

        static wrapped_t construct(value_t const& value)
        {
            return value;
        }

        template <typename ...Args>
        static wrapped_t construct(Args&& ...args)
        {
            return wrapped_t(std::forward<Args>(args)...);
        }

        static void set(wrapped_t& lhs, value_t const& rhs)
        {
            lhs = rhs;
        }

        template <typename ...Args>
        static void set(wrapped_t& lhs, Args&& ...args)
        {
            lhs = value_t(std::forward<Args>(args)...);
        }

        static ref_t get(wrapped_t& wrapped)
        {
            return wrapped;
        }

        static cref_t const_get(wrapped_t const& wrapped)
        {
            return wrapped;
        }
    };

    template <typename T>
    struct ptr
    {
        using value_t = T;
        using ref_t = T&;
        using cref_t = T const&;
        using wrapped_t = std::unique_ptr<T>;

        static wrapped_t construct(value_t const& value)
        {
            return std::make_unique<value_t>(value);
        }

        template <typename ...Args>
        static wrapped_t construct(Args&& ...args)
        {
            return std::make_unique<value_t>(std::forward<Args>(args)...);
        }

        static void set(wrapped_t& lhs, value_t const& rhs)
        {
            lhs = std::make_unique<value_t>(rhs);
        }

        template <typename ...Args>
        static void set(wrapped_t& lhs, Args&& ...args)
        {
            lhs = std::make_unique<value_t>(std::forward<Args>(args)...);
        }

        static ref_t get(wrapped_t& wrapped)
        {
            return *wrapped;
        }

        static cref_t const_get(wrapped_t const& wrapped)
        {
            return *wrapped;
        }
    };

    template <typename WrapTraits>
    typename WrapTraits::wrapped_t copy(typename WrapTraits::wrapped_t const& other)
    {
        return WrapTraits::construct(WrapTraits::const_get(other));
    }

    template <typename WrapTraits>
    void assign(typename WrapTraits::wrapped_t& lhs, typename WrapTraits::wrapped_t const& rhs)
    {
        WrapTraits::set(lhs, WrapTraits::const_get(rhs));
    }
}

template <typename T, template<typename> typename Traits = wrap_traits::id>
class replicable_base
{
public:
    using wrap_traits_t = Traits<T>;
    using replicable_t = replicable_base<T, Traits>;
    using wrapped_t = typename wrap_traits_t::wrapped_t;
    using cref_t = typename wrap_traits_t::cref_t;
    using version_t = unsigned long;

private:
    class replicable_lock
    {
    public:
        explicit replicable_lock(replicable_t const& cref)
            : cref(cref)
            , lock(cref.mutex_)
        {}

        replicable_t const& cref;

    private:
        std::lock_guard<std::mutex> lock;
    };

public:
    using value_t = typename wrap_traits_t::value_t;

    class replica
    {
        explicit replica(replicable_lock const& lock)
            : source_(lock.cref)
            , wrapped_(wrap_traits::copy<wrap_traits_t>(lock.cref.wrapped_))
            , version_(lock.cref.version_)
        {}

    public:
        explicit replica(replicable_t const& source) :
            replica(replicable_lock(source))
        {}

        replica(replica const&) = delete;
        replica& operator=(replica const&) = delete;

        version_t version() const noexcept
        {
            return version_;
        }

        bool is_up_to_date() const noexcept
        {
            return version_ == source_.version_;
        }

        version_t ensure_up_to_date()
        {
            if (version_ != source_.version_)
            {
                replicable_lock lock(source_);
                wrap_traits::assign<wrap_traits_t>(wrapped_, lock.cref.wrapped_);
                version_ = lock.cref.version_;
            }

            return version_;
        }

        cref_t const_get() const
        {
            return wrap_traits_t::const_get(wrapped_);
        }

    private:
        replicable_t const& source_;
        wrapped_t wrapped_;
        version_t version_;
    };

    template <typename ...Args>
    explicit replicable_base(Args&& ...args)
        : wrapped_(wrap_traits_t::construct(std::forward<Args>(args)...))
        , version_(0)
    {}

    replicable_base(replicable_t const&) = delete;
    replicable_t& operator=(replicable_t const&) = delete;

    version_t version() const noexcept
    {
        return version_;
    }

    void set(value_t const& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++version_;
        wrap_traits_t::set(wrapped_, value);
    }

    template <typename Func>
    void modify(Func func)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++version_;
        func(wrap_traits_t::get(wrapped_));
    }

    template <typename ...Args>
    void set(Args&& ...args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++version_;
        wrap_traits_t::set(wrapped_, std::forward<Args>(args)...);
    }

    void replace(wrapped_t&& wrapped)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++version_;
        wrapped_ = wrapped;
    }

private:
    mutable std::mutex mutex_;
    wrapped_t wrapped_;
    std::atomic<version_t> version_;
};

template <typename T>
using replicable_assigning = replicable_base<T, wrap_traits::id>;

template <typename T>
using replicable_replacing = replicable_base<T, wrap_traits::ptr>;
