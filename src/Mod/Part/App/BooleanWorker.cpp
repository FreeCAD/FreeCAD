#include "BooleanOperation.h"
#include <iostream>

int main() {
    BooleanOperation op_data;
    try {
        op_data = BooleanOperation::readInput(STDIN_FILENO);
        std::string result = op_data.performOperation();
        op_data.writeResult(STDOUT_FILENO, result, false);
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        try {
            op_data.writeResult(STDOUT_FILENO, e.what(), true);
        }
        catch (...) {
            std::cerr << "Failed to write error message to stdout" << std::endl;
            return 1;
        }
        return 0;
    }
} 