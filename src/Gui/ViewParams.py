import sys
from os import path

# For importing Base/params_utils.py
sys.path.append(path.join(path.dirname(path.dirname(path.abspath(__file__))), 'Base'))
import params_utils

from params_utils import ParamBool, ParamInt, ParamString, ParamUInt, ParamFloat

NameSpace = 'Gui'
ClassName = 'ViewParams'
ParamPath = 'User parameter:BaseApp/Preferences/View'
ClassDoc = 'Convenient class to obtain view provider related parameters'

Params = [
    ParamBool('UseNewSelection', True),
    ParamBool('UseSelectionRoot', True),
    ParamBool('EnableSelection', True),
    ParamInt('RenderCache', 0),
    ParamBool('RandomColor', False),
    ParamUInt('BoundingBoxColor', 0xffffffff),
    ParamUInt('AnnotationTextColor', 0xffffffff),
    ParamInt('MarkerSize', 9),
    ParamUInt('DefaultLinkColor', 0x66FFFFFF),
    ParamUInt('DefaultShapeLineColor', 0x191919FF),
    ParamUInt('DefaultShapeVertexColor', 0x191919FF),
    ParamUInt('DefaultShapeColor', 0xCCCCCCFF),
    ParamInt('DefaultShapeLineWidth', 2),
    ParamInt('DefaultShapePointSize', 2),
    ParamBool('CoinCycleCheck', True),
    ParamBool('EnablePropertyViewForInactiveDocument', True),
    ParamBool('ShowSelectionBoundingBox', False, "Show bounding box when object is selectedin 3D view"),
    ParamUInt('PropertyViewTimer', 100),
    ParamInt('DefaultFontSize', 0, on_change=True),
]

def declare_begin():
    params_utils.declare_begin(sys.modules[__name__])

def declare_end():
    params_utils.declare_end(sys.modules[__name__])

def define():
    params_utils.define(sys.modules[__name__])
