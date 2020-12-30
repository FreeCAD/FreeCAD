#ifndef SKETCHER_FILTER_CHAIN_H
#define SKETCHER_FILTER_CHAIN_H
#include <vector>
#include <memory>
#include "Filter.h"

namespace Sketcher
{
    namespace FilterChain
    {
        template<typename DATA, typename OUTPUT>
        class FilterChain : public Filter<DATA,OUTPUT>
        {
        public:
            using FilterType = Filter<DATA,OUTPUT> ;

            std::vector<std::unique_ptr<FilterType > > filters;
            virtual FilterOutput<OUTPUT> execute(const typename FilterType::InputType& input)
            {
                auto request = input.withOriginator(this);
                for(const auto &filter: filters){
                    auto response = filter->execute(request);
                    if(response.outcome == Outcome::Stop)
                        return response;
                }
                return FilterType::OutputType::defaultContinue();
            }


        };
    }
}

#endif //SKETCHER_FILTER_CHAIN_H