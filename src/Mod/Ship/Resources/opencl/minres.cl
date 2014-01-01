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

/** Compute residuals of the solution stimator for a linear system.
 * @param A Linear system matrix.
 * @param B Linear system independent term.
 * @param X Solution estimation.
 * @param R Residuals.
 * @param n Linear system dimension.
 */
__kernel void r(__global float* A,
                __global float* B,
                __global float* X,
                __global float* R,
                unsigned int n)
{
	// find position in global arrays
	unsigned int i = get_global_id(0);
	unsigned int j;
	if(i >= n)
		return;
	// Evaluate the row
	R[i] = B[i];
	for(j=0;j<n;j++){
		R[i] -= A[j + i*n]*X[j];
	}
}

/** Compute inner product between a matrix and a vector.
 * @param A Matrix.
 * @param X Vector.
 * @param Y Result.
 * @param n Linear system dimension.
 */
__kernel void dot_mat_vec(__global float* A,
                          __global float* X,
                          __global float* Y,
                          unsigned int n)
{
	// find position in global arrays
	unsigned int i = get_global_id(0);
	unsigned int j;
	if(i >= n)
		return;
	// Evaluate the row
	Y[i] = 0.f;
	for(j=0;j<n;j++){
		Y[i] += A[j + i*n]*X[j];
	}
}

/** Solve a linear system using Minimal residues iterative method.
 * @param A Linear system matrix.
 * @param r Residues vector.
 * @param x0 Solution of the previous iteration.
 * @param x Solution of the actual iteration.
 * @param Ar_r dot(dot(A,r), r).
 * @param Ar_Ar dot(dot(A,r), dot(A,r)).
 * @param n Linear system dimension.
 */
__kernel void minres(__global float* A,
                     __global float* r,
                     __global float* x0,
                     __global float* x,
                     float Ar_r,
                     float Ar_Ar,
                     unsigned int n)
{
	// find position in global arrays
	unsigned int i = get_global_id(0);
	if(i >= n)
		return;
	// Evaluate the row
	x[i] = x0[i] + Ar_r / Ar_Ar * r[i];
}

