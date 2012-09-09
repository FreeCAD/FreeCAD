/*
 * -----------------------------------------------------------------------
 *
 * This source file is part of AQUA-gpusph.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 *
 *
 *    Authors:
 *    - Cercos Pita, Jose Luis
 *	- Miguel Gonzalez, Leo
 *	- Saelices, Jaime
 *	- Souto Iglesias, Antonio
 *
 * -----------------------------------------------------------------------
 */

#ifndef M_PI
	#define M_PI 3,14159265359
#endif

#ifdef _g
	#error '_g' is already defined.
#endif
#define _g __global

#ifdef _l
	#error '_l' is already defined.
#endif
#define _l __local

#ifdef _c
	#error '_c' is already defined.
#endif
#define _c __constant

#ifndef _grav
	#define _grav 9.81
#endif

/** Setup velocity and acceleration potential for initial time step.
 * @param pos Cell position.
 * @param v Cell velocity.
 * @param f Cell acceleration.
 * @param waves Waves (A,T,phase,heading)
 * @param phi Velocity potential.
 * @param Phi Acceleration potential
 * @param N Number of cell elements at each direction.
 * @param n Number of waves.
 */
__kernel void FS(_g float4* pos, _g float4* v, _g float4* f,
                 _g float4* waves, _g float* phi, _g float* Phi,
                 uint2 N, uint n)
{
    // find position in global arrays
    unsigned int i = get_global_id(0);
    unsigned int j = get_global_id(1);
	if( (i >= N.x) || (j >= N.y) )
		return;
	unsigned int id = i*N.y + j;

	// ---- | ------------------------ | ----
	// ---- V ---- Your code here ---- V ----

	unsigned int w;
	for(w=0;w<n;w++){
		float A = waves[w].x;
		float T = waves[w].y;
		float phase = waves[w].z;
		float heading = M_PI*waves[w].w/180.f;
		float lambda  = 0.5f*_grav/M_PI * T*T;
		float k       = 2.f*M_PI/lambda;
		float frec    = 2.f*M_PI/T;
		float l       = pos[id].x*cos(heading) + pos[id].y*sin(heading);
		// Position, velocity and acceleration
		float amp     = A*sin(k*l + phase);
		pos[id].z     = amp;
		v[id].z       = -frec*amp;
		f[id].z       = frec*frec*amp;
		// Potentials
		phi[id]       = _grav/frec*amp;
		Phi[id]       = -_grav*amp;
	}

	// ---- A ---- Your code here ---- A ----
	// ---- | ------------------------ | ----
}
