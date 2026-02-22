// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

#ifndef BASE_RANDOMUTILS_H
#define BASE_RANDOMUTILS_H

#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>

namespace Base
{
/**
 * @brief Generate a random suffix string for creating unique directory names
 * @param prefix Optional prefix for the suffix (default: "fctest")
 * @param includeThreadId Whether to include the thread ID in the suffix (default: true)
 * @return A unique random suffix string suitable for parallel test execution
 */
inline std::string generateRandomName(const std::string& prefix, bool includeThreadId = true)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999); 
    std::ostringstream oss;
    oss << prefix;
    if (includeThreadId) {
        oss << "_" << std::this_thread::get_id();
    }
    oss << "_" << std::setfill('0') << std::setw(6) << dis(gen);
    return oss.str();
}
} // namespace Base

#endif // BASE_RANDOMUTILS_H
