#/***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

__title__="BOPTools.GeneralFuseResult module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecad.org"
__doc__ = "Implementation of GeneralFuseResult class, which parses return of generalFuse."

import Part
from .Utils import HashableShape, HashableShape_Deep, FrozenClass


class GeneralFuseResult(FrozenClass):
    """class GeneralFuseResult: helper object for obtaining info from results of
    Part.Shape.generalFuse() method.

Usage:
def myCustomFusionRoutine(list_of_shapes):
    generalFuse_return = list_of_shapes[0].generalFuse(list_of_shapes[1:])
    ao = GeneralFuseResult(list_of_shapes, generalFuse_return)
    ... (use attributes and methods of ao) ..."""

    def __define_attributes(self):
        # stores the data returned by generalFuse, supplied to class constructor
        self.gfa_return = None

        # pieces that resulted from intersetion routine. List of shapes (non-decorated).
        self.pieces = None
        # key = decorated shape. Value = index (int) into self.pieces
        self._piece_to_index = {}

        # list of shapes that was supplied to generalFuse (plus the self-shape). List of shapes (non-decorated)
        self.source_shapes = []
        # key = decorated shape. Value = index (int) into self.source_shapes
        self._source_to_index = {}

        #list of pieces (indexes) generated from a source shape, by index of source shape. List of lists of ints.
        self._pieces_of_source = []
        #list of source shapes (indexes) the piece came from, by index of piece. List of lists of ints.
        self._sources_of_piece = []

        # dictionary for finding, which source shapes did an element of pieces come from.
        # key = HashableShape (element). Value = set of ints
        self._element_to_source = {}

        self._freeze()

    def __init__(self, source_shapes, generalFuse_return):
        self.__define_attributes()

        self.gfa_return = generalFuse_return
        self.source_shapes = source_shapes
        self.parse()

    def parse(self):
        """Parses the result of generalFuse recorded into self.gfa_return. Recovers missing
        information. Fills in data structures.

        It is called automatically by class constructor."""

        #save things to be parsed and wipe out all other data
        gfa_return = self.gfa_return
        source_shapes = self.source_shapes
        self.__define_attributes()
        self.gfa_return = gfa_return
        self.source_shapes = source_shapes
        # and start filling in data structures...

        compound, map = self.gfa_return
        self.pieces = compound.childShapes()

        # create piece shape index
        for iPiece in range(len(self.pieces)):
            ha_piece = HashableShape(self.pieces[iPiece])
            if not ha_piece in self._piece_to_index:
                self._piece_to_index[ha_piece] = iPiece
            else:
                raise ValueError("GeneralFuseAnalyzer.parse: duplicate piece shape detected.")
        # create source shape index
        for iSource in range(len(self.source_shapes)):
            ha_source = HashableShape(self.source_shapes[iSource])
            if not ha_source in self._source_to_index:
                self._source_to_index[ha_source] = iSource
            else:
                raise ValueError("GeneralFuseAnalyzer.parse: duplicate source shape detected.")

        #test if map has missing entries
        map_needs_repairing = False
        for iSource in range(len(map)):
            if len(map[iSource]) == 0:
                map_needs_repairing = True

        if map_needs_repairing:
            aggregate_types = set(["Wire","Shell","CompSolid","Compound"])
            nonaggregate_types = set(["Vertex","Edge","Face","Solid"])

            types = set()
            for piece in self.pieces:
                types.add(piece.ShapeType)

            types_to_extract = types.intersection(nonaggregate_types)

            def extractor(sh):
                return (
                    (sh.Vertexes if "Vertex" in types_to_extract else []) +
                    (sh.Edges if "Edge" in types_to_extract else []) +
                    (sh.Faces if "Face" in types_to_extract else []) +
                    (sh.Solids if "Solid" in types_to_extract else [])
                )

            aggregate_sources_indexes = [self.indexOfSource(sh) for sh in self.source_shapes if sh.ShapeType in aggregate_types]
            aggregate_pieces = [sh for sh in self.pieces if sh.ShapeType in aggregate_types]
            assert(len(aggregate_sources_indexes) == len(aggregate_pieces))
            for i_aggregate in range(len(aggregate_sources_indexes)):
                iSource = aggregate_sources_indexes[i_aggregate]
                if len(map[iSource]) == 0:#recover only if info is actually missing
                    map[iSource] = [aggregate_pieces[i_aggregate]]
                    #search if any plain pieces are also in this aggregate piece. If yes, we need to add the piece to map.
                    for sh in extractor(aggregate_pieces[i_aggregate]):
                        hash = HashableShape(sh)
                        iPiece = self._piece_to_index.get(hash)
                        if iPiece is not None:
                            #print "found piece {num} in compound {numc}".format(num= iPiece, numc= i_aggregate)
                            if not map[iSource][-1].isSame(self.pieces[iPiece]):
                                map[iSource].append(self.pieces[iPiece])

            # check the map was recovered successfully
            for iSource in range(len(map)):
                if len(map[iSource]) == 0:
                    import FreeCAD as App
                    App.Console.PrintWarning("Map entry {num} is empty. "
                                             "Source-to-piece correspondence information is probably incomplete.".format(num=iSource))

        self._pieces_of_source = [[] for i in range(len(self.source_shapes))]
        self._sources_of_piece = [[] for i in range(len(self.pieces))]
        assert(len(map) == len(self.source_shapes))
        for iSource in range(len(self.source_shapes)):
            list_pieces = map[iSource]
            for piece in list_pieces:
                iPiece = self.indexOfPiece(piece)
                self._sources_of_piece[iPiece].append(iSource)
                self._pieces_of_source[iSource].append(iPiece)

    def parse_elements(self):
        """Fills element-to-source map. Potentially slow, so separated from general parse.
        Needed for splitAggregates; called automatically from splitAggregates."""

        if len(self._element_to_source)>0:
            return #already parsed.

        for iPiece in range(len(self.pieces)):
            piece = self.pieces[iPiece]
            for element in piece.Vertexes + piece.Edges + piece.Faces + piece.Solids:
                el_h = HashableShape(element)
                if el_h in self._element_to_source:
                    self._element_to_source[el_h].update(set(self._sources_of_piece[iPiece]))
                else:
                    self._element_to_source[el_h] = set(self._sources_of_piece[iPiece])

    def indexOfPiece(self, piece_shape):
        "indexOfPiece(piece_shape): returns index of piece_shape in list of pieces"
        return self._piece_to_index[HashableShape(piece_shape)]
    def indexOfSource(self, source_shape):
        "indexOfSource(source_shape): returns index of source_shape in list of arguments"
        return self._source_to_index[HashableShape(source_shape)]

    def piecesFromSource(self, source_shape):
        """piecesFromSource(source_shape): returns list of pieces (shapes) that came from
        given source shape.

        Note: aggregate pieces (e.g. wire, shell, compound) always have only one source - the
        shape they came directly from. Only after executing splitAggregates and
        explodeCompounds the source lists become completely populated."""

        ilist = self._pieces_of_source[self.indexOfSource(source_shape)]
        return [self.pieces[i] for i in ilist]

    def sourcesOfPiece(self, piece_shape):
        """sourcesOfPiece(piece_shape): returns list of source shapes given piece came from.

        Note: aggregate pieces (e.g. wire, shell, compound) always have only one source - the
        shape they came directly from. Only after executing splitAggregates and
        explodeCompounds the source lists become completely populated."""

        ilist = self._sources_of_piece[self.indexOfPiece(piece_shape)]
        return [self.source_shapes[i] for i in ilist]

    def largestOverlapCount(self):
        """largestOverlapCount(self): returns the largest overlap count. For example, if three
        spheres intersect and have some volume common to all three, largestOverlapCount
        returns 3.

        Note: the return value may be incorrect if some of the pieces are wires/shells/
        compsolids/compounds. Please use explodeCompounds and splitAggregates before using this function."""

        return max([len(ilist) for ilist in self._sources_of_piece])

    def splitAggregates(self, pieces_to_split = None):
        """splitAggregates(pieces_to_split = None): splits aggregate shapes (wires, shells,
        compsolids) in pieces of GF result as cut by intersections. Also splits aggregates
        inside compounds. After running this, 'self' is replaced with new data, where the
        pieces_to_split are split.

        'pieces_to_split': list of shapes (from self.pieces), that are to be processed. If
        None, all pieces will be split if possible.

        Notes:
        * this routine is very important to functioning of Connect on shells and wires.
        * Warning: convoluted and slow."""

        if pieces_to_split is None:
            pieces_to_split = self.pieces
        pieces_to_split = [HashableShape(piece) for piece in pieces_to_split]
        pieces_to_split = set(pieces_to_split)

        self.parse_elements()
        new_data = GeneralFuseReturnBuilder(self.source_shapes)
        changed = False

        #split pieces that are not compounds....
        for iPiece in range(len(self.pieces)):
            piece = self.pieces[iPiece]

            if HashableShape(piece) in pieces_to_split:
                new_pieces = self.makeSplitPieces(piece)
                changed = changed or len(new_pieces)>1
                for new_piece in new_pieces:
                    new_data.addPiece(new_piece, self._sources_of_piece[iPiece])
            else:
                new_data.addPiece(piece, self._sources_of_piece[iPiece])

        #split pieces inside compounds
        #prepare index of existing pieces.
        existing_pieces = new_data._piece_to_index.copy()
        for i_new_piece in range(len(new_data.pieces)):
            new_piece = new_data.pieces[i_new_piece]
            if HashableShape(new_piece) in pieces_to_split:
                if new_piece.ShapeType == "Compound":
                    ret = self._splitInCompound(new_piece, existing_pieces)
                    if ret is not None:
                        changed = True
                        new_data.replacePiece(i_new_piece, ret)

        if len(new_data.pieces) > len(self.pieces) or changed:
            self.gfa_return = new_data.getGFReturn()
            self.parse()
        #else:
            #print "Nothing was split"

    def _splitInCompound(self, compound, existing_pieces):
        """Splits aggregates inside compound. Returns None if nothing is split, otherwise
        returns compound.
        existing_pieces is a dict. Key is deep hash. Value is tuple (int, shape). It is
        used to search for if this split piece was already generated, and re-use the old
        one."""

        changed = False
        new_children = []
        for piece in compound.childShapes():
            if piece.ShapeType == "Compound":
                subspl = self._splitInCompound(piece, existing_pieces)
                if subspl is None:
                    new_children.append(piece)
                else:
                    new_children.append(subspl)
                    changed = True
            else:
                new_pieces = self.makeSplitPieces(piece)
                changed = changed or len(new_pieces)>1
                for new_piece in new_pieces:
                    hash = HashableShape_Deep(new_piece)
                    dummy,ex_piece = existing_pieces.get(hash, (None, None))
                    if ex_piece is not None:
                        new_children.append(ex_piece)
                        changed = True
                    else:
                        new_children.append(new_piece)
                        existing_pieces[hash] = (-1, new_piece)
        if changed:
            return Part.Compound(new_children)
        else:
            return None


    def makeSplitPieces(self, shape):
        """makeSplitPieces(self, shape): splits a shell, wire or compsolid into pieces where
        it intersects with other shapes.

        Returns list of split pieces. If no splits were done, returns list containing the
        original shape."""

        if shape.ShapeType == "Wire":
            bit_extractor = lambda sh: sh.Edges
            joint_extractor = lambda sh: sh.Vertexes
        elif shape.ShapeType == "Shell":
            bit_extractor = lambda sh: sh.Faces
            joint_extractor = lambda sh: sh.Edges
        elif shape.ShapeType == "CompSolid":
            bit_extractor = lambda sh: sh.Solids
            joint_extractor = lambda sh: sh.Faces
        else:
            #can't split the shape
            return [shape]

        # for each joint, test if all bits it's connected to are from same number of sources.
        # If not, this is a joint for splitting
        # FIXME: this is slow, and maybe can be optimized
        splits = []
        for joint in joint_extractor(shape):
            joint_overlap_count = len(self._element_to_source[HashableShape(joint)])
            if joint_overlap_count > 1:
                # find elements in pieces that are connected to joint
                for bit in bit_extractor(self.gfa_return[0]):
                    for joint_bit in joint_extractor(bit):
                        if joint_bit.isSame(joint):
                            #bit is connected to joint!
                            bit_overlap_count = len(self._element_to_source[HashableShape(bit)])
                            assert(bit_overlap_count <= joint_overlap_count)
                            if bit_overlap_count < joint_overlap_count:
                                if len(splits) == 0 or splits[-1] is not joint:
                                    splits.append(joint)
        if len(splits)==0:
            #shape was not split - no split points found
            return [shape]

        from . import ShapeMerge

        new_pieces = ShapeMerge.mergeShapes(bit_extractor(shape), split_connections= splits, bool_compsolid= True).childShapes()
        if len(new_pieces) == 1:
            #shape was not split (split points found, but the shape remained in one piece).
            return [shape]
        return new_pieces

    def explodeCompounds(self):
        """explodeCompounds(): if any of self.pieces is a compound, the compound is exploded.
        After running this, 'self' is filled with new data, where pieces are updated to
        contain the stuff extracted from compounds."""

        has_compounds = False
        for piece in self.pieces:
            if piece.ShapeType == "Compound":
                has_compounds = True
        if not has_compounds:
            return

        from .Utils import compoundLeaves

        new_data = GeneralFuseReturnBuilder(self.source_shapes)
        new_data.hasher_class = HashableShape #deep hashing not needed here.

        for iPiece in range(len(self.pieces)):
            piece = self.pieces[iPiece]
            if piece.ShapeType == "Compound":
                for child in compoundLeaves(piece):
                    new_data.addPiece(child, self._sources_of_piece[iPiece])
            else:
                new_data.addPiece(piece, self._sources_of_piece[iPiece])

        self.gfa_return = new_data.getGFReturn()
        self.parse()


class GeneralFuseReturnBuilder(FrozenClass):
    "GeneralFuseReturnBuilder: utility class used by splitAggregates to build fake return of generalFuse, for re-parsing."
    def __define_attributes(self):
        self.pieces = []
        # key = hasher_class(shape). Value = (index_into_self_dot_pieces, shape).
        # Note that GeneralFuseResult uses this item directly.
        self._piece_to_index = {}

        self._pieces_from_source = [] #list of list of ints
        self.source_shapes = []

        self.hasher_class = HashableShape_Deep

        self._freeze()

    def __init__(self, source_shapes):
        self.__define_attributes()
        self.source_shapes = source_shapes
        self._pieces_from_source = [[] for i in range(len(source_shapes))]

    def addPiece(self, piece_shape, source_shape_index_list):
        """addPiece(piece_shape, source_shape_index_list): adds a piece. If the piece
        already exists, returns False, and only updates source<->piece map."""

        ret = False
        i_piece_existing = None
        hash = None
        if piece_shape.ShapeType != "Compound": # do not catch duplicate compounds
            hash = self.hasher_class(piece_shape)
            i_piece_existing, dummy = self._piece_to_index.get(hash, (None, None))

        if i_piece_existing is None:
            #adding
            self.pieces.append(piece_shape)
            i_piece_existing = len(self.pieces)-1
            if hash is not None:
                self._piece_to_index[hash] = (i_piece_existing, piece_shape,)
            ret = True
        else:
            #re-adding
            ret = False
        for iSource in source_shape_index_list:
            if not i_piece_existing in self._pieces_from_source[iSource]:
                self._pieces_from_source[iSource].append(i_piece_existing)
        return ret

    def replacePiece(self, piece_index, new_shape):
        assert(self.pieces[piece_index].ShapeType == "Compound")
        assert(new_shape.ShapeType == "Compound")
        self.pieces[piece_index] = new_shape

    def getGFReturn(self):
        return (Part.Compound(self.pieces), [[self.pieces[iPiece] for iPiece in ilist] for ilist in self._pieces_from_source])
