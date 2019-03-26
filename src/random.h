//
// Created by 邢磊 on 2019/3/26.
//

#ifndef BITCOINTEST_RANDOM_H
#define BITCOINTEST_RANDOM_H


/**
 * Generate random data via the internal PRNG.
 *
 * These functions are designed to be fast (sub microsecond), but do not necessarily
 * meaningfully add entropy to the PRNG state.
 *
 * Thread-safe.
 */
void GetRandBytes(unsigned char* buf, int num) noexcept;
#endif //BITCOINTEST_RANDOM_H
