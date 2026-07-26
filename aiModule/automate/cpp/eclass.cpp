#include "eclass.h"

#include "lsh.h"
#include "disjointset.h"
#include <cassert>

namespace pspy {

std::vector<int> find_equivalence_classes(const std::vector<Eigen::VectorXd>& points, double tolerance)
{
	if (points.size() == 0) {
		return std::vector<int>();
	}
	int n = points[0].size();

	auto lsh = LSH(points, n, tolerance);
	auto ds = DisjointSet(points.size());

	for (int i = 0; i < points.size(); ++i) {
		assert(points[i].size() == n);
		for (int j : lsh.get_nearest_points(points[i])) {
			if (i != j) {
				ds.unite(i, j);
			}
		}
	}

	std::vector<int> eclasses(points.size());
	for (int i = 0; i < points.size(); ++i) {
		eclasses[i] = ds.root(i);
	}

	return eclasses;
}

}