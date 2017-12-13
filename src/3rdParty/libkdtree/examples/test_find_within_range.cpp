// Thanks to James Remillard
//
#include <cstdio>
#include <kdtree++/kdtree.hpp>
#include <vector>
#include <map>
#include <set>

using namespace std;

struct kdtreeNode
{
 typedef double value_type;

 double xyz[3];
 size_t index;

 value_type operator[](size_t n) const
 {
   return xyz[n];
 }

 double distance( const kdtreeNode &node)
 {
   double x = xyz[0] - node.xyz[0];
   double y = xyz[1] - node.xyz[1];
   double z = xyz[2] - node.xyz[2];

// this is not correct   return sqrt( x*x+y*y+z*z);

// this is what kdtree checks with find_within_range()
// the "manhattan distance" from the search point.
// effectively, distance is the maximum distance in any one dimension.
   return max(fabs(x),max(fabs(y),fabs(z)));

 }
};

int main(int argc,char *argv[])
{
 vector<kdtreeNode> pts;

 typedef KDTree::KDTree<3,kdtreeNode> treeType;

 treeType tree;

 // make random 3d points
 for ( size_t n = 0; n < 10000; ++n)
 {
   kdtreeNode node;
   node.xyz[0] = double(rand())/RAND_MAX;
   node.xyz[1] = double(rand())/RAND_MAX;
   node.xyz[2] = double(rand())/RAND_MAX;
   node.index = n;

   tree.insert( node);
   pts.push_back( node);
 }

 for (size_t r = 0; r < 1000; ++r)
 {
   kdtreeNode refNode;
   refNode.xyz[0] = double(rand())/RAND_MAX;
   refNode.xyz[1] = double(rand())/RAND_MAX;
   refNode.xyz[2] = double(rand())/RAND_MAX;

   double limit = double(rand())/RAND_MAX;

   // find the correct return list by checking every single point
   set<size_t> correctCloseList;

   for ( size_t i= 0; i < pts.size(); ++i)
   {
     double dist = refNode.distance( pts[i]);
     if ( dist < limit)
       correctCloseList.insert( i );
   }

   // now do the same with the kdtree.
   vector<kdtreeNode> howClose;
   tree.find_within_range(refNode,limit,back_insert_iterator<vector<kdtreeNode> >(howClose));

   // make sure no extra points are returned, and the return has no missing points.
   for ( size_t i = 0; i < howClose.size(); ++i)
   {
     set<size_t>::iterator hit = correctCloseList.find( howClose[i].index);

     if ( hit != correctCloseList.end())
     {
       correctCloseList.erase(hit);
     }
     else
     {
       // point that is too far away - fail!
       assert(false);
       printf("fail, extra points.\n");
     }
   }

   // fail, not all of the close enough points returned.
   assert( correctCloseList.size() == 0);
   if ( correctCloseList.size() > 0)
   {
     printf("fail, missing points.\n");
   }
 }
}

