#ifndef DISJOINTSET_INCLUDED
#define DISJOINTSET_INCLUDED 1

namespace pspy {

struct DisjointSet
{
public:
	DisjointSet(const int size);
	~DisjointSet();
	int root(int i);
	void unite(int p, int q);
	bool find(int p, int q);

private:
	int* _id;
};

}

#endif // !DISJOINTSET_INCLUDED