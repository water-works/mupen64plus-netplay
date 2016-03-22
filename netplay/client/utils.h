#ifndef UTILS_H_
#define UTILS_H_

#include <chrono>

namespace client_utils {

inline int64_t now_nanos() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

}  // namespace client_utils

#endif  // UTILS_H_
