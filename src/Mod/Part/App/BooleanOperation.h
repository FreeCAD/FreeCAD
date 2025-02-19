#ifndef BOOLEAN_WORKER_H
#define BOOLEAN_WORKER_H

#include <Mod/Part/App/TopoShape.h>
#include <string>
#include <vector>

// Helper functions for I/O (kept as free functions)
bool readExact(int fd, void* buffer, size_t size);
bool writeExact(int fd, const void* buffer, size_t size);

class BooleanOperation {
public:
    char maker;
    std::vector<Part::TopoShape> shapes;
    std::string op;
    double tolerance;

    // Static factory method
    static BooleanOperation readInput(int fd);
    
    // Member functions
    void writeInput(int fd) const;
    void writeResult(int fd, const std::string& data, bool isError) const;
    static std::string readResult(int fd, bool& isError);
    std::string performOperation() const;
};

#endif // BOOLEAN_WORKER_H 