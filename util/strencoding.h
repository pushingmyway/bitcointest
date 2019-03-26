//
// Created by 邢磊 on 2019/3/26.
//

#ifndef BITCOINTEST_STRENCODING_H
#define BITCOINTEST_STRENCODING_H

template<typename T>
std::string HexStr(const T itbegin, const T itend)
{
  std::string rv;
  static const char hexmap[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  rv.reserve(std::distance(itbegin, itend) * 2);
  for(T it = itbegin; it < itend; ++it)
  {
    unsigned char val = (unsigned char)(*it);
    rv.push_back(hexmap[val>>4]);
    rv.push_back(hexmap[val&15]);
  }
  return rv;
}

#endif //BITCOINTEST_STRENCODING_H
