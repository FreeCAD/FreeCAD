#!/usr/bin/python
# Tool for manually calculating the unfolded geometry
# See terminology.png for terminology.
# -*- coding: utf-8 -*-
###################################################################################
#
#  calc-unfold.py
#
#  Copyright 2019 Cerem Cem Aslan
#  Copyright 2023 Ondsel Onc.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
###################################################################################

import math

r = inner_radius = 1.64
T = thickness = 2.0
ML = mold_line_distance = 50
K = k_factor = 0.38
bend_angle = 90.0

t = thickness * k_factor
BA = bend_allowance = 2.0 * math.pi * (r + t) * (bend_angle / 360.0)
leg_length = ML - BA / 2.0
ossb = r + T
flange_diff = ossb - BA / 2.0
flange_length = ossb + leg_length


print("Inputs: r, T, Kf, ML, angle")
print("---------------------------------")
print(f"Effective inner radius: {inner_radius:.2f} mm")
print(f"Effective outer radius: {(r + T):.2f} mm")
print(f"Flange length: {flange_length:.2f} mm")
print(f"* Flange diff (FD = FL - ML): {flange_diff:.2f} mm")
print(f"Bend allowance: {BA:.2f} mm")
print(f"Leg length: {leg_length:.2f} mm")
