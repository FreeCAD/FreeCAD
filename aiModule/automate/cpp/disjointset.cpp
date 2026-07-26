#include "disjointset.h"

namespace pspy {

DisjointSet::DisjointSet(const int size)
{
	_id = new int[size];
	for (int i = 0; i < size; ++i) {
		_id[i] = -1;
	}
}

DisjointSet::~DisjointSet()
{
	delete[] _id;
}

int DisjointSet::root(int i)
{
	while (_id[i] >= 0) {
		if (_id[_id[i]] >= 0) {
			_id[i] = _id[_id[i]];
		}
		i = _id[i];
	}
	return i;
}

void DisjointSet::unite(int p, int q)
{
	int i = root(p);
	int j = root(q);
	if (i == j) return;
	if (_id[i] > _id[j]) {
		_id[j] += _id[i];
		_id[i] = j;
	}
	else {
		_id[i] += _id[j];
		_id[j] = i;
	}
}

bool DisjointSet::find(int p, int q)
{
	return root(p) == root(q);
}

}