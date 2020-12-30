#ifndef SKETCHER_FILTER_OPERATION_H
#define SKETCHER_FILTER_OPERATION_H

namespace Sketcher
{
    namespace FilterChain
    {
        enum class Operation
        {
            Add,
            Remove,
            Inspect,
        };

        enum class Outcome
        {
            Continue,
            Stop,
            Restart
        };
    }
}

#endif //SKETCHER_GEOMETRY_FILTER_OPERATION_H