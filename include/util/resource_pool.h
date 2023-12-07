#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace util {

#define DEFAULT_POOL_SIZE 64

template <typename T>
class resource_pool : public std::enable_shared_from_this<resource_pool<T>> {
public:
  using ptr = std::shared_ptr<resource_pool>;

  static ptr create(size_t pool_size = DEFAULT_POOL_SIZE) {
    pool_size = pool_size ? pool_size : DEFAULT_POOL_SIZE;
    return std::shared_ptr<resource_pool<T>>(new resource_pool<T>(pool_size));
  }

  ~resource_pool() {
    for (auto ptr : objs_) {
      if (ptr) {
        delete ptr;
        ptr = nullptr;
      }
    }
  }

  std::shared_ptr<T> obtain() {
    std::weak_ptr<resource_pool<T>> weak_self = this->shared_from_this();
    return std::shared_ptr<T>(get_obj_ptr(), [weak_self](T *ptr) {
      if (auto strong_self = weak_self.lock()) {
        strong_self->recycle(ptr);
      } else {
        if (ptr) {
          delete ptr;
          ptr = nullptr;
        }
      }
    });
  }

private:
  resource_pool(size_t pool_size) : objs_(pool_size) {
    alloc_ = []() -> T * { return new T(); };
  }

  T *get_obj_ptr() {
    T *ptr = nullptr;
    auto is_busy = busy_.test_and_set();
    // if not busy, then obtain a pointer
    if (!is_busy) {
      if (objs_.size() == 0) {
        ptr = alloc_();
      } else {
        ptr = objs_.back();
        objs_.pop_back();
      }

      busy_.clear();
    } else {
      ptr = alloc_();
    }
    return ptr;
  }

  void recycle(T *ptr) {
    auto is_busy = busy_.test_and_set();
    if (!is_busy) {
      if (objs_.size() >= pool_size_) {
        if (ptr) {
          delete ptr;
          ptr = nullptr;
        }
      } else {
        objs_.emplace_back(ptr);
      }

      busy_.clear();
    } else {
      if (ptr) {
        delete ptr;
        ptr = nullptr;
      }
    }
  }

private:
  size_t pool_size_{0};
  std::vector<T *> objs_;
  std::function<T *(void)> alloc_;
  std::atomic_flag busy_{false};
};

} // namespace util