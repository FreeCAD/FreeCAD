#include <vector>

template <class _Precision> class Vector3; // In template: implicit instantiation of undefined template 'Vector3<float>'
using Vector3f = Vector3<float>;

class xInventorLoader {
public:
   xInventorLoader(std::istream &inp)
        : inp {inp} {}
   /*
    error occurred here
    in instantiation of member function 'std::vector<Vector3<float>>::capacity' requested here
    in instantiation of member function 'std::vector<Vector3<float>>::__annotate_delete' requested here
    in instantiation of member function 'std::vector<Vector3<float>>::~vector' requested here
    template is declared here
   */
   std::vector<Vector3f> vector;
   std::istream &inp;
};
