#pragma once
#include <string>
#include "example.hpp"
#include <iostream>

namespace MyLibrary {

    /**
     * @brief some subclass
     */
    template<typename TemplatedClass>
    class SubclassExample : public Example {
    public:

        /**
         * @bug second bug
         * @return
         */
        int virtualfunc() override;

        /**
         * @brief Template function function
         */
        template <typename T>
        std::shared_ptr<std::string> function_template_test(std::shared_ptr<T>& param);

        /**
         * @brief Extra long function with lots of parameters and many template types.
         *
         * Also has a long return type.
         *
         * @param param1 first parameter
         * @param param2 second parameter
         * @param parameter3 third parameter
         */
        template <typename T, typename Foo, typename Bar, typename Alice, typename Bob, typename Charlie, typename Hello, typename World>
        std::pair<std::string, std::string> long_function_with_many_parameters(std::shared_ptr<T>& param1, std::shared_ptr<std::string>& param2, bool parameter3, Alice paramater4 Bob parameter 5) {
            if(true) {
                std::cout << "this even has some code." << std::endl;
            }
        }

    };

}

