# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 russ4262 <russ4262@gmail.com>                      *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

from __future__ import print_function

__title__ = "Path Surface Utilities Module"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Support functions and classes for 3D Surface."
__contributors__ = ""

import FreeCAD
from PySide import QtCore
import PathScripts.PathLog as PathLog

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')
math = LazyLoader('math', globals(), 'math')
Draft = LazyLoader('Draft', globals(), 'Draft')


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


# Functions for getting a shape envelope and cross-section
class ProjectionToFace():
    """ProjectionToFace(solid, seed_point=None) class
    This class requires a solid shape, and optional seed point arguments.
    This class creates a 2D projection of the solid.  Next, it refines
    the edges contained within the projection. Last it applies an
    algorithm to extract the outer wire of the refined projection edges.
    
    The algorithm used to identify the outer wire works as follows:
        - Identify an edge that lies on the outer wire.  This is done
          using a vector from seed point to (xMin, yMin) vertex in list
          of refined edges from 2D projection.
        - Move only in a counter-clockwise direction around the perimeter
          of the projection edges.
        - At each outer wire node (three or more edges meeting at single vertex),
          determine first edge in node group when rotating counter-clockwise
          from current edge.
        - The basic theory is like maze theory - follow EITHER the left or right wall
          in a maze and you will always complete the maze.  In this case we stay
          to the right (counter-clockwise).
    
    The algorithm used to identify the outer wire could be isolated and placed
    within an independent class.  Doing so would then allow the user to provide
    the new class with a set of horizontally planar edges,
    and retrieve the outer wire.
    
    Future improvements to this class should include a better algorithm
    to fill in very short missing edge segments."""

    def __init__(self, solid, seed_point=None):
        self.solid = solid
        self.seed_point = seed_point
        self.precision = 5
        self.diff_angle_limit = 0.3
        self.diff_length_percent_limit = 0.90

        self.found_outer_loop = False
        self.projection_face = None
        self.edges = None
        self.outer_wires = list()
        self.outer_wires_indexes = list()

        self.outer_edges = None
        self.outer_edges_indexes = None
        self.remaining_edges = None
        self.remaining_edges_indexes = None
        self.edge_data = None
        self.node_data = None
        self.min_vertex_data = None
        self.no_matches = None
        self.not_processed = None
        self.atch_vert_0 = None
        self.atch_vert_1 = None
        self.node_data_new = None
        self.node_cnt = None
        self.edges_cnt = None

        # Debug variables
        self.range_start = 35
        self.trgt_idx = 41

    # Private(internal) methods
    def _get_refined_projection(self):
        """_get_refined_projection()...
        Creates a 2D projection of the class instance's solid.
        Refines the projection edge list.
        Fuses all refined edges together."""
        PathLog.debug('get_refined_projection()')

        fcad = FreeCAD.ActiveDocument
        F = fcad.addObject('Part::Feature', 'tmpProjectionWire')
        name1 = F.Name
        F.Shape = self.solid
        F.purgeTouched()
        try:
            prj = Draft.makeShape2DView(F, FreeCAD.Vector(0, 0, 1))
            prj.HiddenLines = False
            prj.recompute()
            name2 = prj.Name
            prj.purgeTouched()
        except Exception as ee:
            PathLog.error(translate("PathSurfaceUtils", "Failed to make projection of solid."))
            PathLog.error(str(ee))
            return False
        else:
            edges = prj.Shape.Edges  # all edges are BSplines
            try:
                pWire = Part.Wire(edges)
                if pWire.isClosed():
                    pWire.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - pWire.BoundBox.ZMin))
                    fcad.removeObject(name1)
                    fcad.removeObject(name2)
                    indexes = [i for i in range(0, len(edges))]
                    self.outer_edges = edges
                    self.outer_edges_indexes = indexes
                    self.outer_wires.append(pWire)
                    self.outer_wires_indexes.append(indexes)
                    self.found_outer_loop = True
                    return True
            except Exception as exc:
                PathLog.debug(translate("PathSurfaceUtils", "Projection is not simple closed wire."))
                PathLog.debug(str(exc))

        # Refine the list of edges, removing duplicates and isolated edges
        rpe = RefineProjectionEdges(edges)
        attached, detached = rpe.get_refined_edges()

        # Fuse all refined edges together
        e0 = attached.pop(0)
        e1 = attached.pop(0)
        fusion = e0.fuse(e1)
        for i in range(0, len(attached)):
            f = fusion.fuse(attached[i])
            fusion = f

        fcad.removeObject(name1)
        fcad.removeObject(name2)

        self.edges = fusion.Edges
        return True

    def _make_seed_point(self, edges):
        """_make_seed_point(edges)...
        Uses the set of refined edges from the projection to determine
        an offset seed point related to the (XMin, YMin) coordinate
        of the list of edges."""
        PathLog.debug('_make_seed_point()')

        edges = Part.makeCompound(edges)
        ceBB = edges.BoundBox
        xmin_ymin = FreeCAD.Vector(ceBB.XMin, ceBB.YMin, 0.0)
        xmax_ymax = FreeCAD.Vector(ceBB.XMax, ceBB.YMax, 0.0)
        diag = xmin_ymin.sub(xmax_ymax).normalize()
        diag.multiply(2.0)
        self.seed_point = diag

    def _populate_coordinate_lists(self):
        """_populate_coordinate_lists(edges)...
        Creates a set of data lists to be used in the outer wire
        identification algorithm."""
        PathLog.debug('_populate_coordinate_lists()')

        edges = self.edges
        self.edges_cnt = len(edges)
        precision = self.precision
        data = list()
        atch_vert_0 = list()
        atch_vert_1 = list()
        not_processed = list()
        x_min = max(round(edges[0].Vertexes[0].X, precision), round(edges[0].Vertexes[1].X, precision))
        y_min = max(round(edges[0].Vertexes[0].Y, precision), round(edges[0].Vertexes[1].Y, precision))
        idx_min = (0, 0)

        # extract edge data into list for reference
        for i in range(0, self.edges_cnt):
            e = edges[i]
            vrtxs = e.Vertexes

            v0x = round(vrtxs[0].X, precision)
            v0y = round(vrtxs[0].Y, precision)
            # v0z = round(edge_vvrtxsertexes[0].Z, precision)
            v1x = round(vrtxs[1].X, precision)
            v1y = round(vrtxs[1].Y, precision)
            # v1z = round(vrtxs[1].Z, precision)

            # Create point text
            v0_txt = 'x{}_y{}'.format(v0x, v0y)
            v1_txt = 'x{}_y{}'.format(v1x, v1y)

            # Identify x_min and y_min
            if v0x <= x_min:
                if v0y < y_min:
                    x_min = v0x
                    y_min = v0y
                    idx_min = (i, 0)
            if v1x <= x_min:
                if v1y < y_min:
                    x_min = v1x
                    y_min = v1y
                    idx_min = (i, 1)

            # Assign data to primary data list
            tup = (v0_txt, v1_txt, i)
            data.append(tup)

            # add placeholders to lists for later population
            atch_vert_0.append(list())
            atch_vert_1.append(list())
            not_processed.append(True)
        # Efor

        # Save edge data to class variables
        self.min_vertex_data = idx_min
        self.edge_data = data

        # Save empty data lists to class variables
        self.atch_vert_0 = atch_vert_0
        self.atch_vert_1 = atch_vert_1
        self.not_processed = not_processed

    def _identify_attachments(self):
        """_identify_attachments(edges)...
        Identifies and logs attachment data between edges."""
        PathLog.debug('_identify_attachments()')

        nodes = list()
        data = list()
        no_matches = list()

        # prepare source edge data
        for (p0, p1, i) in self.edge_data:
            data.append((p0, p1, i, 0))
            data.append((p1, p0, i, 1))
        data.sort(key=lambda t: t[0])

        stop = len(data)
        i = 0
        while i < stop:
            p1, p2, ei, vi = data[i]
            match_cnt = 1
            this_tup = (ei, vi)
            matching = [this_tup]  # Track matching edge indexes

            for n in range(i + 1, stop):
                np1, np2, nei, nvi = data[n]
                if p1 == np1:
                    mtch_tup = (nei, nvi)
                    matching.append(mtch_tup)  # add matching edge index
                    match_cnt += 1  # increment match count
                    # Debug feedback
                else:
                    break

            if match_cnt > 1:
                # Save attachment data
                for c in range(0, match_cnt):
                    (mei, mvi) = matching[c]
                    atch_list = getattr(self, 'atch_vert_{}'.format(mvi))[mei]
                    for m in matching:
                        # Only add to non-self edges
                        if m[0] != mei:
                            atch_list.append(m)

                # Test for node point
                if match_cnt > 2:
                    # this is node point
                    nodes.append((p1, ei, matching))

                # advance index to next item after matches
                i += match_cnt
            else:
                no_matches.append(data[i])
                PathLog.debug('New ... No match count: {}'.format(data[i]))
                i += 1
        # Ewhile

        # Save data
        self.node_data = nodes
        self.no_matches = no_matches

    def _make_missing_edge(self):
        """_make_missing_edge(edges)...
        This function only produces output if there are two vertexes
        that have no attachments (dead ends).
        This method needs improvement to better handle gaps identified
        within the projectcion edges."""
        PathLog.debug('_make_missing_edge()')

        no_matches = self.no_matches
        if len(no_matches) == 2:
            PathLog.debug('len(no_matches) == 2')
            # Fill gap with line segment
            e0 = no_matches[0][2]  # edge index
            e1 = no_matches[1][2]  # edge index
            e0v = no_matches[0][3]  # vertex index
            e1v = no_matches[1][3]  # vertex index
            # Make connecting edge as line segment
            pnt0 = self.edges[e0].Vertexes[e0v].Point
            pnt1 = self.edges[e1].Vertexes[e1v].Point
            self.missing_edge = Part.makeLine(pnt0, pnt1)
            # Add line segment to edges list
            self.edges.append(self.missing_edge)

            # Update attachment lists for source edges
            atch_list = getattr(self, 'atch_vert_{}'.format(e0v))[e0]
            mtch_tup = (self.edges_cnt, 0)
            atch_list.append(mtch_tup)
            atch_list = getattr(self, 'atch_vert_{}'.format(e1v))[e1]
            mtch_tup = (self.edges_cnt, 1)
            atch_list.append(mtch_tup)

            # Update attachment lists for new connecting edge
            self.atch_vert_0.append([(e0, e0v)])
            self.atch_vert_1.append([(e1, e1v)])
            self.not_processed.append(True)

    def _get_edge_angle(self, ei, vi):
        """_get_edge_angle(edge_index, vertex_index_of_edge)...
        This function returns a standard angle in degrees
        for the edge index provided, using the vertex index as the origin."""
        PathLog.debug('_get_edge_angle()')

        edge = self.edges[ei]
        # Identify start point
        if vi == 0:
            # Regular direction
            p0 = edge.Vertexes[0].Point
        else:
            # Reversed direction
            p0 = edge.Vertexes[1].Point

        edge_len = edge.Length
        dist = 1.0  # 0.5
        if edge_len > dist:
            # Find point at distance along edge
            if vi == 0:
                # Regular direction
                p1 = edge.valueAt(edge.FirstParameter + dist)
            else:
                # Reversed direction
                p1 = edge.valueAt(edge.LastParameter - dist)
        else:
            if vi == 0:
                # Regular direction
                p1 = edge.Vertexes[1].Point
            else:
                # Reversed direction
                p1 = edge.Vertexes[0].Point
        vector = p1.sub(p0)

        return vector_to_degrees(vector)

    def _register_edge(self, ei):
        """_register_edge(edge_index)...
        Register an edge as used, and add it to the outer wire edge list.
        This function takes an edge index argument with the index referring
        to the list of refined projection edges being analyzed."""
        PathLog.debug('_register_edge()')

        edge = self.edges[ei]
        self.not_processed[ei] = False
        self.outer_edges.append(edge)
        self.outer_edges_indexes.append(ei)

    def _identify_next_edge(self, edge_idx, edge_angle, attached_edges_angle_tups):
        """_identify_next_edge(edge_idx, edge_angle, attached_edges_angle_tups)...
        This function identifies the next outer edge in a list of attached edges
        to the current edge identified by edge_idx."""
        PathLog.debug('_identify_next_edge()')

        pos = list()
        neg = list()

        def zero(tup):
            return tup[0]

        for (ang, ei, vi) in attached_edges_angle_tups:
            diff = ang - edge_angle
            if diff < 0.0:
                neg.append((diff, ei, vi))
            else:
                pos.append((diff, ei, vi))

        if pos:
            pos.sort(key=zero)
        if neg:
            neg.sort(key=zero)

        pos.extend(neg)
        PathLog.debug('edge_angle: {};  diff list: {}'.format(edge_angle, pos))

        rtn = None
        for i in range(0, len(pos)):
            (diff, ei, vi) = pos[i]
            # filter out very acute angles and similar length transitions
            if abs(diff) < self.diff_angle_limit:
                if self.edges[ei].Length < self.edges[edge_idx].Length * self.diff_length_percent_limit:
                    rtn = (diff, ei, vi)
                    break
                else:
                    # Flag bad edge as used
                    PathLog.debug('Flagging bad E[{}] as used.'.format(ei))
                    self.not_processed[ei] = False
            else:
                rtn = (diff, ei, vi)
                break
        if rtn:
            return rtn
        else:
            PathLog.debug('No next seg identified to return.')
            return pos[0]

    def _find_outer_edges(self):
        """_find_outer_edges()...
        This is the main control method for the class. It calls the necessary
        methods in proper sequence and constructs the outer edges of the
        projection as it progresses."""
        PathLog.debug('_find_outer_edges()')

        self.outer_edges = list()
        self.outer_edges_indexes = list()
        first_edge_idx = None

        # Make start line to x_min y_min vertex using seed_point
        edge_idx = self.min_vertex_data[0]
        vrtx_idx = self.min_vertex_data[1]
        origin = self.edges[edge_idx].Vertexes[vrtx_idx].Point
        first_edge_idx = edge_idx

        # get seed_point angle
        vect_ang = vector_to_degrees(self.seed_point)
        adjust_angle = 360.0 - vect_ang

        # Get angle of starting edge
        first_angle = self._get_edge_angle(edge_idx, vrtx_idx)
        attached_edges_angle_tups = [(first_angle + adjust_angle, edge_idx, vrtx_idx)]

        # Get angles of segments connected to first edge
        atch_list = getattr(self, 'atch_vert_{}'.format(vrtx_idx))[edge_idx]
        PathLog.debug('atch_list: e-{}, v-{}'.format(edge_idx, vrtx_idx))
        PathLog.debug('origin atch_list: {}'.format(atch_list))
        for (ei, vi) in atch_list:
            angle = self._get_edge_angle(ei, vi)
            attached_edges_angle_tups.append((angle + adjust_angle, ei, vi))
        
        # Sort segment angles
        attached_edges_angle_tups.sort(key=lambda tup: tup[0])

        (ang, ei, vi) = attached_edges_angle_tups[0]  # next_seg
        self._register_edge(ei)
        PathLog.debug('... Saving connection to Edges[{}] v-{}'.format(ei, vi))
        cont = self.edges_cnt * 2 + 1
        while cont:
            cvi = swap_index(vi)
            attached_edges_angle_tups = list()

            origin = self.edges[ei].Vertexes[cvi].Point
            endpoint = self.edges[ei].Vertexes[vi].Point
            vector = endpoint.sub(origin)
            vect_ang = vector_to_degrees(vector)
            adjust_angle = 360.0 - vect_ang
            adjust_angle = 0.0

            # Get angles of segments connected to first edge
            atch_list = getattr(self, 'atch_vert_{}'.format(cvi))[ei]
            atch_list_len = len(atch_list)

            PathLog.debug('Scanning Edges[{}] v-{}'.format(ei, cvi))

            if ei == self.trgt_idx:
                PathLog.debug('___ {}'.format(cont))
                PathLog.debug('vector: {},  vect_ang: {}'.format(vector, vect_ang))
                PathLog.debug('origin atch_list E[{}], vi-{}: {}'.format(ei, cvi, atch_list))
            if atch_list_len == 0:
                PathLog.debug('atch_list is empty.  E{}, v-{}'.format(ei, cvi))
                PathLog.debug('while loop {}'.format(cont))
                break
            elif atch_list_len == 1:
                attached_edges_angle_tups.append((1.0, atch_list[0][0], atch_list[0][1]))
            else:
                for (eei, vvi) in atch_list:
                    if self.not_processed[eei]:
                        angle = self._get_edge_angle(eei, vvi)
                        attached_edges_angle_tups.append((angle + adjust_angle, eei, vvi))
                        # PathLog.debug('Edges[{}] angle: {}'.format(eei, angle))
                    else:
                        PathLog.debug('... processed edges[{}] vrtx-{}'.format(eei, vvi))

            # Sort segment angles
            if attached_edges_angle_tups:
                # Possible edge found
                attached_edges_angle_tups.sort(key=lambda tup: tup[0])
                if ei == self.trgt_idx or True:
                    PathLog.debug('attached_edges_angle_tups Edges[{}]: {}'.format(ei, attached_edges_angle_tups))

                next_seg = self._identify_next_edge(ei, vect_ang, attached_edges_angle_tups)
                (ang, ei, vi) = next_seg

                # Stop while search loop if connection returns to initial edge
                if ei == first_edge_idx:
                    if self.not_processed[ei]:
                        self._register_edge(ei)
                    PathLog.debug('break --- ei == first_edge_idx')
                    self.found_outer_loop = True
                    break

                self._register_edge(ei)
                PathLog.debug('... Saving connection to Edges[{}] v-{}'.format(ei, vi))
            else:
                PathLog.debug('BREAK - empty attached_edges_angle_tups - E[{}], vi-{}'.format(ei, cvi))
                break
            cont -= 1
        # Ewhile

        wire = Part.Wire(self.outer_edges)
        self.outer_wires.append(wire)
        self.outer_wires_indexes.append(self.outer_edges_indexes)

    # Public methods
    def get_projected_face(self):
        """get_projected_face()...
        This is the public method called to execute
        the outer wire identification algorithm and return a face
        created by the edges thereof, if able to do so."""
        PathLog.debug('get_projected_face()')

        grp = self._get_refined_projection()
        if grp:
            if self.edges:
                # Confirm seed point
                if not self.seed_point:
                    self._make_seed_point(self.edges)

                    # Populate coordinate data and find nodes
                    self._populate_coordinate_lists()
                    # Identify attachments between edges
                    self._identify_attachments()

                    # Process un-matched vertexes (repair gaps)
                    if self.no_matches:
                        self._make_missing_edge()

                    # Update count registers
                    self.node_cnt = len(self.node_data)
                    self.edges_cnt = len(self.edges)

                    # Find outer loop from provided set of edges
                    self._find_outer_edges()
                    
                    # Flag all edges attached to outer edges as processed
                    for idx in self.outer_edges_indexes:
                        for tup in self.atch_vert_0[idx]:
                            self.not_processed[tup[0]] = False
                        for tup in self.atch_vert_1[idx]:
                            self.not_processed[tup[0]] = False

                    self.remaining_edges_indexes = [i for i in range(0, self.edges_cnt) if self.not_processed[i]]
                    self.remaining_edges = [self.edges[i] for i in self.remaining_edges_indexes]
                # Eif
            else:
                if self.outer_wires:
                    self.projection_face = Part.Face(self.outer_wires[0])
                    PathLog.debug('Closed wire found.')

        if self.found_outer_loop:
            if self.projection_face:
                return self.projection_face
            elif self.outer_wires:
                return Part.Face(self.outer_wires[0])
            elif self.outer_edges:
                wire = Part.Wire(self.outer_edges)
                return Part.Face(wire)

        return False

    def get_outer_edges(self):
        """get_outer_edges()...
        This is more of a debug method to retrieve the incomplete list
        of outer edges."""
        PathLog.debug('get_outer_edges()')

        if self.outer_edges:
            return self.outer_edges  # (self.outer_edges, self.outer_edges_indexes)
        return False
# Eclass


class RefineProjectionEdges():
    '''RefineProjectionEdges(edge_list) class
    This class receives a list of edges, and removes duplicate
    and orphaned edges from that list.
    The return value is the refined list of edges.'''
    
    def __init__(self, edge_list):
        self.edge_list = edge_list
        self.edge_cnt = len(edge_list)
        self.edge_data = None
        self.active = None  # edge-specific active flags
        self.connected_indexes = None
        self.detached_indexes = None
        self.raw_node_data = None  # Three or more edges connect at single vertex

        self._populate_coordinate_lists()
        # refine edge list
        if self.edge_data:
            # unique_edges = self._filter_out_duplicate_edges(other_list)
            self.connected_indexes, self.detached_indexes = self._get_connected_edges_indexes()

    # Private methods
    def _populate_coordinate_lists(self):
        data = list()
        active = list()
        precision = 4

        # extract edge data into list for reference
        for i in range(0, len(self.edge_list)):
            e = self.edge_list[i]
            v0x = round(e.Vertexes[0].X, precision)
            v0y = round(e.Vertexes[0].Y, precision)
            # v0z = round(e.Vertexes[0].Z, precision)
            v1x = round(e.Vertexes[1].X, precision)
            v1y = round(e.Vertexes[1].Y, precision)
            # v1z = round(e.Vertexes[1].Z, precision)
            p0 = 'x{}_y{}'.format(v0x, v0y)
            p1 = 'x{}_y{}'.format(v1x, v1y)
            tup = (p0, p1, i)
            # Assign data based on characteristics
            data.append(tup)
            active.append(2)  # populate flags

        # Save detail list data
        self.active = active
        self.edge_data = data

    def _filter_out_duplicate_edges(self, coord_list):
        sort_list = list()
        coord_list.sort(key=lambda t: t[0])
        # process first edge in list
        last = coord_list[0]
        sort_list.append(last)

        for i in range(1, len(coord_list)):
            tup = coord_list[i]
            save = True
            # Check if current edge is same as last, and lengths same
            # ***** OPPOSITE SEMI-CIRCLES WILL FAIL ****** Need to compare mid-points also.
            if tup[0] == last[0] and tup[1] == last[1]:
                if self._is_same_length(tup[2], last[2]):
                    save = False  # same edge
            elif tup[0] == last[1] and tup[1] == last[0]:
                if self._is_same_length(tup[2], last[2]):
                    save = False  # same edge

            if save:
                sort_list.append(tup)
                last = tup
                self.active[tup[2]] = True

        return sort_list

    def _is_same_length(self, idx1, idx2):
        len1 = round(self.edge_list[idx1].Length, 6)
        len2 = round(self.edge_list[idx2].Length, 6)
        if len1 ==  len2:
            return True
        return False

    def _get_connected_edges_indexes(self):
        # no_match_idxs = list()
        nodes = list()
        data = list()
        attached = list()
        detached = list()
        active = self.active

        # prepare source edge data
        for (p0, p1, i) in self.edge_data:
            data.append((p0, p1, i))
            data.append((p1, p0, i))
        data.sort(key=lambda t: t[0])

        stop = len(data)
        i = 0
        while i < stop:
            p0 = data[i]
            match_cnt = 0
            matching = [p0[2]]  # Track matching edge indexes

            for n in range(i + 1, stop):
                p1 = data[n]
                if p0[0] == p1[0]:
                    match_cnt += 1  # increment match count
                    matching.append(p1[2])  # add matching edge index
                else:
                    break

            if match_cnt:
                # Test for node point
                if match_cnt > 2:
                    nodes.append((p0[0], p0[2], matching))
                    # this is node point
                # advance index to next item after matches
                i += match_cnt + 1
            else:
                # no match: save index and go to next item
                # no_match_idxs.append(i)
                active[p0[2]] -= 1
                i += 1
        # Ewhile

        # Save node data
        self.raw_node_data = nodes

        # Complie attached and detached edge index lists
        for i in range(0, self.edge_cnt):
            if active[i] > 0:
                attached.append(i)
            else:
                detached.append(i)

        return (attached, detached)

    # Public method
    def get_refined_edges(self):
        edges = self.edge_list
        return ([edges[i] for i in self.connected_indexes], [edges[i] for i in self.detached_indexes])
# Eclass

# Independent support functions used in classes above
def swap_index(i):
    if i == 1:
        return 0
    return 1

def vector_to_degrees(vector):
    ang = round(math.degrees(math.atan2(vector.y, vector.x)), 8)
    if ang < 0.0:
        ang += 360.0
    return ang
