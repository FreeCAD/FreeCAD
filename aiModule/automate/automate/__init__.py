import os
import sys
from pathlib import Path


_ROOT = Path(__file__).resolve().parent.parent
_BUILD_DIR = _ROOT / ("build-ai" if sys.version_info[:2] == (3, 10) else "build-msvc-clean")
if str(_BUILD_DIR) not in sys.path:
    sys.path.insert(0, str(_BUILD_DIR))
_DLL_DIR = None
if os.name == "nt":
    _DLL_DIR = os.add_dll_directory(str(Path(sys.prefix) / "Library" / "bin"))

from .conversions import jsonify, torchify
from .brep import PartFeatures, part_to_graph, HetData, PartDataset, flatbatch
from .sbgcn import SBGCN, LinearBlock, BipartiteResMRConv
from .eclasses import find_eclasses
from .mate_dataset import MateBatch, MateDataset, MateSample, collate_mates, make_mate_dataloader
from .mate_model import MateModelConfig, MateModelOutput, MatePairModel
from .mate_type_dataset import (
    MateTypeBatch,
    MateTypeDataset,
    MateTypeSample,
    collate_mate_types,
    make_mate_type_dataloader,
)
from .mate_type_model import MateTypeModel, MateTypeModelConfig

from automate_cpp import Part, PartOptions

try:
    from .util import run_model, ArgparseInitialized
except ModuleNotFoundError as exc:
    if exc.name != "pytorch_lightning":
        raise
    run_model = None
    ArgparseInitialized = None


__all__ = [
    'jsonify', 
    'torchify', 
    'PartFeatures', 
    'part_to_graph', 
    'HetData', 
    'SBGCN',
    'LinearBlock',
    'PartDataset',
    'flatbatch',
    'BipartiteResMRConv',
    'Part',
    'PartOptions',
    'find_eclasses',
    'MateBatch',
    'MateDataset',
    'MateSample',
    'collate_mates',
    'make_mate_dataloader',
    'MateModelConfig',
    'MateModelOutput',
    'MatePairModel',
    'MateTypeBatch',
    'MateTypeDataset',
    'MateTypeSample',
    'collate_mate_types',
    'make_mate_type_dataloader',
    'MateTypeModel',
    'MateTypeModelConfig',
]

if run_model is not None:
    __all__.extend(['run_model', 'ArgparseInitialized'])
