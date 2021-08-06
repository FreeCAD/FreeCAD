def create_nodes(femmesh):
    # nodes
    femmesh.addNode(0.0, 500.0, 0.0, 1)
    femmesh.addNode(0.0, 500.00000000000324, 1000.0, 2)
    femmesh.addNode(8000.0, 500.0, 0.0, 3)
    femmesh.addNode(8000.0, 500.00000000000324, 1000.0, 4)
    femmesh.addNode(0.0, 500.0000000000001, 500.0, 5)
    femmesh.addNode(0.0, 500.00000000000006, 250.0, 6)
    femmesh.addNode(0.0, 500.00000000000017, 750.0, 7)
    femmesh.addNode(4000.0, 500.0, 0.0, 8)
    femmesh.addNode(2000.0, 500.0, 0.0, 9)
    femmesh.addNode(6000.0, 500.0, 0.0, 10)
    femmesh.addNode(8000.0, 500.0000000000001, 500.0, 11)
    femmesh.addNode(8000.0, 500.00000000000006, 250.0, 12)
    femmesh.addNode(8000.0, 500.00000000000017, 750.0, 13)
    femmesh.addNode(4000.0, 500.0000000000002, 1000.0, 14)
    femmesh.addNode(2000.0, 500.0000000000002, 1000.0, 15)
    femmesh.addNode(6000.0, 500.0000000000002, 1000.0, 16)
    femmesh.addNode(4000.0, 500.0000000000001, 500.0, 17)
    femmesh.addNode(2000.0, 500.0000000000001, 500.0, 18)
    femmesh.addNode(4000.0, 500.00000000000017, 750.0, 19)
    femmesh.addNode(2000.0, 500.00000000000017, 750.0, 20)
    femmesh.addNode(6000.0, 500.0000000000001, 500.0, 21)
    femmesh.addNode(6000.0, 500.00000000000017, 750.0, 22)
    femmesh.addNode(4000.0, 500.00000000000006, 250.0, 23)
    femmesh.addNode(6000.0, 500.00000000000006, 250.0, 24)
    femmesh.addNode(2000.0, 500.00000000000006, 250.0, 25)
    return True


def create_elements(femmesh):
    # elements
    femmesh.addFace([1, 5, 17, 8, 6, 18, 23, 9], 1)
    femmesh.addFace([5, 2, 14, 17, 7, 15, 19, 18], 2)
    femmesh.addFace([17, 14, 4, 11, 19, 16, 13, 21], 3)
    femmesh.addFace([8, 17, 11, 3, 23, 21, 12, 10], 4)
    return True
