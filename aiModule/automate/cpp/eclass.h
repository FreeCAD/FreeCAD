#ifndef ECLASS_INCLUDED
#define ECLASS_INCLUDED 1

#include <vector>
#include <Eigen/Core>

namespace pspy {

std::vector<int> find_equivalence_classes(const std::vector<Eigen::VectorXd>& points, double tolerance);

}

#endif // !ECLASS_INCLUDED
