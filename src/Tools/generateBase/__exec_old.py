# remove this file once the python2 support has stopped


def __exec_old__(text, globals, locals):
    exec text in globals, locals  # noqa: E999 a Python 3 syntax error
