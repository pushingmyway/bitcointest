//
// Created by 邢磊 on 2019/3/26.
//

#ifndef BITCOINTEST_MEMORY_H
#define BITCOINTEST_MEMORY_H

#include <memory>

template <typename T, typename... Args>
std::unique_ptr<T> MakeUnique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif //BITCOINTEST_MEMORY_H
