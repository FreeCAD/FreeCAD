def create_nodes(femmesh):
    # nodes
    femmesh.addNode(0.0, 500.0, 0.0, 1)
    femmesh.addNode(0.0, 500.0000000000002, 1000.0, 2)
    femmesh.addNode(8000.0, 500.0, 0.0, 3)
    femmesh.addNode(8000.0, 500.0000000000002, 1000.0, 4)
    femmesh.addNode(0.0, 500.0000000000001, 500.0, 5)
    femmesh.addNode(4000.0, 500.0, 0.0, 6)
    femmesh.addNode(2000.0, 500.0, 0.0, 7)
    femmesh.addNode(6000.0, 500.0, 0.0, 8)
    femmesh.addNode(8000.0, 500.0000000000001, 500.0, 9)
    femmesh.addNode(4000.0, 500.0000000000002, 1000.0, 10)
    femmesh.addNode(2000.0, 500.0000000000002, 1000.0, 11)
    femmesh.addNode(6000.0, 500.0000000000002, 1000.0, 12)
    femmesh.addNode(4000.0, 500.0000000000001, 500.0, 13)
    femmesh.addNode(2000.0, 500.0000000000001, 500.0, 14)
    femmesh.addNode(6000.0, 500.0000000000001, 500.0, 15)
    return True


def create_elements(femmesh):
    # elements
    femmesh.addFace([1, 7, 14, 5], 13)
    femmesh.addFace([2, 5, 14, 11], 14)
    femmesh.addFace([10, 11, 14, 13], 15)
    femmesh.addFace([6, 13, 14, 7], 16)
    femmesh.addFace([3, 9, 15, 8], 17)
    femmesh.addFace([6, 8, 15, 13], 18)
    femmesh.addFace([10, 13, 15, 12], 19)
    femmesh.addFace([4, 12, 15, 9], 20)
    return True
