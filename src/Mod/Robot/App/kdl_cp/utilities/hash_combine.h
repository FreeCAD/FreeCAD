#ifndef KDL_HASH_COMBINE_H_
#define KDL_HASH_COMBINE_H_

#include <functional>

namespace KDL
{

/**
 * @brief Combine hash of object \p v to the \p seed
 * @param seed Seed to append the hash of \p v
 * @param v Object of which the hash should be appended to the seed
 *
 * Inspired by:
 * @link https://github.com/boostorg/multiprecision/blob/boost-1.79.0/include/boost/multiprecision/detail/hash.hpp#L35-L41
 */
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}

#endif
