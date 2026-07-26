#ifndef LSH_INCLUDED
#define LSH_INCLUDED 1

#include <Eigen/Core>
#include <vector>
#include <unordered_map>
#include <set>

namespace pspy {
class LSH {
public:
	LSH(const std::vector<Eigen::VectorXd>& points, int nDimensions, double tolerance);
	// Note, sets can't seem to be passed in as a reference via pybind
	std::set<unsigned int> get_nearest_points(const Eigen::VectorXd& point) const;
private:
	typedef std::unordered_map<long, std::set<unsigned int>> IndexHashMap;
	std::vector<IndexHashMap> hashMap_;
	std::vector<Eigen::VectorXd> points_;
	double tolerance_;
	int nDimensions_;
};

}

#endif // !LSH_INCLUDED