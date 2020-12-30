#ifndef SKETCHER_FILTER_H
#define SKETCHER_FILTER_H

#include "FilterOperation.h"

namespace Sketcher
{
    namespace FilterChain
    {
        template<typename INPUT, typename OUTPUT> class Filter;
        template<typename OUTPUT>
        class FilterOutput
        {
            public:
                const Outcome outcome;
                const OUTPUT output;
                FilterOutput(Outcome outcome): outcome(outcome), output(0) {}
                FilterOutput(Outcome outcome, const OUTPUT output): outcome(outcome), output(output) {}
                FilterOutput(const FilterOutput & outcome) = default;
                static const FilterOutput defaultContinue() 
                {
                    return FilterOutput(Outcome::Continue);
                }
        };
        template<>
        class FilterOutput<void>
         {
            public:
                const Outcome outcome;
                FilterOutput(Outcome outcome):outcome(outcome){}
                FilterOutput(const FilterOutput & outcome) = default;
                static const FilterOutput defaultContinue() 
                {
                    return FilterOutput(Outcome::Continue);
                }
        };


        template<typename DATA, typename OUTPUT>
        class FilterInput
        {
            public:
                const DATA& data;
                Filter<DATA,OUTPUT> * const originator;

                FilterInput(const DATA & data, Filter<DATA,OUTPUT> * originator):
                data(data), originator(originator){}

                FilterInput<DATA,OUTPUT> withOriginator(Filter<DATA,OUTPUT> * const newOriginator) const
                {
                    return FilterInput(data, newOriginator);
                }

                FilterInput<DATA,OUTPUT> withData(const DATA& data) const
                {
                    return FilterInput(data, originator);
                }
        };

        template<typename DATA, typename OUTPUT>
        class Filter
        {
        public:
            using InputType = FilterInput<DATA,OUTPUT> ;
            using OutputType = FilterOutput<OUTPUT>;
            Filter() = default;
            Filter(Filter const&f) = default;
            virtual OutputType execute(const InputType & )=0;
        };
    }
}

#endif //SKETCHER_FILTER_H