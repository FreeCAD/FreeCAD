/*
 *   Copyright (c) 2011, 2012                                              *
 *   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License (LGPL)    *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 */

#ifndef M_PI
	#define M_PI 3.14159265f
#endif

#ifndef _NG_
	#define _NG_ 16
#endif

/** Compute \$G_{ab}\$ effect: \n
 * \$ G_{ab} = \frac{1}{4 \pi \vert \mathbf{r} \vert } \$
 * @param r Union vector \$ \mathbf{r}_{ab} \$
 */
float G_val(float4 r)
{
	return 1.f / ( 4.f*M_PI * length(r) );
}

/** Compute \$H_{ab}\$ effect: \n
 * \$ H_{ab} = \frac{\mathbf{r}}{4 \pi \vert \mathbf{r} \vert^3} \cdot n_b \$
 * @param r Union vector \$ \mathbf{r}_{ab} \$
 * @param n Element normal \$ n_b \$
 */
float H_val(float4 r,
            float4 n)
{
	return - dot(r,n) / (4.f*M_PI * pow(dot(r,r),1.5f));
}

/** Computes z coordinate due to the waves superposition
 * for a desired position.
 * @param w Array of waves.
 * @param p Point to compute.
 * @param t Simulation time.
 * @param nW Number of waves.
 * @return z coordinate.
 */
float waves_z(__global float4* w, float4 p, float t, unsigned int nW)
{
	/*
	return 0.f;
	*/

	unsigned int i;
	float z = 0.f;
	for(i=0;i<nW;i++){
		float omega  = 2.f * M_PI / w[i].y;
		float k      = omega * omega / 9.81f;
		float beta   = w[i].w * M_PI / 180.f;
		float l      = p.x*cos(beta) + p.y*sin(beta);
		z           += w[i].x * sin(k*l - omega*t + w[i].z);
	}
	return z;
}

/** Computes velocity potential due to the waves superposition
 * for a desired position.
 * @param w Array of waves.
 * @param p Point to compute.
 * @param t Simulation time.
 * @param nW Number of waves.
 * @return z coordinate.
 */
float waves_phi(__global float4* w, float4 p, float t, unsigned int nW)
{
	/*
	float4 r;
	r.w = 0.f;
	r.xy = p.xy;
	r.z  = 15.f;
	return G_val(r);
	*/


	unsigned int i;
	float z = 0.f;
	for(i=0;i<nW;i++){
		float omega  = 2.f * M_PI / w[i].y;
		float k      = omega * omega / 9.81f;
		float beta   = w[i].w * M_PI / 180.f;
		float l      = p.x*cos(beta) + p.y*sin(beta);
		z           -= w[i].x*omega/k * cos(k*l - omega*t + w[i].z)*exp(k*p.z);
	}
	return z;
}

/** Computes velocity potential gradient z coordinate
 * due to the waves superposition
 * for a desired position.
 * @param w Array of waves.
 * @param p Point to compute.
 * @param t Simulation time.
 * @param nW Number of waves.
 * @return z coordinate.
 */
float waves_gradphi(__global float4* w, float4 p, float t, unsigned int nW)
{
	/*
	float4 r,n;
	r.w = 0.f;
	r.xy = p.xy;
	r.z  = 15.f;
	n.x = 0.f;
	n.y = 0.f;
	n.z = 1.f;
	n.w = 0.f;
	return H_val(r,n);
	*/


	unsigned int i;
	float z = 0.f;
	for(i=0;i<nW;i++){
		float omega  = 2.f * M_PI / w[i].y;
		float k      = omega * omega / 9.81f;
		float beta   = w[i].w * M_PI / 180.f;
		float l      = p.x*cos(beta) + p.y*sin(beta);
		z           -= w[i].x*omega * cos(k*l - omega*t + w[i].z)*exp(k*p.z);
	}
	return z;
}

/** Computes \$G_{ab}\$ and \$H_{ab}\$ in the case that \$a = b\$ \n
 * \$ G_{ab} = \frac{1}{4 \pi \vert \mathbf{r} \vert } \$
 * \$ H_{ab} = \frac{\mathbf{r}}{4 \pi \vert \mathbf{r} \vert^3} \cdot n_b - \frac{1}{2} \$
 * @param positions Array of points of the free surface
 * @param w Array of waves data (Amplitude,period,phase,heading)
 * @param normals Element normals
 * @param I x index of the point to compute.
 * @param J y index of the point to compute.
 * @param L Free surface length in the x direction.
 * @param B Free surface length in the y direction.
 * @param dx Distance between element centers in the x direction.
 * @param dy Distance between element centers in the x direction.
 * @param t Simulation time.
 * @param nx Free surface points in the x direction.
 * @param ny Free surface points in the y direction.
 * @param nW Number of waves.
 */
float2 GH(__global float4* positions,
          __global float4* w,
          __global float4* normals,
          unsigned int I, unsigned int J,
          float L, float B,
          float dx, float dy,
          float t,
          unsigned int nx,
          unsigned int ny,
          unsigned int nW)
{
	__private float4 P[9];
	__private float  K[9]; 
	unsigned int i,j;
	float4 p = positions[I*ny + J];
	float4 n = normals[I*ny + J];
	float Dx = dx / _NG_;
	float Dy = dy / _NG_;
	for(i=0;i<3;i++){
		for(j=0;j<3;j++){
			P[i*3+j].x = p.x + (i-1)*dx;
			P[i*3+j].y = p.y + (j-1)*dy;
			if( (P[i*3+j].x > -0.5*L) &&
			    (P[i*3+j].x <  0.5*L) &&
			    (P[i*3+j].y > -0.5*B) &&
			    (P[i*3+j].y <  0.5*B))
				P[i*3+j].z = positions[(I-1+i)*ny + (J-1+j)].z;
			else
				P[i*3+j].z = waves_z(w, P[i*3+j], t, nW);
			P[i*3+j].w = 0.f;
		}
	}
	// Get SPline surface coeffs
	K[0] = P[0].z;                               // k_{0}
	K[1] = 4*P[3].z - P[6].z - 3*P[0].z;         // k_{u}
	K[2] = 4*P[1].z - P[2].z - 3*P[0].z;         // k_{v}
	K[3] = P[8].z - 4*P[7].z + 3*P[6].z + 
           3*P[2].z - 12*P[1].z + 9*P[0].z + 
           -4*P[5].z + 16*P[4].z - 12*P[3].z;    // k_{uv}
	K[4] = 2*P[6].z + 2*P[0].z - 4*P[3].z;       // k_{uu}
	K[5] = 2*P[2].z + 2*P[0].z - 4*P[1].z;       // k_{vv}
	K[6] = -2*P[8].z + 8*P[7].z - 6*P[6].z + 
           -2*P[2].z + 8*P[1].z - 6*P[0].z + 
           4*P[5].z - 16*P[4].z + 12*P[3].z;     // k_{uuv}
	K[7] = -2*P[8].z + 4*P[7].z - 2*P[6].z + 
           -6*P[2].z + 12*P[1].z - 6*P[0].z + 
           8*P[5].z - 16*P[4].z + 8*P[3].z;      // k_{uuv}
	K[8] = 4*P[8].z - 8*P[7].z + 4*P[6].z + 
           4*P[2].z - 8*P[1].z + 4*P[0].z + 
           -8*P[5].z + 16*P[4].z - 8*P[3].z;     // k_{uuvv}
	// Loop around the point p collecting the integral
	float2 gh;
	gh.x =  0.0f;
	gh.y = -0.5f;
	for(i=0;i<_NG_;i++){
		for(j=0;j<_NG_;j++){
			float4 p_a;
			float u,v;
			p_a.x = positions[I*ny + J].x - 0.5f*dx + (i+0.5f)*Dx;
			p_a.y = positions[I*ny + J].y - 0.5f*dy + (j+0.5f)*Dy;
			u     = (p_a.x - P[0].x) / (P[6].x - P[0].x);
			v     = (p_a.y - P[0].y) / (P[2].y - P[0].y);
			p_a.z = K[0] + K[1]*u + K[2]*v + K[3]*u*v + 
                    K[4]*u*u + K[5]*v*v + K[6]*u*u*v + 
                    K[7]*u*v*v + K[8]*u*u*v*v;
			p_a.w = 1.f;
			gh.x += G_val(p_a - p)*Dx*Dy;
			// For some reason H is not well integrated
			// gh.y += H_val(p_a - p, n)*Dx*Dy;
		}
	}
	return gh;
}

/** Compute Linear system matrix. Desingularized sources must taken into account.
 * @param A Linear system matrix.
 * @param B Independent term for velocity potentials.
 * @param positions Elements points.
 * @param areas Elements area.
 * @param normals Elements normals.
 * @param p Velocity potentials.
 * @param dp Acceleration potentials.
 * @param waves Array of waves data (Amplitude,period,phase,heading)
 * @param l Free surface length in the x direction.
 * @param b Free surface length in the y direction.
 * @param dx Distance between element centers in the x direction.
 * @param dy Distance between element centers in the x direction.
 * @param t Simulation time.
 * @param nx Free surface points in the x direction.
 * @param ny Free surface points in the y direction.
 * @param nFS Number of points in the free surface.
 * @param nB Number of points in the body (ignored yet, should be 0).
 * @param n Total number of points.
 * @param nSeax Number of repetitions of the free surface in the x direction.
 * @param nSeay Number of repetitions of the free surface in the y direction.
 * @param nW Number of waves.
 */
__kernel void matrixGen(__global float*  A,
                        __global float*  B,
                        __global float4* positions,
                        __global float*  areas,
                        __global float4* normals,
                        __global float*  p,
                        __global float*  dp,
                        __global float4* waves,
                        float l,
                        float b,
                        float dx,
                        float dy,
                        float t,
						unsigned int nx,
						unsigned int ny,
                        unsigned int nFS,
                        unsigned int nB,
                        unsigned int n,
                        int nSeax,
                        int nSeay,
                        unsigned int nW)
{
	// find position in global arrays
	unsigned int i = get_global_id(0);
	unsigned int j;
	int I,J;
	if(i >= n)
		return;
	// Get the point where we want to evaluate
	float4 p_a = positions[i];
	// Evaluate the row
	B[i] = 0.f;
	for(j=0;j<nFS;j++){
		// Get G,H for this pair of points
		float2 gh;
		float4 p_b = positions[j];
		float4 n_b = normals[j];
		if(i == j){
			gh = GH(positions,waves,normals,i/ny,i%ny,
			        l,b,dx,dy,t,nx,ny,nW);
		}
		else{
			p_b = positions[j];
			gh.x = G_val(p_b - p_a)*dx*dy;
			gh.y = H_val(p_b - p_a, n_b)*dx*dy;
		}
		// In the free surface G goes to the linear system
		// matrix while H goes to the B term
		A[j + i*n]  = -gh.x;
		B[i]       += gh.y * p[j];
		// Compute also the terms of the virtual free surface
		// extension.
		for(I=-nSeax;I<=nSeax;I++){
			for(J=-nSeay;J<=nSeay;J++){
				if((!I) && (!J))
					continue;
				float4 p_c = p_b;
				p_c.x += I*l;
				p_c.y += J*b;
				p_c.z  = waves_z(waves, p_c, t, nW);
				gh.x = G_val(p_c - p_a)*dx*dy;
				gh.y = H_val(p_c - p_a, n_b)*dx*dy;
				// In the extended free surface both terms goes to
				// the B term
				B[i] += gh.y*waves_phi(waves, p_c, t, nW) - gh.x*waves_gradphi(waves, p_c, t, nW);
			}
		}
	}
}

