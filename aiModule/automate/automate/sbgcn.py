import torch
import torch_scatter
from torch.nn import Linear, Sequential, ModuleList, BatchNorm1d, Dropout, LeakyReLU, ReLU
import torch_geometric as tg
from .uvnet_encoders import UVNetCurveEncoder, UVNetSurfaceEncoder

"""
HetData(n_faces=22, n_edges=52, n_vertices=32, n_loops=22, faces=[22, 56], loops=[22, 38], edges=[52, 63], 
vertices=[32, 3], face_to_loop=[2, 22], loop_to_edge=[2, 104], edge_to_vertex=[2, 104], face_to_face=[3, 52], 
flat_topos=[128, 0], face_to_flat_topos=[2, 22], edge_to_flat_topos=[2, 52], loop_to_flat_topos=[2, 22], 
vertex_to_flat_topos=[2, 32], V=[72, 3], F=[3, 140], F_to_faces=[1, 140], E_to_edges=[3, 184], 
V_to_vertices=[2, 32], part_feat=[1, 20], face_samples=[22, 9, 10, 10], edge_samples=[52, 7, 10], mcfs=[246, 6], mcf_refs=[3, 246])
"""
class SBGCN(torch.nn.Module):
    def __init__(self,
        f_in_width,
        l_in_width,
        e_in_width,
        v_in_width,
        out_width,
        k,
        use_uvnet_features=False,
        crv_in_dim=[0, 1, 2, 3, 4, 5],
        srf_in_dim=[0, 1, 2, 3, 4, 5, 8],
        crv_emb_dim=64,
        srf_emb_dim=64,
        normalize_inputs=False,
        normalize_intermediate=False,
    ):
        super().__init__()
        self.normalize_inputs = normalize_inputs
        self.normalize_intermediate = normalize_intermediate
        self.use_uvnet_features = use_uvnet_features
        self.crv_in_dim = crv_in_dim
        self.srf_in_dim = srf_in_dim
        if use_uvnet_features:
            self.crv_emb_dim = crv_emb_dim
            self.srf_emb_dim = srf_emb_dim
            self.curv_encoder = UVNetCurveEncoder(
                in_channels=len(crv_in_dim), output_dims=crv_emb_dim
            )
            self.surf_encoder = UVNetSurfaceEncoder(
                in_channels=len(srf_in_dim), output_dims=srf_emb_dim
            )
            f_in_width += srf_emb_dim
            e_in_width += crv_emb_dim

        self.embed_f_in = LinearBlock(f_in_width, out_width)
        self.embed_l_in = LinearBlock(l_in_width, out_width)
        self.embed_e_in = LinearBlock(e_in_width, out_width)
        self.embed_v_in = LinearBlock(v_in_width, out_width)
        
        self.V2E = BipartiteResMRConv(out_width)
        self.E2L = BipartiteResMRConv(out_width)
        self.L2F = BipartiteResMRConv(out_width)
        
        self.ffLayers = ModuleList()
        for i in range(k):
            self.ffLayers.append(BipartiteResMRConv(out_width))
    
        self.F2L = BipartiteResMRConv(out_width)
        self.L2E = BipartiteResMRConv(out_width)
        self.E2V = BipartiteResMRConv(out_width)

    
    def forward(self, data):
        x_f = data.faces
        x_l = data.loops
        x_e = data.edges
        x_v = data.vertices

        if self.normalize_inputs:
            # B-Rep physical features span many orders of magnitude (for
            # example moments of inertia on imported parts). Scaling each
            # entity by its largest absolute feature prevents finite forward
            # values from producing infinite SB-GCN gradients.
            def safe_scale(values):
                scale = values.abs().amax(dim=1, keepdim=True).clamp_min(1.0)
                return values / scale

            x_f = safe_scale(x_f)
            x_l = safe_scale(x_l)
            x_e = safe_scale(x_e)
            x_v = safe_scale(x_v)


        # Compute uvnet features
        if self.use_uvnet_features:
            hidden_srf_feat = self.surf_encoder(data.face_samples[:,self.srf_in_dim,:,:])
            hidden_crv_feat = self.curv_encoder(data.edge_samples[:,self.crv_in_dim,:])
            x_f = torch.cat((x_f, hidden_srf_feat), dim=1)
            x_e = torch.cat((x_e, hidden_crv_feat), dim=1)


        # Apply Input Encoders
        x_f = self.embed_f_in(x_f)
        x_l = self.embed_l_in(x_l)
        x_e = self.embed_e_in(x_e)
        x_v = self.embed_v_in(x_v)

        def stabilize(values):
            if not self.normalize_intermediate:
                return values
            scale = values.detach().abs().amax(dim=1, keepdim=True).clamp_min(1.0)
            return values / scale


        # Upward Pass ([[1,0]] flips downwards graph edges)
        x_e = stabilize(self.V2E(x_v, x_e, data.edge_to_vertex[[1,0]]))
        x_l = stabilize(self.E2L(x_e, x_l, data.loop_to_edge[[1,0]]))
        x_f = stabilize(self.L2F(x_l, x_f, data.face_to_loop[[1,0]]))
        
        # Meta-Edge Spine
        for conv in self.ffLayers:
            x_f = stabilize(conv(x_f, x_f, data.face_to_face[:2,:]))
        
        # Downward Pass
        x_l = stabilize(self.F2L(x_f, x_l, data.face_to_loop))
        x_e = stabilize(self.L2E(x_l, x_e, data.loop_to_edge))
        x_v = stabilize(self.E2V(x_e, x_v, data.edge_to_vertex))
        
        # Flatten Topology Representations
        n_topos = x_f.size(0) + x_l.size(0) + x_e.size(0) + x_v.size(0)
        n_feats = x_f.size(1)
        x_t = torch.zeros((n_topos, n_feats)).type_as(x_f)
        
        x_t[data.face_to_flat_topos[1]] = x_f[data.face_to_flat_topos[0]]
        x_t[data.edge_to_flat_topos[1]] = x_e[data.edge_to_flat_topos[0]]
        x_t[data.vertex_to_flat_topos[1]] = x_v[data.vertex_to_flat_topos[0]]
        x_t[data.loop_to_flat_topos[1]] = x_l[data.loop_to_flat_topos[0]]

        # Global Pool
        x_p = tg.nn.global_max_pool(x_t, data.flat_topos_to_graph_idx.flatten())

        return x_t, x_p, x_f, x_l, x_e, x_v

class BipartiteResMRConv(torch.nn.Module):
    def __init__(self, width):
        super().__init__()
        self.mlp = LinearBlock(2*width, width)
    
    def forward(self, x_src, x_dst, e):
        diffs = torch.index_select(x_dst, 0, e[1]) - torch.index_select(x_src, 0, e[0])
        maxes, _ = torch_scatter.scatter_max(
            diffs, 
            e[1], 
            dim=0, 
            dim_size=x_dst.shape[0]
        )
        return x_dst + self.mlp(torch.cat([x_dst, maxes], dim=1))


class LinearBlock(torch.nn.Module):
    def __init__(self, *layer_sizes, batch_norm=False, dropout=0.0, last_linear=False, leaky=True):
        super().__init__()

        layers = []
        for i in range(len(layer_sizes) - 1):
            c_in = layer_sizes[i]
            c_out = layer_sizes[i + 1]

            layers.append(Linear(c_in, c_out))
            if last_linear and i+1 >= len(layer_sizes) - 1:
                break
            if batch_norm:
                layers.append(BatchNorm1d(c_out))
            if dropout > 0:
                layers.append(Dropout(p=dropout))
            layers.append((LeakyReLU() if leaky else ReLU()))

        self.f = Sequential(*layers)

    def forward(self, x):
        return self.f(x)
