"""
The CadQuery Gateway Interface.
Provides classes and tools for executing CadQuery scripts
"""
import ast
import traceback
import time
import cadquery

CQSCRIPT = "<cqscript>"

def parse(script_source):
    """
    Parses the script as a model, and returns a model.

    If you would prefer to access the underlying model without building it,
    for example, to inspect its available parameters, construct a CQModel object.

    :param script_source: the script to run. Must be a valid cadquery script
    :return: a CQModel object that defines the script and allows execution

    """
    model = CQModel(script_source)
    return model


class CQModel(object):
    """
    Represents a Cadquery Script.

    After construction, the metadata property contains
    a ScriptMetaData object, which describes the model in more detail,
    and can be used to retrieve the parameters defined by the model.

    the build method can be used to generate a 3d model
    """
    def __init__(self, script_source):
        """
        Create an object by parsing the supplied python script.
        :param script_source: a python script to parse
        """
        self.metadata = ScriptMetadata()
        self.ast_tree = ast.parse(script_source, CQSCRIPT)
        self.script_source = script_source
        self._find_vars()

        # TODO: pick up other script metadata:
        # describe
        # pick up validation methods
        self._find_descriptions()

    def _find_vars(self):
        """
        Parse the script, and populate variables that appear to be
        overridable.
        """
        #assumption here: we assume that variable declarations
        #are only at the top level of the script. IE, we'll ignore any
        #variable definitions at lower levels of the script

        #we don't want to use the visit interface because here we explicitly
        #want to walk only the top level of the tree.
        assignment_finder = ConstantAssignmentFinder(self.metadata)

        for node in self.ast_tree.body:
            if isinstance(node, ast.Assign):
                assignment_finder.visit_Assign(node)

    def _find_descriptions(self):
        description_finder = ParameterDescriptionFinder(self.metadata)
        description_finder.visit(self.ast_tree)

    def validate(self, params):
        """
        Determine if the supplied parameters are valid.
        NOT IMPLEMENTED YET-- raises NotImplementedError
        :param params: a dictionary of parameters

        """
        raise NotImplementedError("not yet implemented")

    def build(self, build_parameters=None, build_options=None):
        """
        Executes the script, using the optional parameters to override those in the model
        :param build_parameters: a dictionary of variables. The variables must be
        assignable to the underlying variable type. These variables override default values in the script
        :param build_options: build options for how to build the model. Build options include things like
        timeouts, tessellation tolerances, etc
        :raises: Nothing. If there is an exception, it will be on the exception property of the result.
        This is the interface so that we can return other information on the result, such as the build time
        :return: a BuildResult object, which includes the status of the result, and either
        a resulting shape or an exception
        """
        if not build_parameters:
            build_parameters = {}

        start = time.clock()
        result = BuildResult()

        try:
            self.set_param_values(build_parameters)
            collector = ScriptCallback()
            env = EnvironmentBuilder().with_real_builtins().with_cadquery_objects() \
                .add_entry("__name__", "__cqgi__") \
                .add_entry("show_object", collector.show_object) \
                .add_entry("debug", collector.debug) \
                .add_entry("describe_parameter",collector.describe_parameter) \
                .build()

            c = compile(self.ast_tree, CQSCRIPT, 'exec')
            exec (c, env)
            result.set_debug(collector.debugObjects )
            result.set_success_result(collector.outputObjects)

        except Exception as ex:
            #print "Error Executing Script:"
            result.set_failure_result(ex)
            #traceback.print_exc()
            #print "Full Text of Script:"
            #print self.script_source

        end = time.clock()
        result.buildTime = end - start
        return result

    def set_param_values(self, params):
        model_parameters = self.metadata.parameters

        for k, v in params.items():
            if k not in model_parameters:
                raise InvalidParameterError("Cannot set value '%s': not a parameter of the model." % k)

            p = model_parameters[k]
            p.set_value(v)


class ShapeResult(object):
    """
    An object created by a build, including the user parameters provided
    """
    def __init__(self):
        self.shape = None
        self.options = None

class BuildResult(object):
    """
    The result of executing a CadQuery script.
    The success property contains whether the exeuction was successful.

    If successful, the results property contains a list of all results,
    and the first_result property contains the first result.

    If unsuccessful, the exception property contains a reference to
    the stack trace that occurred.
    """
    def __init__(self):
        self.buildTime = None
        self.results = [] #list of ShapeResult
        self.debugObjects = [] #list of ShapeResult
        self.first_result = None
        self.success = False
        self.exception = None

    def set_failure_result(self, ex):
        self.exception = ex
        self.success = False

    def set_debug(self, debugObjects):
        self.debugObjects = debugObjects

    def set_success_result(self, results):
        self.results = results
        if len(self.results) > 0:
            self.first_result = self.results[0]
        self.success = True


class ScriptMetadata(object):
    """
    Defines the metadata for a parsed CQ Script.
    the parameters property is a dict of InputParameter objects.
    """
    def __init__(self):
        self.parameters = {}

    def add_script_parameter(self, p):
        self.parameters[p.name] = p

    def add_parameter_description(self,name,description):
        #print 'Adding Parameter name=%s, desc=%s' % ( name, description )
        p = self.parameters[name]
        p.desc = description


class ParameterType(object):
    pass


class NumberParameterType(ParameterType):
    pass


class StringParameterType(ParameterType):
    pass


class BooleanParameterType(ParameterType):
    pass


class InputParameter:
    """
    Defines a parameter that can be supplied when the model is executed.

    Name, varType, and default_value are always available, because they are computed
    from a variable assignment line of code:

    The others are only available if the script has used define_parameter() to
    provide additional metadata

    """
    def __init__(self):

        #: the default value for the variable.
        self.default_value = None

        #: the name of the parameter.
        self.name = None

        #: type of the variable: BooleanParameter, StringParameter, NumericParameter
        self.varType = None

        #: help text describing the variable. Only available if the script used describe_parameter()
        self.desc = None

        #: valid values for the variable. Only available if the script used describe_parameter()
        self.valid_values = []

        self.ast_node = None

    @staticmethod
    def create(ast_node, var_name, var_type, default_value, valid_values=None, desc=None):

        if valid_values is None:
            valid_values = []

        p = InputParameter()
        p.ast_node = ast_node
        p.default_value = default_value
        p.name = var_name
        p.desc = desc
        p.varType = var_type
        p.valid_values = valid_values
        return p

    def set_value(self, new_value):
        if len(self.valid_values) > 0 and new_value not in self.valid_values:
            raise InvalidParameterError(
                "Cannot set value '{0:s}' for parameter '{1:s}': not a valid value. Valid values are {2:s} "
                    .format(str(new_value), self.name, str(self.valid_values)))

        if self.varType == NumberParameterType:
            try:
                # Sometimes a value must stay as an int for the script to work properly
                if isinstance(new_value, int):
                    f = int(new_value)
                else:
                    f = float(new_value)

                self.ast_node.n = f
            except ValueError:
                raise InvalidParameterError(
                    "Cannot set value '{0:s}' for parameter '{1:s}': parameter must be numeric."
                        .format(str(new_value), self.name))

        elif self.varType == StringParameterType:
            self.ast_node.s = str(new_value)
        elif self.varType == BooleanParameterType:
            if new_value:
                if hasattr(ast, 'NameConstant'):
                    self.ast_node.value = True
                else:
                    self.ast_node.id = 'True'
            else:
                if hasattr(ast, 'NameConstant'):
                    self.ast_node.value = False
                else:
                    self.ast_node.id = 'False'
        else:
            raise ValueError("Unknown Type of var: ", str(self.varType))

    def __str__(self):
        return "InputParameter: {name=%s, type=%s, defaultValue=%s" % (
            self.name, str(self.varType), str(self.default_value))


class ScriptCallback(object):
    """
    Allows a script to communicate with the container
    the show_object() method is exposed to CQ scripts, to allow them
    to return objects to the execution environment
    """
    def __init__(self):
        self.outputObjects = []
        self.debugObjects = []

    def show_object(self, shape,options={}):
        """
        return an object to the executing environment, with options
        :param shape: a cadquery object
        :param options: a dictionary of options that will be made available to the executing environment
        """
        o = ShapeResult()
        o.options=options
        o.shape = shape
        self.outputObjects.append(o)

    def debug(self,obj,args={}):
        """
        Debug print/output an object, with optional arguments.
        """
        s = ShapeResult()
        s.shape = obj
        s.options = args
        self.debugObjects.append(s)

    def describe_parameter(self,var_data ):
        """
        Do Nothing-- we parsed the ast ahead of execution to get what we need.
        """
        pass

    def add_error(self, param, field_list):
        """
        Not implemented yet: allows scripts to indicate that there are problems with inputs
        """
        pass

    def has_results(self):
        return len(self.outputObjects) > 0



class InvalidParameterError(Exception):
    """
    Raised when an attempt is made to provide a new parameter value
    that cannot be assigned to the model
    """
    pass


class NoOutputError(Exception):
    """
    Raised when the script does not execute the show_object() method to
    return a solid
    """
    pass


class ScriptExecutionError(Exception):
    """
        Represents a script syntax error.
        Useful for helping clients pinpoint issues with the script
        interactively
    """

    def __init__(self, line=None, message=None):
        if line is None:
            self.line = 0
        else:
            self.line = line

        if message is None:
            self.message = "Unknown Script Error"
        else:
            self.message = message

    def full_message(self):
        return self.__repr__()

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return "ScriptError [Line %s]: %s" % (self.line, self.message)


class EnvironmentBuilder(object):
    """
    Builds an execution environment for a cadquery script.
    The environment includes the builtins, as well as
    the other methods the script will need.
    """
    def __init__(self):
        self.env = {}

    def with_real_builtins(self):
        return self.with_builtins(__builtins__)

    def with_builtins(self, env_dict):
        self.env['__builtins__'] = env_dict
        return self

    def with_cadquery_objects(self):
        self.env['cadquery'] = cadquery
        self.env['cq'] = cadquery
        return self

    def add_entry(self, name, value):
        self.env[name] = value
        return self

    def build(self):
        return self.env

class ParameterDescriptionFinder(ast.NodeTransformer):
    """
    Visits a parse tree, looking for function calls to describe_parameter(var, description )
    """
    def __init__(self, cq_model):
        self.cqModel = cq_model

    def visit_Call(self,node):
       """
       Called when we see a function call. Is it describe_parameter?
       """
       try:
            if node.func.id == 'describe_parameter':
                #looks like we have a call to our function.
                #first parameter is the variable,
                #second is the description
                varname = node.args[0].id
                desc = node.args[1].s
                self.cqModel.add_parameter_description(varname,desc)

       except:
            #print "Unable to handle function call"
            pass
       return node

class ConstantAssignmentFinder(ast.NodeTransformer):
    """
    Visits a parse tree, and adds script parameters to the cqModel
    """

    def __init__(self, cq_model):
        self.cqModel = cq_model

    def handle_assignment(self, var_name, value_node):
        try:

            if type(value_node) == ast.Num:
                self.cqModel.add_script_parameter(
                    InputParameter.create(value_node, var_name, NumberParameterType, value_node.n))
            elif type(value_node) == ast.Str:
                self.cqModel.add_script_parameter(
                    InputParameter.create(value_node, var_name, StringParameterType, value_node.s))
            elif type(value_node) == ast.Name:
                if value_node.id == 'True':
                    self.cqModel.add_script_parameter(
                        InputParameter.create(value_node, var_name, BooleanParameterType, True))
                elif value_node.id == 'False':
                    self.cqModel.add_script_parameter(
                        InputParameter.create(value_node, var_name, BooleanParameterType, False))
            elif hasattr(ast, 'NameConstant') and type(value_node) == ast.NameConstant:
                if value_node.value == True:
                    self.cqModel.add_script_parameter(
                        InputParameter.create(value_node, var_name, BooleanParameterType, True))
                else:
                    self.cqModel.add_script_parameter(
                        InputParameter.create(value_node, var_name, BooleanParameterType, False))
        except:
            print("Unable to handle assignment for variable '%s'" % var_name)
            pass

    def visit_Assign(self, node):

        try:
            left_side = node.targets[0]

                    #do not handle attribute assignments
            if isinstance(left_side,ast.Attribute):
                return

            # Handle the NamedConstant type that is only present in Python 3
            astTypes = [ast.Num, ast.Str, ast.Name]
            if hasattr(ast, 'NameConstant'):
                astTypes.append(ast.NameConstant)

            if type(node.value) in astTypes:
                self.handle_assignment(left_side.id, node.value)
            elif type(node.value) == ast.Tuple:
                # we have a multi-value assignment
                for n, v in zip(left_side.elts, node.value.elts):
                    self.handle_assignment(n.id, v)
        except:
            traceback.print_exc()
            print("Unable to handle assignment for node '%s'" % ast.dump(left_side))

        return node
