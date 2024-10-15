
#include <vector>

#include <Type.h>

namespace Base {
struct Type::Helpers {
    static int getAllDerivedFrom(const Type& type, std::vector<Type>& List);
};
}