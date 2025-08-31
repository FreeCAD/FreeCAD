from .camotics import CamoticsToolBitSerializer
from .fctb import FCTBSerializer


all_serializers = CamoticsToolBitSerializer, FCTBSerializer


__all__ = [
    "CamoticsToolBitSerializer",
    "FCTBSerializer",
    "all_serializers",
]
