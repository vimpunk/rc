#ifndef RC_HPP
#define RC_HPP

#include <utility>

namespace mnd {
namespace detail {

template<class T>
struct shared_region {
    int use_count = 1;
    int weak_count = 0;
    T object;

    template<class... Args>
    shared_region(Args&&... args) : object(std::forward<Args>(args)...) {}
};

template<class T, class... Rest>
struct first_arg {
    using type = T;
};

template<class T, class... Rest>
struct num_args {
    static const int value = 1 + num_args<Rest...>::value;
};

template<class T>
struct num_args<T> {
    static const int value = 1;
};

} // detail

template<class T>
class weak_rc;

template<class T>
class rc {
    detail::shared_region<T>* region_ = nullptr;

public:
    rc() = default;
    rc(const rc& that) noexcept { assign(that); }
    rc(rc&& that) noexcept { move_assign(that); }
    ~rc() noexcept { reset(); }

    template<
        class... Args,
        // FIXME: this is a bit hacky but we need to isolate non-rc types
        // otherwise the copy and other constructors aren't going to be invoked
        // due to perfect forwarding having a higher precedence. However, this
        // is wrong because it will effectively disable types whose constructor
        // takes an rc of the same type as its first argument (however unlikely
        // the case may be).
        class = typename std::enable_if<
            (not std::is_same<
                typename std::decay<
                    typename detail::first_arg<Args...>::type
                >::type,
                rc
            >::value and not std::is_same<
                typename std::decay<
                    typename detail::first_arg<Args...>::type
                >::type,
                weak_rc<T>
            >::value)
            or (detail::num_args<Args...>::value > 1)
        >::type
    > rc(Args&&... args) : region_(new detail::shared_region<T>(std::forward<Args>(args)...)) {}

    rc(const weak_rc<T>& w) {
        if(!w.expired()) {
            region_ = w.region_;
            if(region_) {
                ++region_->use_count;
            }
        }
    }

    rc(void*& vptr) noexcept : region_(static_cast<detail::shared_region<T>*>(vptr)) {
        // We don't need to increment the use count because when casting an `rc`
        // to a void pointer, the use count is incremented, and here we are
        // consuming the void pointer, keeping the use count unchanged.
        vptr = nullptr;
    }

    rc(void*&& vptr) noexcept : region_(static_cast<detail::shared_region<T>*>(vptr)) {}

    rc& operator=(const rc& that) noexcept {
        if(this != &that) {
            reset();
            assign(that);
        }
        return *this;
    }

    rc& operator=(rc&& that) noexcept {
        if(this != &that) {
            reset();
            move_assign(that);
        }
        return *this;
    }

    weak_rc<T> weak() noexcept { return {*this}; }

    operator void*() const noexcept {
        if(region_) {
            ++region_->use_count;
            return region_;
        } else {
            return nullptr;
        }
    }

    bool expired() const noexcept { return region_ == nullptr; }
    operator bool() const noexcept { return !expired(); }

    int use_count() const noexcept {
        if(region_) {
            return region_->use_count;
        } else {
            return 0;
        }
    }

    int weak_count() const noexcept {
        if(region_) {
            return region_->weak_count;
        } else {
            return 0;
        }
    }

    T& operator*() const noexcept {
        return region_->object;
    }

    T* operator->() const noexcept {
        return &region_->object;
    }

    void reset() noexcept {
        if(region_) {
            --region_->use_count;
            // If this was the very last reference to the shared region, we must
            // deallocate it.
            if(region_->use_count <= 0 && region_->weak_count <= 0) {
                delete region_;
            }
            region_ = nullptr;
        }
    }

private:
    void assign(const rc& that) noexcept {
        region_ = that.region_;
        if(region_) {
            ++region_->use_count;
        }
    }

    void move_assign(rc& that) noexcept {
        region_ = that.region_;
        that.region_ = nullptr;
    }

    template<class R>
    friend class weak_rc;
};

template<class T>
class weak_rc {
    detail::shared_region<T>* region_ = nullptr;

public:
    weak_rc() = default;
    weak_rc(const weak_rc& that) noexcept { assign(that); }
    weak_rc(weak_rc&& that) noexcept { move_assign(that); }
    ~weak_rc() noexcept { reset(); }

    weak_rc(void*& vptr) noexcept {
        region_ = static_cast<detail::shared_region<T>*>(vptr);
        vptr = nullptr;
    }

    weak_rc(void*&& vptr) noexcept {
        region_ = static_cast<detail::shared_region<T>*>(vptr);
    }

    weak_rc(const rc<T>& rc) noexcept {
        if(rc) {
            region_ = rc.region_;
            if(region_) {
                ++region_->weak_count;
            }
        }
    }

    weak_rc& operator=(const weak_rc& that) noexcept {
        if(this != &that) {
            reset();
            assign(that);
        }
        return *this;
    }

    weak_rc& operator=(weak_rc&& that) noexcept {
        if(this != &that) {
            reset();
            move_assign(that);
        }
        return *this;
    }

    //operator void*() const noexcept { }

    bool expired() const noexcept { return use_count() <= 0; }
    operator bool() const noexcept { return !expired(); }

    operator void*() const noexcept {
        if(region_) {
            ++region_->weak_count;
            return region_;
        } else {
            return nullptr;
        }
    }

    int use_count() const noexcept {
        if(region_) {
            return region_->use_count;
        } else {
            return 0;
        }
    }

    int weak_count() const noexcept {
        if(region_) {
            return region_->weak_count;
        } else {
            return 0;
        }
    }

    rc<T> lock() noexcept {
        rc<T> rc;
        if(!expired()) {
            rc.region_ = region_;
            if(region_) {
                ++region_->use_count;
            }
        }
        return rc;
    }

    void reset() noexcept {
        if(region_) {
            --region_->weak_count;
            // If this was the very last reference to the shared region, we must
            // deallocate it.
            if(region_->weak_count <= 0 && region_->use_count <= 0) {
                delete region_;
            }
            region_ = nullptr;
        }
    }

private:
    void assign(const weak_rc& that) noexcept {
        region_ = that.region_;
        if(region_) {
            ++region_->weak_count;
        }
    }

    void move_assign(weak_rc& that) noexcept {
        region_ = that.region_;
        that.region_ = nullptr;
    }

    template<class R>
    friend class rc;
};

static_assert(sizeof(rc<int>) == sizeof(void*), "rc is not the same size as void*");

} // mnd

#endif
