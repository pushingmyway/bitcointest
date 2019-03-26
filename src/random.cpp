//
// Created by 邢磊 on 2019/3/26.
//
#include "random.h"


void GetRandBytes(unsigned char *buf, int num) noexcept {

//  ProcRand(buf, num, RNGLevel::FAST);

  for (auto i = 0; i < num; i++) {
    buf[i] = 1;
  }
}

