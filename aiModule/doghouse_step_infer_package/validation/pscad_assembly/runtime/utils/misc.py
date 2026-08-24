import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import random
import torch
import torch.nn as nn
import torch.nn.functional as F
import os
from collections import abc
from pointnet2_ops import pointnet2_utils


def fps(data, number):
    '''
        data B N 3
        number int
    '''
    fps_idx = pointnet2_utils.furthest_point_sample(data, number) 
    fps_data = pointnet2_utils.gather_operation(data.transpose(1, 2).contiguous(), fps_idx).transpose(1,2).contiguous()
    return fps_data


def worker_init_fn(worker_id):
    np.random.seed(np.random.get_state()[1][0] + worker_id)

def build_lambda_sche(opti, config):
    if config.get('decay_step') is not None:
        lr_lbmd = lambda e: max(config.lr_decay ** (e / config.decay_step), config.lowest_decay)
        scheduler = torch.optim.lr_scheduler.LambdaLR(opti, lr_lbmd)
    else:
        raise NotImplementedError()
    return scheduler

def build_lambda_bnsche(model, config):
    if config.get('decay_step') is not None:
        bnm_lmbd = lambda e: max(config.bn_momentum * config.bn_decay ** (e / config.decay_step), config.lowest_decay)
        bnm_scheduler = BNMomentumScheduler(model, bnm_lmbd)
    else:
        raise NotImplementedError()
    return bnm_scheduler
    
def set_random_seed(seed, deterministic=False):
    """Set random seed.
    Args:
        seed (int): Seed to be used.
        deterministic (bool): Whether to set the deterministic option for
            CUDNN backend, i.e., set `torch.backends.cudnn.deterministic`
            to True and `torch.backends.cudnn.benchmark` to False.
            Default: False.

    # Speed-reproducibility tradeoff https://pytorch.org/docs/stable/notes/randomness.html
    if cuda_deterministic:  # slower, more reproducible
        cudnn.deterministic = True
        cudnn.benchmark = False
    else:  # faster, less reproducible
        cudnn.deterministic = False
        cudnn.benchmark = True

    """
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed_all(seed)
    if deterministic:
        torch.backends.cudnn.deterministic = True
        torch.backends.cudnn.benchmark = False


def is_seq_of(seq, expected_type, seq_type=None):
    """Check whether it is a sequence of some type.
    Args:
        seq (Sequence): The sequence to be checked.
        expected_type (type): Expected type of sequence items.
        seq_type (type, optional): Expected sequence type.
    Returns:
        bool: Whether the sequence is valid.
    """
    if seq_type is None:
        exp_seq_type = abc.Sequence
    else:
        assert isinstance(seq_type, type)
        exp_seq_type = seq_type
    if not isinstance(seq, exp_seq_type):
        return False
    for item in seq:
        if not isinstance(item, expected_type):
            return False
    return True


def set_bn_momentum_default(bn_momentum):
    def fn(m):
        if isinstance(m, (nn.BatchNorm1d, nn.BatchNorm2d, nn.BatchNorm3d)):
            m.momentum = bn_momentum
    return fn

class BNMomentumScheduler(object):

    def __init__(
            self, model, bn_lambda, last_epoch=-1,
            setter=set_bn_momentum_default
    ):
        if not isinstance(model, nn.Module):
            raise RuntimeError(
                "Class '{}' is not a PyTorch nn Module".format(
                    type(model).__name__
                )
            )

        self.model = model
        self.setter = setter
        self.lmbd = bn_lambda

        self.step(last_epoch + 1)
        self.last_epoch = last_epoch

    def step(self, epoch=None):
        if epoch is None:
            epoch = self.last_epoch + 1

        self.last_epoch = epoch
        self.model.apply(self.setter(self.lmbd(epoch)))

    def get_momentum(self, epoch=None):
        if epoch is None:
            epoch = self.last_epoch + 1
        return self.lmbd(epoch)



def seprate_point_cloud(xyz, num_points, crop, fixed_points = None, padding_zeros = False):
    '''
     seprate point cloud: usage : using to generate the incomplete point cloud with a setted number.
    '''
    _,n,c = xyz.shape

    assert n == num_points
    assert c == 3
    if crop == num_points:
        return xyz, None
        
    INPUT = []
    CROP = []
    for points in xyz:
        if isinstance(crop,list):
            num_crop = random.randint(crop[0],crop[1])
        else:
            num_crop = crop

        points = points.unsqueeze(0)

        if fixed_points is None:       
            center = F.normalize(torch.randn(1,1,3),p=2,dim=-1).cuda()
        else:
            if isinstance(fixed_points,list):
                fixed_point = random.sample(fixed_points,1)[0]
            else:
                fixed_point = fixed_points
            center = fixed_point.reshape(1,1,3).cuda()

        distance_matrix = torch.norm(center.unsqueeze(2) - points.unsqueeze(1), p =2 ,dim = -1)  # 1 1 2048

        idx = torch.argsort(distance_matrix,dim=-1, descending=False)[0,0] # 2048

        if padding_zeros:
            input_data = points.clone()
            input_data[0, idx[:num_crop]] =  input_data[0,idx[:num_crop]] * 0

        else:
            input_data = points.clone()[0, idx[num_crop:]].unsqueeze(0) # 1 N 3

        crop_data =  points.clone()[0, idx[:num_crop]].unsqueeze(0)

        if isinstance(crop,list):
            INPUT.append(fps(input_data,2048))
            CROP.append(fps(crop_data,2048))
        else:
            INPUT.append(input_data)
            CROP.append(crop_data)

    input_data = torch.cat(INPUT,dim=0)# B N 3
    crop_data = torch.cat(CROP,dim=0)# B M 3

    return input_data.contiguous(), crop_data.contiguous()

def get_ptcloud_img(ptcloud,roll,pitch):
    fig = plt.figure(figsize=(8, 8))

    x, z, y = ptcloud.transpose(1, 0)
    ax = fig.gca(projection=Axes3D.name, adjustable='box')
    ax.axis('off')
    # ax.axis('scaled')
    ax.view_init(roll,pitch)
    max, min = np.max(ptcloud), np.min(ptcloud)
    ax.set_xbound(min, max)
    ax.set_ybound(min, max)
    ax.set_zbound(min, max)
    ax.scatter(x, y, z, zdir='z', c=y, cmap='jet')

    fig.canvas.draw()
    img = np.fromstring(fig.canvas.tostring_rgb(), dtype=np.uint8, sep='')
    img = img.reshape(fig.canvas.get_width_height()[::-1] + (3, ))
    return img



def visualize_KITTI(path, data_list, titles = ['input','pred'], cmap=['bwr','autumn'], zdir='y', 
                         xlim=(-1, 1), ylim=(-1, 1), zlim=(-1, 1) ):
    fig = plt.figure(figsize=(6*len(data_list),6))
    cmax = data_list[-1][:,0].max()

    for i in range(len(data_list)):
        data = data_list[i][:-2048] if i == 1 else data_list[i]
        color = data[:,0] /cmax
        ax = fig.add_subplot(1, len(data_list) , i + 1, projection='3d')
        ax.view_init(30, -120)
        b = ax.scatter(data[:, 0], data[:, 1], data[:, 2], zdir=zdir, c=color,vmin=-1,vmax=1 ,cmap = cmap[0],s=4,linewidth=0.05, edgecolors = 'black')
        ax.set_title(titles[i])

        ax.set_axis_off()
        ax.set_xlim(xlim)
        ax.set_ylim(ylim)
        ax.set_zlim(zlim)
    plt.subplots_adjust(left=0, right=1, bottom=0, top=1, wspace=0.2, hspace=0)
    if not os.path.exists(path):
        os.makedirs(path)

    pic_path = path + '.png'
    fig.savefig(pic_path)

    np.save(os.path.join(path, 'input.npy'), data_list[0].numpy())
    np.save(os.path.join(path, 'pred.npy'), data_list[1].numpy())
    plt.close(fig)


def random_dropping(pc, e):
    up_num = max(64, 768 // (e//50 + 1))
    pc = pc
    random_num = torch.randint(1, up_num, (1,1))[0,0]
    pc = fps(pc, random_num)
    padding = torch.zeros(pc.size(0), 2048 - pc.size(1), 3).to(pc.device)
    pc = torch.cat([pc, padding], dim = 1)
    return pc
    

def random_scale(partial, scale_range=[0.8, 1.2]):
    scale = torch.rand(1).cuda() * (scale_range[1] - scale_range[0]) + scale_range[0]
    return partial * scale
