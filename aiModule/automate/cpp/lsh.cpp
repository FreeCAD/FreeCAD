#include "lsh.h"
#include <cassert>


namespace pspy {

long primes[16] = { 
	73856093, 
	19349669, 
	83492791, 
	49979539, 
	86028157, 
	15485867, 
	32452843, 
	21653669, 
	45647201, 
	97623107, 
	37605409, 
	76179877, 
	53563061, 
	53303309, 
	38523587, 
	57352609
};
long computeHashKey(const Eigen::VectorXd& point, int gridIndex, double tolerance) {
	assert(point.size() <= 16); // Maximum number of dimensions
	long value = 0;
	for (int i = 0; i < point.size(); ++i) {
		long gridPoint = floor((point(i) + gridIndex * tolerance) / ((point.size() + 1) * tolerance));
		value ^= (primes[i] * gridPoint);
	}
	return value;
}

LSH::LSH(const std::vector<Eigen::VectorXd>& points, int nDimensions, double tolerance) {
	assert(nDimensions <= 16); // Maximum number of dimensions
	tolerance_ = tolerance;
	points_ = points;
	nDimensions_ = nDimensions;
	for (int i = 0; i < (nDimensions_ + 1); ++i) {
		IndexHashMap iHashMap;
		hashMap_.push_back(iHashMap);
		for (size_t index = 0; index < points.size(); ++index) {
			assert(points[index].size() == nDimensions_);
			long hashValue = computeHashKey(points[index], i, tolerance_);
			auto mapElement = hashMap_[i].find(hashValue);
			if (mapElement != hashMap_[i].end()) {
				(mapElement)->second.insert(index);
			}
			else {
				hashMap_[i][hashValue] = std::set<unsigned int>{ (unsigned int)index };
			}
		}
	}
}
std::set<unsigned int> LSH::get_nearest_points(const Eigen::VectorXd& point) const {
	std::set<unsigned int> nearestPoints{};

	assert(point.size() == nDimensions_);
	for (int i = 0; i < (nDimensions_ + 1); ++i) {
		long hashValue = computeHashKey(point, i, tolerance_);
		auto mapElement = hashMap_[i].find(hashValue);
		if (mapElement != hashMap_[i].end())
		{
			for (unsigned int index : mapElement->second) {
				if ((points_[index] - point).norm() <= tolerance_) {
					nearestPoints.insert(index);
				}
			}
		}
	}
	return nearestPoints;
}

}