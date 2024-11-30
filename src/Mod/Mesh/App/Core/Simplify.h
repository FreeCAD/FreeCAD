// clang-format off
// http://voxels.blogspot.de/2014/05/quadric-mesh-simplification-with-source.html
// https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification
//
// MIT License

// Changes:
// * Use Base::Vector3f as vec3f class
// * Move global variables to a class to make the algorithm usable for multi-threading
// * Comment out printf statements
// * Fix compiler warnings
// * Remove macros loop,i,j,k

#include <vector>


#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

using vec3f = Base::Vector3f;

class SymmetricMatrix {

public:

    // Constructor

    SymmetricMatrix(double c=0) { for (std::size_t i=0;i<10;++i ) m[i] = c; }

    SymmetricMatrix(double m11, double m12, double m13, double m14,
                               double m22, double m23, double m24,
                                           double m33, double m34,
                                                       double m44) {
            m[0] = m11;  m[1] = m12;  m[2] = m13;  m[3] = m14;
                         m[4] = m22;  m[5] = m23;  m[6] = m24;
                                      m[7] = m33;  m[8] = m34;
                                                   m[9] = m44;
    }

    // Make plane

    SymmetricMatrix(double a,double b,double c,double d)
    {
        m[0] = a*a;  m[1] = a*b;  m[2] = a*c;  m[3] = a*d;
                     m[4] = b*b;  m[5] = b*c;  m[6] = b*d;
                                  m[7 ] =c*c;  m[8 ] = c*d;
                                               m[9 ] = d*d;
    }

    double operator[](int c) const { return m[c]; }

    // Determinant

    double det(int a11, int a12, int a13,
               int a21, int a22, int a23,
               int a31, int a32, int a33)
    {
        double det =  m[a11]*m[a22]*m[a33] + m[a13]*m[a21]*m[a32] + m[a12]*m[a23]*m[a31]
                    - m[a13]*m[a22]*m[a31] - m[a11]*m[a23]*m[a32] - m[a12]*m[a21]*m[a33];
        return det;
    }

    const SymmetricMatrix operator+(const SymmetricMatrix& n) const
    {
        return SymmetricMatrix( m[0]+n[0],    m[1]+n[1],   m[2]+n[2],   m[3]+n[3],
                                              m[4]+n[4],   m[5]+n[5],   m[6]+n[6],
                                                           m[7]+n[7],   m[8]+n[8],
                                                                        m[9]+n[9]);
    }

    SymmetricMatrix& operator+=(const SymmetricMatrix& n)
    {
        m[0]+=n[0];   m[1]+=n[1];   m[2]+=n[2];   m[3]+=n[3];
        m[4]+=n[4];   m[5]+=n[5];   m[6]+=n[6];   m[7]+=n[7];
        m[8]+=n[8];   m[9]+=n[9];
        return *this;
    }

    double m[10];
};
///////////////////////////////////////////

class Simplify
{
public:
    struct Triangle { int v[3];double err[4];int deleted,dirty;vec3f n; };
    struct Vertex { vec3f p;int tstart,tcount;SymmetricMatrix q;int border;};
    struct Ref { int tid,tvertex; };
    std::vector<Triangle> triangles;
    std::vector<Vertex> vertices;
    std::vector<Ref> refs;

    void simplify_mesh(int target_count, double tolerance, double aggressiveness=7);

private:
    // Helper functions

    double vertex_error(const SymmetricMatrix& q, double x, double y, double z);
    double calculate_error(int id_v1, int id_v2, vec3f &p_result);
    bool flipped(vec3f p,int i0,int i1,Vertex &v0,Vertex &v1,std::vector<int> &deleted);
    void update_triangles(int i0,Vertex &v,std::vector<int> &deleted,int &deleted_triangles);
    void update_mesh(int iteration);
    void compact_mesh();
};

//
// Main simplification function
//
// target_count   : target nr. of triangles
// tolerance      : tolerance for the quadratic errors
// aggressiveness : sharpness to increase the threshold.
//                  5..8 are good numbers
//                  more iterations yield higher quality
// If the passed tolerance is > 0 then this will be used to check
// the quadratic error metric of all triangles. If none of them is below
// the tolerance the algorithm will stop at this point. The number of the
// remaining triangles usually will be higher than \a target_count
//
void Simplify::simplify_mesh(int target_count, double tolerance, double aggressiveness)
{
    // init
    //printf("%s - start\n",__FUNCTION__);
    //int timeStart=timeGetTime();

    for (std::size_t i=0;i<triangles.size();++i)
        triangles[i].deleted=0;

    // main iteration loop

    int deleted_triangles=0;
    std::vector<int> deleted0,deleted1;
    int triangle_count=triangles.size();

    for (int iteration=0;iteration<100;++iteration)
    {
        // target number of triangles reached ? Then break
        //printf("iteration %d - triangles %d\n",iteration,triangle_count-deleted_triangles);
        if (triangle_count-deleted_triangles<=target_count)
            break;

        // update mesh once in a while
        if (iteration%5==0)
        {
            update_mesh(iteration);
        }

        // clear dirty flag
        for (std::size_t i=0;i<triangles.size();++i)
            triangles[i].dirty=0;

        //
        // All triangles with edges below the threshold will be removed
        //
        // The following numbers works well for most models.
        // If it does not, try to adjust the 3 parameters
        //
        double threshold = 0.000000001*pow(double(iteration+3),aggressiveness);
        if (tolerance > 0.0)
        {
            bool canContinue = false;
            for (std::size_t i=0;i<triangles.size();++i)
            {
                Triangle &t=triangles[i];
                if (t.deleted)
                    continue;
                if (t.dirty)
                    continue;
                if (fabs(t.err[3])<tolerance)
                {
                    canContinue = true;
                    break;
                }
            }

            if (!canContinue)
                break;
        }

        // remove vertices & mark deleted triangles
        for (std::size_t i=0;i<triangles.size();++i)
        {
            Triangle &t=triangles[i];
            if (t.err[3]>threshold)
                continue;
            if (t.deleted)
                continue;
            if (t.dirty)
                continue;

            for (std::size_t j=0;j<3;++j)
            {
                if (t.err[j]<threshold)
                {
                    int i0=t.v[ j     ]; Vertex &v0 = vertices[i0];
                    int i1=t.v[(j+1)%3]; Vertex &v1 = vertices[i1];

                    // Border check
                    if (v0.border != v1.border)
                        continue;

                    // Compute vertex to collapse to
                    vec3f p;
                    calculate_error(i0,i1,p);

                    deleted0.resize(v0.tcount); // normals temporarily
                    deleted1.resize(v1.tcount); // normals temporarily

                    // don't remove if flipped
                    if (flipped(p,i0,i1,v0,v1,deleted0))
                        continue;
                    if (flipped(p,i1,i0,v1,v0,deleted1))
                        continue;

                    // not flipped, so remove edge
                    v0.p=p;
                    v0.q=v1.q+v0.q;
                    int tstart=refs.size();

                    update_triangles(i0,v0,deleted0,deleted_triangles);
                    update_triangles(i0,v1,deleted1,deleted_triangles);

                    int tcount=refs.size()-tstart;

                    if (tcount<=v0.tcount)
                    {
                        // save ram
                        if (tcount)
                            memcpy(&refs[v0.tstart],&refs[tstart],tcount*sizeof(Ref));
                    }
                    else
                    {
                        // append
                        v0.tstart=tstart;
                    }

                    v0.tcount=tcount;
                    break;
                }
            }

            // done?
            if (triangle_count-deleted_triangles<=target_count)
                break;
        }
    }

    // clean up mesh
    compact_mesh();

    // ready
    //int timeEnd=timeGetTime();
    //printf("%s - %d/%d %d%% removed in %d ms\n",__FUNCTION__,
    //    triangle_count-deleted_triangles,
    //    triangle_count,deleted_triangles*100/triangle_count,
    //    timeEnd-timeStart);

}

// Check if a triangle flips when this edge is removed

bool Simplify::flipped(vec3f p, int i0, int i1,
                       Vertex &v0,
                       Vertex &v1,
                       std::vector<int> &deleted)
{
    (void)i0; (void)v1;
    for (int k=0;k<v0.tcount;++k)
    {
        Triangle &t=triangles[refs[v0.tstart+k].tid];
        if (t.deleted)
            continue;

        int s=refs[v0.tstart+k].tvertex;
        int id1=t.v[(s+1)%3];
        int id2=t.v[(s+2)%3];

        if (id1==i1 || id2==i1) // delete ?
        {
            deleted[k]=1;
            continue;
        }
        vec3f d1 = vertices[id1].p-p; d1.Normalize();
        vec3f d2 = vertices[id2].p-p; d2.Normalize();
        if (fabs(d1.Dot(d2))>0.999)
            return true;
        vec3f n;
        n = d1.Cross(d2);
        n.Normalize();
        deleted[k]=0;
        if (n.Dot(t.n)<0.2)
            return true;
    }
    return false;
}

// Update triangle connections and edge error after a edge is collapsed

void Simplify::update_triangles(int i0,Vertex &v,std::vector<int> &deleted,int &deleted_triangles)
{
    vec3f p;
    for (int k=0;k<v.tcount;++k)
    {
        Ref &r=refs[v.tstart+k];
        Triangle &t=triangles[r.tid];
        if (t.deleted)
            continue;
        if (deleted[k])
        {
            t.deleted=1;
            deleted_triangles++;
            continue;
        }
        t.v[r.tvertex]=i0;
        t.dirty=1;
        t.err[0]=calculate_error(t.v[0],t.v[1],p);
        t.err[1]=calculate_error(t.v[1],t.v[2],p);
        t.err[2]=calculate_error(t.v[2],t.v[0],p);
        t.err[3]=std::min(t.err[0],std::min(t.err[1],t.err[2]));
        refs.push_back(r);
    }
}

// compact triangles, compute edge error and build reference list

void Simplify::update_mesh(int iteration)
{
    if(iteration>0) // compact triangles
    {
        int dst=0;
        for (std::size_t i=0;i<triangles.size();++i)
        {
            if (!triangles[i].deleted)
            {
                triangles[dst++]=triangles[i];
            }
        }
        triangles.resize(dst);
    }
    //
    // Init Quadrics by Plane & Edge Errors
    //
    // required at the beginning ( iteration == 0 )
    // recomputing during the simplification is not required,
    // but mostly improves the result for closed meshes
    //
    if (iteration == 0)
    {
        for (std::size_t i=0;i<vertices.size();++i)
            vertices[i].q=SymmetricMatrix(0.0);

        for (std::size_t i=0;i<triangles.size();++i)
        {
            Triangle &t=triangles[i];
            vec3f n,p[3];
            for (std::size_t j=0;j<3;++j)
                p[j]=vertices[t.v[j]].p;
            n = (p[1]-p[0]).Cross(p[2]-p[0]);
            n.Normalize();
            t.n=n;
            for (std::size_t j=0;j<3;++j)
                vertices[t.v[j]].q = vertices[t.v[j]].q+SymmetricMatrix(n.x,n.y,n.z,-n.Dot(p[0]));
        }
        for (std::size_t i=0;i<triangles.size();++i)
        {
            // Calc Edge Error
            Triangle &t=triangles[i];vec3f p;
            for (std::size_t j=0;j<3;++j)
                t.err[j] = calculate_error(t.v[j],t.v[(j+1)%3],p);
            t.err[3]=std::min(t.err[0],std::min(t.err[1],t.err[2]));
        }
    }

    // Init Reference ID list
    for (std::size_t i=0;i<vertices.size();++i)
    {
        vertices[i].tstart=0;
        vertices[i].tcount=0;
    }
    for (std::size_t i=0;i<triangles.size();++i)
    {
        Triangle &t=triangles[i];
        for (std::size_t j=0;j<3;++j)
            vertices[t.v[j]].tcount++;
    }
    int tstart=0;
    for (std::size_t i=0;i<vertices.size();++i)
    {
        Vertex &v=vertices[i];
        v.tstart=tstart;
        tstart+=v.tcount;
        v.tcount=0;
    }

    // Write References
    refs.resize(triangles.size()*3);
    for (std::size_t i=0;i<triangles.size();++i)
    {
        Triangle &t=triangles[i];
        for (std::size_t j=0;j<3;++j)
        {
            Vertex &v=vertices[t.v[j]];
            refs[v.tstart+v.tcount].tid=i;
            refs[v.tstart+v.tcount].tvertex=j;
            v.tcount++;
        }
    }

    // Identify boundary : vertices[].border=0,1
    if (iteration == 0)
    {
        std::vector<int> vcount,vids;

        for (std::size_t i=0;i<vertices.size();++i)
            vertices[i].border=0;

        for (std::size_t i=0;i<vertices.size();++i)
        {
            Vertex &v=vertices[i];
            vcount.clear();
            vids.clear();
            for (int j=0; j<v.tcount; ++j)
            {
                int k=refs[v.tstart+j].tid;
                Triangle &t=triangles[k];
                for (int k=0;k<3;++k)
                {
                    std::size_t ofs=0; int id=t.v[k];
                    while(ofs<vcount.size())
                    {
                        if (vids[ofs]==id)
                            break;
                        ofs++;
                    }
                    if(ofs==vcount.size())
                    {
                        vcount.push_back(1);
                        vids.push_back(id);
                    }
                    else
                    {
                        vcount[ofs]++;
                    }
                }
            }
            for (std::size_t j=0;j<vcount.size();++j) {
                if (vcount[j]==1)
                    vertices[vids[j]].border=1;
            }
        }
    }
}

// Finally compact mesh before exiting

void Simplify::compact_mesh()
{
    int dst=0;
    for (std::size_t i=0;i<vertices.size();++i)
    {
        vertices[i].tcount=0;
    }
    for (std::size_t i=0;i<triangles.size();++i)
    {
        if (!triangles[i].deleted)
        {
            Triangle &t=triangles[i];
            triangles[dst++]=t;
            for (std::size_t j=0;j<3;++j)
                vertices[t.v[j]].tcount=1;
        }
    }

    triangles.resize(dst);
    dst=0;
    for (std::size_t i=0;i<vertices.size();++i)
    {
        if (vertices[i].tcount)
        {
            vertices[i].tstart=dst;
            vertices[dst].p=vertices[i].p;
            dst++;
        }
    }
    for (std::size_t i=0;i<triangles.size();++i)
    {
        Triangle &t=triangles[i];
        for (std::size_t j=0;j<3;++j)
            t.v[j]=vertices[t.v[j]].tstart;
    }
    vertices.resize(dst);
}

// Error between vertex and Quadric

double Simplify::vertex_error(const SymmetricMatrix& q, double x, double y, double z)
{
    return   q[0]*x*x + 2*q[1]*x*y + 2*q[2]*x*z + 2*q[3]*x + q[4]*y*y
         + 2*q[5]*y*z + 2*q[6]*y + q[7]*z*z + 2*q[8]*z + q[9];
}

// Error for one edge

double Simplify::calculate_error(int id_v1, int id_v2, vec3f &p_result)
{
    // compute interpolated vertex

    SymmetricMatrix q = vertices[id_v1].q + vertices[id_v2].q;
    bool   border = vertices[id_v1].border & vertices[id_v2].border;
    double error=0;
    double det = q.det(0, 1, 2, 1, 4, 5, 2, 5, 7);

    if (det != 0 && !border)
    {
        // q_delta is invertible
        p_result.x = -1/det*(q.det(1, 2, 3, 4, 5, 6, 5, 7 , 8));    // vx = A41/det(q_delta)
        p_result.y =  1/det*(q.det(0, 2, 3, 1, 5, 6, 2, 7 , 8));    // vy = A42/det(q_delta)
        p_result.z = -1/det*(q.det(0, 1, 3, 1, 4, 6, 2, 5,  8));    // vz = A43/det(q_delta)
        error = vertex_error(q, p_result.x, p_result.y, p_result.z);
    }
    else
    {
        // det = 0 -> try to find best result
        vec3f p1=vertices[id_v1].p;
        vec3f p2=vertices[id_v2].p;
        vec3f p3=(p1+p2)/2;
        double error1 = vertex_error(q, p1.x,p1.y,p1.z);
        double error2 = vertex_error(q, p2.x,p2.y,p2.z);
        double error3 = vertex_error(q, p3.x,p3.y,p3.z);
        error = std::min(error1, std::min(error2, error3));
        if (error1 == error)
            p_result=p1;
        if (error2 == error)
            p_result=p2;
        if (error3 == error)
            p_result=p3;
    }
    return error;
}

///////////////////////////////////////////
// clang-format on
