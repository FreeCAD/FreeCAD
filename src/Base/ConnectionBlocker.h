#ifndef BASE_CONNECTIONBLOCKER_H
#define BASE_CONNECTIONBLOCKER_H

#include <boost/signals2/shared_connection_block.hpp>

class ConnectionBlocker
{
    using Connection = boost::signals2::connection;
    using ConnectionBlock = boost::signals2::shared_connection_block;
    ConnectionBlock* blocker;

public:
    ConnectionBlocker(Connection& c)
        : blocker(c)
    {}
    ~ConnectionBlocker() = default;
};
// NOLINTEND

#endif