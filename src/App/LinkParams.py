import sys
from os import sys, path

# Actual code generation is done in Base/param_utils.py.

# The following code is to import param_util.py without needing __init__.py in Base directory
sys.path.append(path.join(path.dirname(path.dirname(path.abspath(__file__))), 'Base'))
import params_utils

from params_utils import ParamBool, ParamInt, ParamString, ParamUInt, ParamFloat

NameSpace = 'App'
ClassName = 'LinkParams'
ParamPath = 'User parameter:BaseApp/Preferences/Link'
ClassDoc = 'Convenient class to obtain App::Link related parameters'
HeaderFile = 'Link.h'
SourceFile = 'Link.cpp'

Params = [
    ParamBool('CopyOnChangeApplyToAll', True, '''\
Stores the last user choice of whether to apply CopyOnChange setup to all link
that links to the same configurable object'''),
]

def declare():
    params_utils.declare_begin(sys.modules[__name__], header=False)
    params_utils.declare_end(sys.modules[__name__])

def define():
    params_utils.define(sys.modules[__name__], header=False)
