#ifndef MATH_HPP
#define MATH_HPP
// -*- C++ -*- Header-only

/**
 * @file include/math.hpp
 * This is the mathematic header file for Cest testing library.
 */
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace cest {
namespace math {

/**
 * @brief This algorithm calculates the distance between 2 floating points (x,
 * y)
 * @param x float X is the actual number
 * @param y float Y is the expected number
 * @returns the difference between two points
 * @note Algorithm used is ULP Difference and can be found at this url :
 * https://softwareengineering.stackexchange.com/questions/448398/how-to-handle-precision-problems-with-the-correct-terminology
 */
unsigned long ulpDifference(float x, float y) {
  assert(sizeof(float) == sizeof(uint32_t));

  int32_t ux, uy;

  std::memcpy(&ux, &x, sizeof(float));
  std::memcpy(&uy, &y, sizeof(float));

  if (ux < 0)
    ux = INT32_MIN - ux;
  if (uy < 0)
    uy = INT32_MIN - uy;

  uint32_t uUx, uUy;

  std::memcpy(&uUx, &ux, sizeof(int32_t));
  std::memcpy(&uUy, &uy, sizeof(int32_t));

  return (ux >= uy) ? (uUx - uUy) : (uUy - uUx);
}

/**
 * @brief This algorithm calculates the distance between 2 double points (x, y)
 * @param x double X is the actual number
 * @param y double Y is the expected number
 * @returns the difference between two points
 */
unsigned long long eUlpDifference(double x, double y) {
  assert(sizeof(double) == sizeof(uint64_t));

  int64_t ux, uy;

  std::memcpy(&ux, &x, sizeof(double));
  std::memcpy(&uy, &y, sizeof(double));

  if (ux < 0)
    ux = INT64_MIN - ux;
  if (uy < 0)
    uy = INT64_MIN - uy;

  uint64_t uUx, uUy;

  std::memcpy(&uUx, &ux, sizeof(int64_t));
  std::memcpy(&uUy, &uy, sizeof(int64_t));

  return (ux > uy) ? (uUx - uUy) : (uUy - uUx);
}

} // namespace math
} // namespace cest

#endif // MATH_HPP