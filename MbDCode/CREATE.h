#pragma once
#include <memory>

namespace MbD {

    template<typename T>
    class CREATE {
    public:
        static std::shared_ptr<T> With(const char* name) {
            auto inst = std::make_shared<T>(name);
            inst->initialize();
            return inst;
        }
        static std::shared_ptr<T> With() {
            auto inst = std::make_shared<T>();
            inst->initialize();
            return inst;
        }
    };
}

