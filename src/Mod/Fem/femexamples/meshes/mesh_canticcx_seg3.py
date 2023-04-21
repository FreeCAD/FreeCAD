def create_nodes(femmesh):
    # nodes
    femmesh.addNode(0.0, 500.0, 500.0, 1)
    femmesh.addNode(8000.0, 500.0, 500.0, 2)
    femmesh.addNode(1600.0000000000023, 500.0, 500.0, 3)
    femmesh.addNode(3200.000000000006, 500.0, 500.0, 4)
    femmesh.addNode(4800.000000000003, 500.0, 500.0, 5)
    femmesh.addNode(6399.999999999996, 500.0, 500.0, 6)
    femmesh.addNode(800.0000000000011, 500.0, 500.0, 7)
    femmesh.addNode(2400.000000000004, 500.0, 500.0, 8)
    femmesh.addNode(4000.0000000000045, 500.0, 500.0, 9)
    femmesh.addNode(5599.999999999999, 500.0, 500.0, 10)
    femmesh.addNode(7199.999999999998, 500.0, 500.0, 11)
    return True


def create_elements(femmesh):
    # elements
    femmesh.addEdge([1, 3, 7], 1)
    femmesh.addEdge([3, 4, 8], 2)
    femmesh.addEdge([4, 5, 9], 3)
    femmesh.addEdge([5, 6, 10], 4)
    femmesh.addEdge([6, 2, 11], 5)
    return True
