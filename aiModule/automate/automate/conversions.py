import numpy as np
import torch

# Functions that convert pybind11 objects to serializable formats

def torchify(obj):
    r"""
    Convert to nested dictionary of torch tensors for use with 
    torch.save. Roughly 6.3x file size of original Parasolid .x_t
    """

    if torch.is_tensor(obj):
        return obj

    primitive = (int, str, bool, float)
    number = (int, float)
    if isinstance(obj, primitive):
        return obj
    if isinstance(obj, np.ndarray):
        # pybind11/Eigen commonly exposes read-only views owned by the C++
        # object. Copy so cached tensors own writable memory after that object
        # is released.
        return torch.from_numpy(obj.copy())
    if isinstance(obj, list):
        if len(obj) > 0:
            if isinstance(obj[0], number):
                return torch.tensor(obj)
            if isinstance(obj[0], np.ndarray):
                return torch.tensor(np.stack(obj))
            if isinstance(obj[0], list):
                if len(obj[0]) > 0:
                    if isinstance(obj[0][0], np.ndarray):
                        return torch.tensor(
                            np.stack([
                                np.stack(l) for l in obj
                            ])
                        )
                else:
                    return [[]]   
        return [torchify(x) for x in obj]
    
    keys = [x for x in dir(obj) if not x.startswith('__')]
    
    if 'name' in keys and 'value' in keys: # this is an enum
        return {
            'value': getattr(obj, 'value'),
            'name': getattr(obj, 'name'),
            'enum_size': len(keys) - 2 # the other keys are all enum opts
        }

    return dict(zip(
        keys,
        [torchify(getattr(obj, key)) for key in keys]
    ))


def jsonify(obj):
    r"""
    Convert to nested dictionary of python primitives for use with 
    json.dump. Roughly 10x file size of original Parasolid .x_t
    """
    primitive = (int, str, bool, float)
    if isinstance(obj, primitive):
        return obj
    if isinstance(obj, np.ndarray):
        return obj.tolist()
    if isinstance(obj, list):
        return [jsonify(x) for x in obj]
    
    keys = [x for x in dir(obj) if not x.startswith('__')]
    
    if 'name' in keys and 'value' in keys: # this is an enum
        return {
            'value': getattr(obj, 'value'),
            'name': getattr(obj, 'name'),
            'enum_size': len(keys) - 2 # the other keys are all enum opts
        }

    return dict(zip(
        keys,
        [jsonify(getattr(obj, key)) for key in keys]
    ))
