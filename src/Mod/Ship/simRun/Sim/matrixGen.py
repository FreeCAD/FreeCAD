#***************************************************************************
#*																		 *
#*   Copyright (c) 2011, 2012											  *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>							*
#*																		 *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)	*
#*   as published by the Free Software Foundation; either version 2 of	 *
#*   the License, or (at your option) any later version.				   *
#*   for detail see the LICENCE text file.								 *
#*																		 *
#*   This program is distributed in the hope that it will be useful,	   *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of		*
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		 *
#*   GNU Library General Public License for more details.				  *
#*																		 *
#*   You should have received a copy of the GNU Library General Public	 *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA																   *
#*																		 *
#***************************************************************************

# numpy
import numpy as np

grav=9.81

class simMatrixGen:
	def __init__(self, context=None, queue=None):
		""" Constructor.
		@param context OpenCL context where apply. Only for compatibility, 
		must be None.
		@param queue OpenCL command queue. Only for compatibility, 
		must be None.
		"""
		self.context = context
		self.queue   = queue

	def execute(self, x, p, dp, fs, sea, bem, body):
		""" Compute system matrix.
		@param x Free surface z coordinates.
		@param fs Free surface instance.
		@param sea Sea boundary instance.
		@param bem Boundary Element Method instance.
		@param body Body instance.
		"""
		nFS	  = fs['N']
		nSea  = sea['N']
		nB	  = body['N']
		n     = nFS + nSea + nB
		A     = bem['A']
		B     = bem['B']
		dB    = bem['dB']
		# Free surface sources rows
		nx	  = fs['Nx']
		ny	  = fs['Ny']
		for i in range(0,nx):
			for j in range(0,ny):
				pos    = np.copy(fs['pos'][i,j])
				pos[2] = x[i,j]
				# Compute G terms
				fsG  = self.fsG(x, pos, fs)
				seaG = self.seaG(pos, sea)
				# Compute H terms
				fsH  = self.fsH(i*ny+j, x, pos, fs)
				seaH = self.seaH(i*ny+j, pos, fs, sea)
				# Append terms to linear system matrix
				A[i*ny+j,0:nFS] = fsG
				A[i*ny+j,nFS:n] = seaH
				# Set independent terms
				B[i*ny+j]       = np.dot(fsH, p[0:nFS])  + np.dot(seaG, p[nFS:nFS+nSea])
				dB[i*ny+j]      = np.dot(fsH, dp[0:nFS]) + np.dot(seaG, dp[nFS:nFS+nSea])
				# Append body effect
				# ...
		# Sea sources rows
		ids   = ['front','back','left','right','bottom']
		count = 0
		for index in ids:
			s  = sea[index]
			nx = s['Nx']
			ny = s['Ny']
			for i in range(0,nx):
				for j in range(0,ny):
					pos    = np.copy(s['pos'][i,j])
					# Compute G terms
					fsG  = self.fsG(x, pos, fs)
					seaG = self.seaG(pos, sea)
					# Compute H terms
					fsH  = self.fsH(nFS+count, x, pos, fs)
					seaH = self.seaH(nFS+count, pos, fs, sea)
					# Append terms to linear system matrix
					A[nFS+count, 0:nFS] = fsG
					A[nFS+count, nFS:n] = seaH
					# Set independent terms
					B[nFS+count]        = np.dot(fsH, p[0:nFS])  + np.dot(seaG, p[nFS:nFS+nSea])
					dB[nFS+count]       = np.dot(fsH, dp[0:nFS]) + np.dot(seaG, dp[nFS:nFS+nSea])
					# Append body effect
					# ...
					count = count + 1
		# Solid sources rows
		# ...

	def fsG(self, x, pos, fs):
		r""" Compute free surface terms potential effect over desired position. Desingularized 
		sources must taken into account.
		\$ G_{ij} = \sum_{j=0}^{n_{FS}-1} \log(\mathbf{r}_{ij}) \$
		@param x Free surface z coordinates.
		@param pos Point to evaluate.
		@param fs Free surface instance.
		@return Free surface effect row.
		"""
		nx  = fs['Nx']
		ny  = fs['Ny']
		nF  = nx*ny
		row = np.ndarray(nF, dtype=np.float32)
		for i in range(0,nx):
			for j in range(0,ny):
				# Get source position (desingularized)
				source    = np.copy(fs['pos'][i,j])
				source[2] = x[i,j]
				area      = fs['area'][i,j]
				normal    = fs['normal'][i,j]
				source    = source + np.sqrt(area)*normal
				# Get union vector between points
				r           = pos-source
				row[i*ny+j] = area * 0.5*np.log(np.dot(r,r))
		return row

	def fsH(self, index, x, pos, fs):
		r""" Compute free surface terms potential gradient effect over desired position. Desingularized 
		sources must taken into account.
		\$ H_{ij} = \sum_{j=0}^{n_{FS}-1} \frac{\mathbf{r}_{ij}}{\vert \mathbf{r}_{ij} \vert^2} \$
		When the point effect over himself is considered, -1/2 must be append.
		@param index Potential point index.
		@param x Free surface z coordinates.
		@param pos Point to evaluate.
		@param fs Free surface instance.
		@return Free surface effect row.
		"""
		nx  = fs['Nx']
		ny  = fs['Ny']
		nF  = nx*ny
		row = np.ndarray(nF, dtype=np.float32)
		for i in range(0,nx):
			for j in range(0,ny):
				# Get source position (desingularized)
				source    = np.copy(fs['pos'][i,j])
				source[2] = x[i,j]
				area      = fs['area'][i,j]
				normal    = fs['normal'][i,j]
				source    = source + np.sqrt(area)*normal
				# Get union vector between points
				r           = pos-source
				row[i*ny+j] = area * np.dot(r,normal) / np.dot(r,r)
				# If effect over himslef is considered, apply the correction
				if(index == i*ny+j):
					row[i*ny+j] = row[i*ny+j] - 0.5
		return row

	def seaG(self, pos, sea):
		r""" Compute sea boundary terms potential effect over desired position. Desingularized 
		sources must taken into account.
		\$ G_{ij} = \sum_{j=0}^{n_{FS}-1} \log(\mathbf{r}_{ij}) \$
		@param pos Point to evaluate.
		@param sea Sea boundaries instance.
		@return Sea boundaries effect row.
		"""
		ids   = ['front','back','left','right','bottom']
		count = 0
		row   = np.ndarray(sea['N'], dtype=np.float32)
		for index in ids:
			s  = sea[index]
			nx = s['Nx']
			ny = s['Ny']
			for i in range(0,nx):
				for j in range(0,ny):
					# Get source position (desingularized)
					source = np.copy(s['pos'][i,j])
					area   = s['area'][i,j]
					normal = s['normal'][i,j]
					source = source + np.sqrt(area)*normal
					# Get distance between points
					r          = pos-source
					row[count] = area * 0.5*np.log(np.dot(r,r))
					count      = count + 1
		return row

	def seaH(self, index, pos, fs, sea):
		r""" Compute sea boundary terms potential gradient effect over desired position. Desingularized 
		sources must taken into account.
		\$ H_{ij} = \sum_{j=0}^{n_{FS}-1} \frac{\mathbf{r}_{ij}}{\vert \mathbf{r}_{ij} \vert^2} \$
		When the point effect over himself is considered, -1/2 must be append.
		@param index Potential point index.
		@param pos Point to evaluate.
		@param fs Free surface instance.
		@param sea Sea boundaries instance.
		@return Sea boundaries effect row.
		"""
		nF    = fs['N']
		ids   = ['front','back','left','right','bottom']
		count = 0
		row   = np.ndarray(sea['N'], dtype=np.float32)
		for index in ids:
			s  = sea[index]
			nx = s['Nx']
			ny = s['Ny']
			for i in range(0,nx):
				for j in range(0,ny):
					# Get source position (desingularized)
					source = np.copy(s['pos'][i,j])
					area   = s['area'][i,j]
					normal = s['normal'][i,j]
					source = source + np.sqrt(area)*normal
					# Get distance between points
					r          = pos-source
					row[count] = area * np.dot(r,normal) / np.dot(r,r)
					# If effect over himslef is considered, apply the correction
					if(index == count+nF):
						row[count] = row[count] - 0.5
					count      = count + 1
		return row

