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

/** Get matrix column.
 * @param A Linear system matrix.
 * @param v Column vector (output).
 * @param col Column index.
 * @param n Linear system dimension.
 */
__kernel void column(__global float* A,
                     __global float* v,
                     unsigned int col,
                     unsigned int n)
{
	// find position in global arrays
	unsigned int i = get_global_id(0);
	if(i >= n)
		return;
	v[i] = A[i + col*n];
}

/** Performs matrix column product by a constant.
 * @param A Linear system matrix.
 * @param c Constant.
 * @param col Column index.
 * @param n Linear system dimension.
 */
__kernel void prod_c_column(__global float* A,
                            float c,
                            unsigned int col,
                            unsigned int n)
{
	// find position in global arrays
	unsigned int i = get_global_id(0);
	if(i >= n)
		return;
	A[i + col*n] *= c;
}

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

/** Compute inner product between a transposed matrix and a vector.
 * @param A Matrix.
 * @param X Vector.
 * @param Y Result.
 * @param n Linear system dimension.
 */
__kernel void dot_matT_vec(__global float* A,
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
		Y[i] += A[i + j*n]*X[j];
	}
}

/** Create u vector for the next iteration.
 * @note u loads beta inside, you must compute
 * the norm and divide him.
 * @param A Linear system matrix.
 * @param u0 u vector from previous step.
 * @param v0 v vector from previous step.
 * @param u Looked u vector for next step.
 * @param alpha \$ \alpha_{i} \$.
 * @param n Linear system dimension.
 */
__kernel void u(__global float* A,
                __global float* u0,
                __global float* v0,
                __global float* u,
                float alpha,
                unsigned int n)
{
	// find position in global arrays
	unsigned int i = get_global_id(0);
	unsigned int j;
	if(i >= n)
		return;
	// Evaluate the row
	u[i] = - alpha * u0[i];
	for(j=0;j<n;j++){
		u[i] += A[j + i*n]*v0[j];
	}
}

/** Create v vector for the next iteration.
 * @note v loads alpha inside, you must compute
 * the norm and divide him.
 * @param A Linear system matrix.
 * @param u u vector for next step.
 * @param v0 v vector from previous step.
 * @param v Looked v vector for next step.
 * @param beta \$ \beta_{i+1} \$.
 * @param n Linear system dimension.
 */
__kernel void v(__global float* A,
                __global float* u,
                __global float* v0,
                __global float* v,
                float beta,
                unsigned int n)
{
	// find position in global arrays
	unsigned int i = get_global_id(0);
	unsigned int j;
	if(i >= n)
		return;
	// Evaluate the row
	v[i] = - beta * v0[i];
	for(j=0;j<n;j++){
		v[i] += A[i + j*n]*u[j];
	}
}

