#pragma once
#include <gsl/assert>

constexpr char deathstring[] = "Expected Death";
constexpr char failed_set_terminate_deathstring[] = ".*";

// This prevents a failed call to set_terminate from failing the test suite.
constexpr const char* GetExpectedDeathString(std::terminate_handler handle)
{
    return handle ? deathstring : failed_set_terminate_deathstring;
}
