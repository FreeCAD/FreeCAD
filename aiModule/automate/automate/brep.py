import torch_geometric as tg
import torch
import numpy as np
from dotmap import DotMap
from .conversions import torchify
from torch_geometric.data import Batch
from automate_cpp import Part, PartOptions
import json
import os
import xxhash
import torch_scatter

class PartDataset(torch.utils.data.Dataset):
    def __init__(
        self,
        splits_path,
        data_dir,
        mode = 'train',
        cache_dir = None,
        part_options = None,
        graph_options = None
    ):
        super().__init__()
        self.splits_path = splits_path
        self.mode = mode
        self.cache_dir = cache_dir
        self.data_dir = data_dir

        if self.cache_dir is not None:
            os.makedirs(os.path.join(self.cache_dir, self.mode), exist_ok=True)
        
        with open(splits_path, 'r') as f:
            splits = json.load(f)
        self.part_paths = splits[self.mode]

        self.options = PartOptions() if part_options is None else part_options

        self.features = PartFeatures() if graph_options is None else graph_options
    
    def __getitem__(self, idx):
        if isinstance(idx, slice):
            return [self[i] for i in range(len(self.part_paths))[idx]]
        if not self.cache_dir is None:
            cache_file = os.path.join(self.cache_dir, self.mode, f'{idx}.pt')
            if os.path.exists(cache_file):
                return torch.load(cache_file)
        part_path = os.path.join(self.data_dir, self.part_paths[idx])
        if part_path.endswith('.pt'):
            part = torch.load(part_path)
        else:
            part = Part(part_path, self.options)
        graph = part_to_graph(part, self.features)
        if not self.cache_dir is None:
            torch.save(graph, cache_file)
        return graph

    def __len__(self):
        return len(self.part_paths)

# Convert a Part object into a pytorch-geometric compatible
# heterogeneous graph

# We use a custom torch geometry data object to implement heterogeneous
# graphs since pytorch geometric had not yet implemented them when
# this project was started

class PartFeatures:
    r"""
    Options for what to load into a BREP Graph Data Object
    """
    def __init__(self):
        
        # Topology Information
        self.brep = True # Include Topology Level Information

        self.face = FaceFeatures()
        self.loop = LoopFeatures()
        self.edge = EdgeFeatures()
        self.vertex = VertexFeatures()

        self.meta_paths = True # Face-Face edge set

        # Include Mesh Data and Relation to BREP topology
        self.mesh = True
        self.mesh_to_topology = True

        # Include Grid Samples
        self.default_num_samples = 10
        self.samples = True # Overrides other options
        self.face_samples = True
        self.normals = True
        self.edge_samples = True
        self.tangents = True

        # Include random samples
        self.random_samples = True
        self.num_uniform_samples = 10000
        self.uniform_samples = False

        # Part Level Data
        self.bounding_box = True
        self.volume = True
        self.center_of_gravity = True
        self.moment_of_inertia = True
        self.surface_area = True

        # Mating Coordinate Frames
        self.mcfs = True

def to_flat(x):
    r"""
    Ensure input is a flat torch float tensor
    """
    number = (float, int, bool)
    if isinstance(x, number):
        return torch.tensor(x).float().reshape((1,))
    if isinstance(x, np.ndarray):
        return torch.from_numpy(x.copy()).flatten().float()
    if torch.is_tensor(x):
        return x.float().flatten()

def to_index(x):
    r"""
    Ensure input is a torch long tensor (necessary for edge arrays)
    """
    if isinstance(x, int):
        return torch.tensor(x).long().reshape((1,1))
    if isinstance(x, np.ndarray):
        return torch.from_numpy(x.copy()).long()
    if torch.is_tensor(x):
        return x.long()

def pad_to(tensor, length):
    if isinstance(tensor, list):
        return pad_to(torch.tensor(tensor), length)
    return torch.cat([tensor, torch.zeros(length - tensor.size(0))]).float()

FACE_PARAM_SIZE = 11 # The maximum parameter size for a face is 8
                    # so we will pad all parameter arrays to size 8
class FaceFeatures:
    r"""
    Options for which features to load for each BREP Face
    """
    def __init__(self):
        
        self.parametric_function = True
        self.parameter_values = True
        self.exclude_origin = False

        self.orientation = True

        self.surface_area = True
        self.circumference = True


        self.bounding_box = True
        self.na_bounding_box = True
        self.center_of_gravity = True
        self.moment_of_inertia = True
    
    def size(self):
        s = 0
        if self.parametric_function:
            s += 13
            if self.parameter_values:
                s += FACE_PARAM_SIZE
        
                if self.exclude_origin:
                    s -= 3
        if self.orientation:
            s += 1
    
        if self.surface_area:
            s += 1
        if self.circumference:
            s += 1
        if self.bounding_box:
            s += 6
        if self.na_bounding_box:
            s += 15
        if self.center_of_gravity:
            s += 3
        if self.moment_of_inertia:
            s += 9

        return s



def featurize_face(f, options):
    if not isinstance(f, dict):
        f = DotMap(torchify(f))
    feature_parts = []
    if options.face.parametric_function:
        feature_parts.append(
            torch.nn.functional.one_hot(
                torch.tensor(f.function.value), 
                f.function.enum_size))
        if options.face.parameter_values:
            params = pad_to(f.parameters, FACE_PARAM_SIZE)
            if options.face.exclude_origin:
                params = params[3:]
            feature_parts.append(params)
    if options.face.orientation:
        feature_parts.append(to_flat(f.orientation))
    
    if options.face.surface_area:
        feature_parts.append(to_flat(f.surface_area))
    if options.face.circumference:
        feature_parts.append(to_flat(f.circumference))
    if options.face.bounding_box:
        feature_parts.append(to_flat(f.bounding_box))
    if options.face.na_bounding_box:
        feature_parts.append(to_flat(f.na_bounding_box))
    if options.face.center_of_gravity:
        feature_parts.append(to_flat(f.center_of_gravity))
    if options.face.moment_of_inertia:
        feature_parts.append(to_flat(f.moment_of_inertia))
    
    return torch.cat(feature_parts).flatten().float()


class LoopFeatures:
    r"""
    Options for which features to load for each BREP Loop
    """
    def __init__(self):
        self.type = True
        self.length = True
        self.na_bounding_box = True
        self.center_of_gravity = True
        self.moment_of_inertia = True
    
    def size(self):
        s = 0
        if self.type:
            s += 10
        if self.length:
            s += 1
        if self.na_bounding_box:
            s += 15
        if self.center_of_gravity:
            s += 3
        if self.moment_of_inertia:
            s += 9
        
        return s


def featurize_loop(l, options):
    if not isinstance(l, dict):
        l = DotMap(torchify(l))
    feature_parts = []
    if options.loop.type:
        feature_parts.append(
            torch.nn.functional.one_hot(
                torch.tensor(l.type.value), 
                l.type.enum_size))
    
    if options.loop.length:
        feature_parts.append(to_flat(l.length))
    if options.loop.na_bounding_box:
        feature_parts.append(to_flat(l.na_bounding_box))
    if options.loop.center_of_gravity:
        feature_parts.append(to_flat(l.center_of_gravity))
    if options.loop.moment_of_inertia:
        feature_parts.append(to_flat(l.moment_of_inertia))
    
    return torch.cat(feature_parts).flatten().float()


EDGE_PARAM_SIZE = 11

class EdgeFeatures:
    r"""
    Options for which features to load for each BREP Edge
    """
    def __init__(self):
        # Parametric Definition
        self.parametric_function = True
        self.parameter_values = True
        self.exclude_origin = False
        self.orientation = True

        self.t_range = True # Parametric Range

        # 3D Start and End and Mid Points
        self.start = True
        self.end = True
        self.mid_point = True

        self.length = True

        self.bounding_box = True
        self.na_bounding_box = True

        self.center_of_gravity = True
        self.moment_of_inertia = True
    
    def size(self):
        s = 0
        if self.parametric_function:
            s += 11
            if self.parameter_values:
                s += EDGE_PARAM_SIZE

                if self.exclude_origin:
                    s -= 3
        if self.orientation:
            s += 1
        if self.t_range:
            s += 2
        if self.length:
            s += 1
        if self.start:
            s += 3
        if self.end:
            s += 3
        if self.mid_point:
            s += 3
        if self.bounding_box:
            s += 6
        if self.na_bounding_box:
            s += 15
        if self.center_of_gravity:
            s += 3
        if self.moment_of_inertia:
            s += 9
        return s
        

def featurize_edge(e, options):
    if not isinstance(e, dict):
        e = DotMap(torchify(e))
    feature_parts = []
    if options.edge.parametric_function:
        feature_parts.append(
            torch.nn.functional.one_hot(
                torch.tensor(e.function.value), 
                e.function.enum_size))
        if options.edge.parameter_values:
            params = pad_to(e.parameters, EDGE_PARAM_SIZE)
            if options.edge.exclude_origin:
                params = params[3:]
            feature_parts.append(params)
    if options.edge.orientation:
        feature_parts.append(to_flat(e.orientation))
    
    if options.edge.t_range:
        feature_parts.append(to_flat(e.t_range))

    if options.edge.length:
        feature_parts.append(to_flat(e.length))

    if options.edge.start:
        feature_parts.append(to_flat(e.start))
    if options.edge.end:
        feature_parts.append(to_flat(e.end))
    if options.edge.mid_point:
        feature_parts.append(to_flat(e.mid_point))
    
    if options.edge.bounding_box:
        feature_parts.append(to_flat(e.bounding_box))
    if options.edge.na_bounding_box:
        feature_parts.append(to_flat(e.na_bounding_box)) # TODO - does this actually contain everything?
    if options.edge.center_of_gravity:
        feature_parts.append(to_flat(e.center_of_gravity))
    if options.edge.moment_of_inertia:
        feature_parts.append(to_flat(e.moment_of_inertia))
    
    return torch.cat(feature_parts).flatten().float()


class VertexFeatures:
    r"""
    Options for which features to load for each BREP Vertex
    """
    def __init__(self):
        self.position = True

    def size(self):
        return 3 if self.position else 0

def featurize_vert(v, options):
    if not isinstance(v, dict):
        v = DotMap(torchify(v))
    feature_parts = []
    if options.vertex.position:
        feature_parts.append(to_flat(v.position))
    return torch.cat(feature_parts).flatten().float()


def flatbatch(datalist):
    """Batch HetData graphs while retaining AutoMate's flat tensor layout.

    PyG 2.x stores user attributes in its internal store, so ``dir(batch)`` no
    longer enumerates them reliably.  ``batch.keys()`` is the supported API.
    """
    follow_batch = []
    if hasattr(datalist[0], 'mcfs'):
        follow_batch.append('mcfs')
    batch = Batch.from_data_list(datalist, follow_batch=follow_batch)
    data = HetData()
    for key in batch.keys():
        if not key.endswith('_batch') and key not in {'batch', 'ptr'}:
            val = batch[key]
            if isinstance(val, torch.Tensor):
                setattr(data, key, val)
    # Copy these containers because the batching-only mappings below must not
    # mutate metadata held by cached single-part graphs.
    data.__edge_sets__ = dict(datalist[0].__edge_sets__)
    if hasattr(batch, 'mcfs_batch'):
        data.mcf_to_graph_idx = batch.mcfs_batch.expand((1, batch.mcfs.shape[0]))
        data.__edge_sets__['mcf_to_graph_idx'] = ['graph_idx']
    data.__num_nodes__ = batch.num_nodes
    data.__node_sets__ = set(datalist[0].__node_sets__)
    data.__edge_sets__['flat_topos_to_graph_idx'] = ['graph_idx']
    return data


def part_to_graph(part, options):
    # Add dot (.) access to deserialized parts so they act more like C++ module Parts
    if isinstance(part, dict):
        part = DotMap(part)
    
    data = HetData()

    # Keep track of which graph is which during batching
    data.graph_idx = to_index(0)

    # It is useful to have a unified "topology" node set for referencing
    # arbitrary topological entities against. We essentially "stack"
    # The 4 node types in this order [faces, edges, vertices, loops]
    # Note that this is different than the normal order, for compatibility
    # with older versions that did not consider loops
    # Compute the number of each type of node, and the offsets for their
    # indices in the global topology list
    n_faces = len(part.brep.nodes.faces)
    n_edges = len(part.brep.nodes.edges)
    n_vertices = len(part.brep.nodes.vertices)
    n_loops = len(part.brep.nodes.loops)
    data.n_faces = torch.tensor(n_faces).long()
    data.n_edges = torch.tensor(n_edges).long()
    data.n_vertices = torch.tensor(n_vertices).long()
    data.n_loops = torch.tensor(n_loops).long()

    n_topos = n_faces + n_edges + n_vertices + n_loops
    face_offset = 0
    edge_offset = n_faces
    vertex_offset = edge_offset + n_edges
    loop_offset = vertex_offset + n_vertices
    topo_offsets = [face_offset, edge_offset, vertex_offset, loop_offset]

    # Setup Node Data
    if options.brep:
        face_features = [featurize_face(f, options) for f in part.brep.nodes.faces]
        loop_features = [featurize_loop(f, options) for f in part.brep.nodes.loops]
        edge_features = [featurize_edge(f, options) for f in part.brep.nodes.edges]
        vert_features = [featurize_vert(f, options) for f in part.brep.nodes.vertices]


        data.face_export_ids = torch.tensor([xxhash.xxh32(f.export_id).intdigest() for f in part.brep.nodes.faces]).long()
        data.loop_export_ids = torch.tensor([xxhash.xxh32(l.export_id).intdigest() for l in part.brep.nodes.loops]).long()
        data.edge_export_ids = torch.tensor([xxhash.xxh32(e.export_id).intdigest() for e in part.brep.nodes.edges]).long()
        data.vertex_export_ids = torch.tensor([xxhash.xxh32(v.export_id).intdigest() for v in part.brep.nodes.vertices]).long()

        data.__node_sets__.add('face_export_ids')
        data.__node_sets__.add('loop_export_ids')
        data.__node_sets__.add('edge_export_ids')
        data.__node_sets__.add('vertex_export_ids')

        data.faces = torch.stack(face_features) if face_features else torch.empty((0, options.face.size()), dtype=torch.float)
        data.__node_sets__.add('faces')
        data.loops = torch.stack(loop_features) if loop_features else torch.empty((0, options.loop.size()), dtype=torch.float)
        data.edges = torch.stack(edge_features) if edge_features else torch.empty((0, options.edge.size()), dtype=torch.float)
        data.vertices = torch.stack(vert_features) if vert_features else torch.empty((0, options.vertex.size()), dtype=torch.float)

        data.face_to_loop = to_index(part.brep.relations.face_to_loop)
        data.__edge_sets__['face_to_loop'] = ['faces', 'loops']
        data.loop_to_edge = to_index(part.brep.relations.loop_to_edge)
        data.__edge_sets__['loop_to_edge'] = ['loops', 'edges']
        data.edge_to_vertex = to_index(part.brep.relations.edge_to_vertex)
        data.__edge_sets__['edge_to_vertex'] = ['edges', 'vertices']

        if options.meta_paths:
            data.face_to_face = to_index(part.brep.relations.face_to_face)
            data.__edge_sets__['face_to_face'] = ['faces', 'faces', 'edges']
        
        # Add links to the flattened topology list
        data.flat_topos = torch.empty((n_topos,0)).float()
        data.num_nodes = n_topos
        data.flat_topos_to_graph_idx = torch.zeros((1,n_topos)).long()
        data.__edge_sets__['flat_topos_to_graph_idx'] = ['graph_idx']
        
        data.face_to_flat_topos = torch.stack([
            torch.arange(n_faces).long(),
            torch.arange(n_faces).long() + face_offset
        ])
        data.__edge_sets__['face_to_flat_topos'] = ['faces', 'flat_topos']
        
        data.edge_to_flat_topos = torch.stack([
            torch.arange(n_edges).long(),
            torch.arange(n_edges).long() + edge_offset
        ])
        data.__edge_sets__['edge_to_flat_topos'] = ['edges', 'flat_topos']

        data.loop_to_flat_topos = torch.stack([
            torch.arange(n_loops).long(),
            torch.arange(n_loops).long() + loop_offset
        ])
        data.__edge_sets__['loop_to_flat_topos'] = ['loops', 'flat_topos']

        data.vertex_to_flat_topos = torch.stack([
            torch.arange(n_vertices).long(),
            torch.arange(n_vertices).long() + vertex_offset
        ])
        data.__edge_sets__['vertex_to_flat_topos'] = ['vertices', 'flat_topos']
    
    if options.mesh:
        data.V = torchify(part.mesh.V).float()
        data.F = torchify(part.mesh.F).long().T
        data.__edge_sets__['F'] = ['V','V','V']

        num_faces = data.F.size(1)

        # Only make edges into the brep if we have one
        if options.mesh_to_topology and options.brep:
            data.F_to_faces = torchify(part.mesh_topology.face_to_topology).long().reshape((1,-1))
            data.__edge_sets__['F_to_faces'] = ['faces']
            
            # mesh_topology.edge_to_topology is formatted as a #Fx3 matrix
            # that can contain -1 wherever there isn't a correspondence (e.g. mesh
            # edges in the centers of topological faces). We need to remove these
            # and format it as an edge set
            # We do this by exanding out into a 3 x (3x #faces) tensor with
            # face indices and positions explicit, then filter out the -1s
            edge_to_topo = torchify(
                part.mesh_topology.edge_to_topology).long().flatten()
            face_idx = torch.arange(num_faces).repeat_interleave(3)
            idx_in_face = torch.tensor([0,1,2]).long().repeat(num_faces)
            E_to_edges = torch.stack([face_idx, idx_in_face, edge_to_topo])
            E_to_edges = E_to_edges[:, (E_to_edges[2] != -1)]
            data.E_to_edges = E_to_edges
            data.__edge_sets__['E_to_edges'] = ['F', 3, 'faces']
            
            # Similar story with vetrices
            vert_to_topo = torchify(
                part.mesh_topology.point_to_topology).long().flatten()
            vert_indices = torch.arange(vert_to_topo.size(0))
            V_to_vertices = torch.stack([vert_indices, vert_to_topo])
            data.V_to_vertices = V_to_vertices[:, (V_to_vertices[1] != -1)]
            data.__edge_sets__['V_to_vertices'] = ['V', 'vertices']



    # Setup Part-Level Data
    part_feature_list = []
    if options.volume:
        part_feature_list.append(to_flat(part.summary.volume))
    if options.surface_area:
        part_feature_list.append(to_flat(part.summary.surface_area))
    if options.center_of_gravity:
        part_feature_list.append(to_flat(part.summary.center_of_gravity))
    if options.bounding_box:
        part_feature_list.append(to_flat(part.summary.bounding_box))
    if options.moment_of_inertia:
        part_feature_list.append(to_flat(part.summary.moment_of_inertia.flatten()))
    
    if len(part_feature_list) > 0:
        part_feature = torch.cat(part_feature_list).reshape((1,-1))

        data.part_feat = part_feature

    # Setup Samples
    if options.samples:
        if options.face_samples:
            samples = part.samples.face_samples
            if isinstance(samples, list):
                samples = torchify(samples).float()
            # Only use normals if the part object has them
            has_normals = (samples.size(1) == 9)
            if has_normals and not options.normals:
                samples = samples[:,[0,1,2,8],:,:]
            data.face_samples = samples
            data.__node_sets__.add('face_samples')
        if options.edge_samples:
            samples = part.samples.edge_samples
            if isinstance(samples, list):
                if samples:
                    samples = torchify(samples).float()
                else:
                    samples = torch.empty((0, 7, options.default_num_samples))
            # Only use tangents if the part object has them
            has_tangents = (samples.size(1) == 7)
            if has_tangents and not options.tangents:
                samples = samples[:,:3,:]
            data.edge_samples = samples
    
    # Setup Random Samples
    if options.random_samples:
        random_samples = part.random_samples.samples
        if options.uniform_samples:
            surface_areas = torch.tensor([face.surface_area for face in part.brep.nodes.faces])
            face_choices = torch.multinomial(surface_areas, options.num_uniform_samples, replacement=True)
            face_counts = torch_scatter.scatter_add(torch.ones(face_choices.shape, dtype=torch.long), face_choices, dim_size=len(surface_areas))
            
            all_samples = []
            for sample, count in zip(random_samples, face_counts):
                sample = torch.from_numpy(sample).float()
                sample_filtered = sample[sample[:,6] > 0.5,:6]
                if count < sample_filtered.shape[0]:
                    sample_filtered = sample_filtered[:count]
                all_samples.append(sample_filtered)
            data.uniform_samples = torch.vstack(all_samples).float()
        else:
            if isinstance(random_samples, list):
                random_samples = torchify(random_samples)
            data.random_samples = random_samples

    # Setup MCFs
    if options.mcfs:
        mcf_origins = []
        mcf_axes = []
        mcf_refs = []
        for mcf in part.default_mcfs:
            mcf_origins.append(mcf.origin)
            mcf_axes.append(mcf.axis)

            # torchify computes the size of enums, which we want for 
            # one-hot encoding
            axis_ref = mcf.ref.axis_ref
            origin_ref = mcf.ref.origin_ref

            axis_tt = axis_ref.reference_type.value
            axis_ti = axis_ref.reference_index + topo_offsets[axis_tt]

            origin_tt = origin_ref.reference_type.value
            origin_ti = origin_ref.reference_index + topo_offsets[origin_tt]
            origin_it = origin_ref.inference_type.value

            mcf_refs.append([axis_ti, origin_ti, origin_it])

        if len(mcf_axes) > 0 and isinstance(mcf_axes[0], np.ndarray):
            mcf_axes = torch.from_numpy(np.stack(mcf_axes))
            mcf_origins = torch.from_numpy(np.stack(mcf_origins))
        else:
            mcf_axes = torch.stack(mcf_axes)
            mcf_origins = torch.stack(mcf_origins)
        data.mcfs = torch.cat([
            mcf_axes, 
            mcf_origins],1).float()
        
        data.mcf_refs = torch.tensor(mcf_refs).long().T
        data.__edge_sets__['mcf_refs'] = ['flat_topos','flat_topos',0]

    return data


class HetData(tg.data.Data):
    r"""
    An extension of pytorch-geometric's data objects that allows easier
    configuration of heterogeneous graphs. Set __edge_sets__ to be a dictionary
    where the keys are strings of the edge attribute names (e.g. 'edge_index'),
    and the values are the string names of the node data tensors for the
    src and dst sides of each edge. Optionally set __node_sets__ to contain
    names of node sets (useful for overriding pygeo defaults like 'faces')
    """

    def __init__(self):
        super().__init__()
        self.__edge_sets__ = {}
        self.__node_sets__ = set()

    def __inc__(self, key, value, *args, **kwargs):
        if key in self.__edge_sets__:
            def get_sizes(nodes):
                if isinstance(nodes, int):
                    return nodes
                if isinstance(nodes, str):
                    if nodes in self.__edge_sets__:
                        return self[nodes].size(1)
                    return self[nodes].size(0)
                if isinstance(nodes, list) or isinstance(nodes, tuple):
                    return torch.tensor([[get_sizes(x)] for x in nodes])

            return get_sizes(self.__edge_sets__[key])
        if key in self.__node_sets__:
            return 0
        return super().__inc__(key, value)

    def __cat_dim__(self, key, value, *args, **kwargs):
        if key in self.__edge_sets__:
            return 1
        elif key in self.__node_sets__:
            return 0
        return super().__cat_dim__(key, value)
