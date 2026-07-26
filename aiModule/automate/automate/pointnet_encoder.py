from torch.nn import Module
from automate import LinearBlock
import torch

class PointNetEncoder(Module):
    def __init__(self, K=3, layers=(64, 64, 64, 128, 1024)):
        super().__init__()
        self.encode = LinearBlock(K, *layers)
        self.K = K
    def forward(self, pc):
        pc2 = pc.reshape(-1, self.K)
        x = self.encode(pc2)
        x = x.reshape(*pc.shape[:-1], -1)
        x_p = torch.max(x, dim=-2)[0]
        return x, x_p