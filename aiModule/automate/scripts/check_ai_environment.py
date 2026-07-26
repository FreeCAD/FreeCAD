"""Print and validate the isolated AutoMate AI runtime."""

from __future__ import annotations

import platform
import sys

import numpy
import pandas
import pyarrow
import torch
import torch_geometric
import torch_scatter


def main():
    print(f"python={sys.version.split()[0]} platform={platform.platform()}")
    print(f"numpy={numpy.__version__}")
    print(f"pandas={pandas.__version__}")
    print(f"pyarrow={pyarrow.__version__}")
    print(f"torch={torch.__version__}")
    print(f"torch_geometric={torch_geometric.__version__}")
    print(f"torch_scatter={torch_scatter.__version__}")
    print(f"cuda_available={torch.cuda.is_available()}")
    if torch.cuda.is_available():
        print(f"cuda_runtime={torch.version.cuda}")
        print(f"gpu={torch.cuda.get_device_name(0)}")
        total = torch.cuda.get_device_properties(0).total_memory / (1024 ** 3)
        print(f"gpu_memory_gib={total:.2f}")
    else:
        raise SystemExit("CUDA is not available in the AutoMate AI environment")


if __name__ == "__main__":
    main()
