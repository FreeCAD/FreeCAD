#include <Base/Console.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/PartGlobal.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "BooleanOperation.h"
#include <unistd.h>

// Helper functions for I/O with file descriptors
bool readExact(int fd, void* buffer, size_t size) {
    size_t total_read = 0;
    while (total_read < size) {
        ssize_t bytes_read = read(fd, static_cast<char*>(buffer) + total_read, size - total_read);
        if (bytes_read <= 0) {
            return false;
        }
        total_read += bytes_read;
    }
    return true;
}

bool writeExact(int fd, const void* buffer, size_t size) {
    size_t total_written = 0;
    while (total_written < size) {
        ssize_t bytes_written = write(fd, static_cast<const char*>(buffer) + total_written, size - total_written);
        if (bytes_written <= 0) {
            return false;
        }
        total_written += bytes_written;
    }
    return true;
}

BooleanOperation BooleanOperation::readInput(int fd) {
    BooleanOperation op_data;
    
    // Read operation type (single char)
    char op_type;
    if (!readExact(fd, &op_type, 1)) {
        throw std::runtime_error("Failed to read operation type");
    }
    op_data.maker = op_type;

    // Read tolerance
    if (!readExact(fd, &op_data.tolerance, sizeof(double))) {
        throw std::runtime_error("Failed to read tolerance");
    }

    // Read operation string
    ssize_t op_size;
    if (!readExact(fd, &op_size, sizeof(ssize_t))) {
        throw std::runtime_error("Failed to read operation size");
    }
    
    std::vector<char> op_buffer(op_size);
    if (!readExact(fd, op_buffer.data(), op_size)) {
        throw std::runtime_error("Failed to read operation");
    }
    op_data.op = std::string(op_buffer.begin(), op_buffer.end());

    // Read number of shapes
    uint64_t numShapes;
    if (!readExact(fd, &numShapes, sizeof(numShapes))) {
        throw std::runtime_error("Failed to read shape count");
    }

    if (numShapes < 2) {
        throw std::runtime_error("Need at least 2 shapes");
    }

    // Read shapes
    for (uint64_t i = 0; i < numShapes; i++) {
        uint64_t size;
        if (!readExact(fd, &size, sizeof(size))) {
            throw std::runtime_error("Failed to read shape size");
        }

        std::string brep_data(size, '\0');
        if (!readExact(fd, brep_data.data(), size)) {
            throw std::runtime_error("Failed to read shape data");
        }

        Part::TopoShape shape;
        std::istringstream shape_is(brep_data);
        shape.importBinary(shape_is);
        op_data.shapes.push_back(shape);
    }

    return op_data;
}

std::string BooleanOperation::performOperation() const {
    Part::TopoShape result;
    result.makeElementBoolean(
        maker == 'C' ? Part::OpCodes::Cut : Part::OpCodes::Fuse,
        shapes,
        op.c_str(),
        tolerance
    );

    std::ostringstream os;
    result.exportBinary(os);
    return os.str();
}

void BooleanOperation::writeInput(int fd) const {
    // Write operation type
    if (!writeExact(fd, &maker, 1)) {
        throw std::runtime_error("Failed to write operation type");
    }

    // Write tolerance
    if (!writeExact(fd, &tolerance, sizeof(double))) {
        throw std::runtime_error("Failed to write tolerance");
    }

    // Write operation
    ssize_t op_size = op.size();
    if (!writeExact(fd, &op_size, sizeof(ssize_t))) {
        throw std::runtime_error("Failed to write operation size");
    }
    if (!writeExact(fd, op.c_str(), op_size)) {
        throw std::runtime_error("Failed to write operation");
    }

    // Write number of shapes
    uint64_t numShapes = static_cast<uint64_t>(shapes.size());
    if (!writeExact(fd, &numShapes, sizeof(numShapes))) {
        throw std::runtime_error("Failed to write shape count");
    }

    // Write shapes
    for (const auto& shape : shapes) {
        // Use a direct buffer approach
        std::stringbuf buffer;
        std::ostream os(&buffer);
        shape.exportBinary(os);
        
        // Get the buffer view to avoid extra copying
        std::string brep_data = buffer.str();
        uint64_t size = brep_data.size();

        if (!writeExact(fd, &size, sizeof(size)) ||
            !writeExact(fd, brep_data.data(), size)) {
            throw std::runtime_error("Failed to write shape data");
        }
    }
}

void BooleanOperation::writeResult(int fd, const std::string& data, bool isError) const {
    uint64_t size = data.size();
    
    if (!writeExact(fd, &size, sizeof(size)) ||
        !writeExact(fd, &isError, 1) ||
        !writeExact(fd, data.data(), size)) {
        throw std::runtime_error("Failed to write output");
    }
}

std::string BooleanOperation::readResult(int fd, bool& isError) {
    // Read size
    uint64_t size;
    //Base::Console().Error("Reading result size\n");
    if (!readExact(fd, &size, sizeof(size))) {
        throw std::runtime_error("Failed to read result size");
    }
    //Base::Console().Error("Result size read: %d\n", size);

    // Read error flag
    //Base::Console().Error("Reading error flag\n");
    if (!readExact(fd, &isError, 1)) {
        throw std::runtime_error("Failed to read error flag");
    }
    //Base::Console().Error("Error flag read: %d\n", isError);

    // Read data
    std::string data(size, '\0');
    //Base::Console().Error("Reading result data\n");
    if (!readExact(fd, data.data(), size)) {
        throw std::runtime_error("Failed to read result data");
    }
    //Base::Console().Error("Result data read\n");
    return data;
}
