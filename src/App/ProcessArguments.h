// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace App
{

/** An owned argument list with deep-copy semantics.
 *
 * Copies preserve the active argument count and order, including changes made
 * through argc()/argv(). Keep this object alive and do not assign or move it
 * while a consumer such as QApplication retains references to its buffers.
 */
class ProcessArguments
{
public:
    ProcessArguments()
        : ProcessArguments(std::vector<std::string> {})
    {}

    explicit ProcessArguments(std::vector<std::string> values)
        : arguments(std::move(values))
    {
        pointers.reserve(arguments.size() + 1);
        for (auto& argument : arguments) {
            pointers.push_back(argument.data());
        }
        pointers.push_back(nullptr);
        argumentCount = static_cast<int>(arguments.size());
    }

    ProcessArguments(const ProcessArguments& other)
        : ProcessArguments(std::vector<std::string>(
              other.pointers.begin(),
              other.pointers.begin() + other.count()
          ))
    {}

    ProcessArguments(ProcessArguments&& other)
        : ProcessArguments()
    {
        swap(other);
    }

    ProcessArguments& operator=(ProcessArguments other)
    {
        swap(other);
        return *this;
    }

    [[nodiscard]] int count() const
    {
        return argumentCount;
    }

    /** Read-only access in the current argument order. */
    [[nodiscard]] std::string_view operator[](std::size_t index) const
    {
        return pointers[index];
    }

    /** Mutable C API access; argc and argv must be used together. */
    [[nodiscard]] int& argc()
    {
        return argumentCount;
    }

    [[nodiscard]] char** argv()
    {
        return pointers.data();
    }

private:
    void swap(ProcessArguments& other) noexcept
    {
        arguments.swap(other.arguments);
        pointers.swap(other.pointers);
        std::swap(argumentCount, other.argumentCount);
    }

    std::vector<std::string> arguments;
    std::vector<char*> pointers;
    int argumentCount = 0;
};

}  // namespace App
