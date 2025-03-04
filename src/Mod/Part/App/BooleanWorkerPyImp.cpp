#include "BooleanOperation.h"
#include <iostream>

PyObject* RunBooleanWorker(PyObject* /*self*/, PyObject* /*args*/) {
    BooleanOperation op_data;
    try {
        op_data = BooleanOperation::readInput(STDIN_FILENO);
        std::string result = op_data.performOperation();
        op_data.writeResult(STDOUT_FILENO, result, false);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        try {
            op_data.writeResult(STDOUT_FILENO, e.what(), true);
        }
        catch (...) {
            std::cerr << "Failed to write error message to stdout" << std::endl;
            _exit(1);
        }
    }
    _exit(0);
} 

PyMethodDef BooleanWorkerMethods[] = {
    {"RunBooleanWorker", RunBooleanWorker, METH_VARARGS, "Run the boolean worker"},
    {NULL, NULL, 0, NULL}
};