#
# $Id: py-kdtree_test.py 2268 2008-08-20 10:08:58Z richert $
#

import unittest

from kdtree import KDTree_2Int, KDTree_4Int, KDTree_3Float, KDTree_4Float, KDTree_6Float


class KDTree_2IntTestCase(unittest.TestCase):
    def test_empty(self):
        nn = KDTree_2Int()
        self.assertEqual(0, nn.size())
        
        actual = nn.find_nearest((2,3))
        self.assertTrue(None==actual, "%s != %s"%(str(None), str(actual)))

    def test_get_all(self):
        nn = KDTree_2Int()
        o1 = object()
        nn.add(((1,1), id(o1)))
        o2 = object()
        nn.add(((10,10), id(o2)))
        o3 = object()
        nn.add(((11,11), id(o3)))

        self.assertEqual([((1,1), id(o1)), ((10,10), id(o2)), ((11,11), id(o3))], nn.get_all())
        self.assertEqual(3, len(nn))
        
        nn.remove(((10,10), id(o2)))
        self.assertEqual(2, len(nn))        
        self.assertEqual([((1,1), id(o1)), ((11,11), id(o3))], nn.get_all())
        
    def test_nearest(self):
        nn = KDTree_2Int()

        nn_id = {}
        
        o1 = object()
        nn.add(((1,1), id(o1)))
        nn_id[id(o1)] = o1
        o2 = object()
        nn.add(((10,10), id(o2)))
        nn_id[id(o2)] = o2
        
        expected =  o1
        actual = nn.find_nearest((2,2))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

        expected = o2
        actual = nn.find_nearest((6, 6))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

    def test_find_within_range(self):
        nn = KDTree_6Float()
 
        nn_id = {}
        
        o1 = object()
        nn.add(((1,1,0,0,0,0), id(o1)))
        nn_id[id(o1)] = o1
        o2 = object()
        nn.add(((10,10,0,0,0,0), id(o2)))
        nn_id[id(o2)] = o2
        o3 = object()
        nn.add(((4.1, 4.1,0,0,0,0), id(o3)))
        nn_id[id(o3)] = o3
        
        expected =  set([long(id(o1)), long(id(o3))])
        actual = set([ident
                      for _coord, ident
                      in nn.find_within_range((2.1,2.1,0,0,0,0), 3.9)])
        self.assertTrue(expected==actual, "%s != %s"%(str(expected), str(actual)))
 

    def test_remove(self):
        class C:
            def __init__(self, i):
                self.i = i
                self.next = None

        nn = KDTree_2Int()

        k1, o1 = (1,1), C(7)
        self.assertFalse(nn.remove((k1, id(o1))), "This cannot be removed!")
        nn.add((k1, id(o1)))

        k2, o2 = (1,1), C(7)
        nn.add((k2, id(o2)))

        self.assertEqual(2, nn.size())
        self.assertTrue(nn.remove((k2, id(o2))))
        self.assertEqual(1, nn.size())        
        self.assertFalse(nn.remove((k2, id(o2))))        
        self.assertEqual(1, nn.size())

        nearest = nn.find_nearest(k1)
        self.assertTrue(nearest[1] == id(o1), "%s != %s"%(nearest[1], o1))
        #self.assertTrue(nearest[1] is o1, "%s,%s is not %s"%(str(nearest[0]), str(nearest[1]), str((k1,id(o1)))))

    def test_count_within_range(self):
        nn = KDTree_2Int()

        for p in [(0,0), (1,0), (0,1), (1,1)]:
            nn.add((p, id(p)))

        res = nn.count_within_range((0,0), 1.0)
        self.assertEqual(3, res, "Counted %i points instead of %i"%(res, 3))
        
        res = nn.count_within_range((0,0), 1.9)
        self.assertEqual(4, res, "Counted %i points instead of %i"%(res, 4))        

class KDTree_4IntTestCase(unittest.TestCase):
    def test_empty(self):
        nn = KDTree_4Int()
        self.assertEqual(0, nn.size())
        
        actual = nn.find_nearest((0,0,2,3))
        self.assertTrue(None==actual, "%s != %s"%(str(None), str(actual)))

    def test_get_all(self):
        nn = KDTree_4Int()
        o1 = object()
        nn.add(((0,0,1,1), id(o1)))
        o2 = object()
        nn.add(((0,0,10,10), id(o2)))
        o3 = object()
        nn.add(((0,0,11,11), id(o3)))

        self.assertEqual([((0,0,1,1), id(o1)), ((0,0,10,10), id(o2)), ((0,0,11,11), id(o3))], nn.get_all())
        self.assertEqual(3, len(nn))
        
        nn.remove(((0,0,10,10), id(o2)))
        self.assertEqual(2, len(nn))        
        self.assertEqual([((0,0,1,1), id(o1)), ((0,0,11,11), id(o3))], nn.get_all())
        
    def test_nearest(self):
        nn = KDTree_4Int()

        nn_id = {}
        
        o1 = object()
        nn.add(((0,0,1,1), id(o1)))
        nn_id[id(o1)] = o1
        o2 = object()
        nn.add(((0,0,10,10), id(o2)))
        nn_id[id(o2)] = o2
        
        expected =  o1
        actual = nn.find_nearest((0,0,2,2))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

        expected = o2
        actual = nn.find_nearest((0,0,6,6))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

    def test_remove(self):
        class C:
            def __init__(self, i):
                self.i = i
                self.next = None

        nn = KDTree_4Int()

        k1, o1 = (0,0,1,1), C(7)
        self.assertFalse(nn.remove((k1, id(o1))), "This cannot be removed!")
        nn.add((k1, id(o1)))

        k2, o2 = (0,0,1,1), C(7)
        nn.add((k2, id(o2)))

        self.assertEqual(2, nn.size())
        self.assertTrue(nn.remove((k2, id(o2))))
        self.assertEqual(1, nn.size())        
        self.assertFalse(nn.remove((k2, id(o2))))        
        self.assertEqual(1, nn.size())

        nearest = nn.find_nearest(k1)
        self.assertTrue(nearest[1] == id(o1), "%s != %s"%(nearest[1], o1))
        #self.assertTrue(nearest[1] is o1, "%s,%s is not %s"%(str(nearest[0]), str(nearest[1]), str((k1,id(o1)))))

class KDTree_4FloatTestCase(unittest.TestCase):
    def test_empty(self):
        nn = KDTree_4Float()
        self.assertEqual(0, nn.size())
        
        actual = nn.find_nearest((0,0,2,3))
        self.assertTrue(None==actual, "%s != %s"%(str(None), str(actual)))

    def test_get_all(self):
        nn = KDTree_4Int()
        o1 = object()
        nn.add(((0,0,1,1), id(o1)))
        o2 = object()
        nn.add(((0,0,10,10), id(o2)))
        o3 = object()
        nn.add(((0,0,11,11), id(o3)))

        self.assertEqual([((0,0,1,1), id(o1)), ((0,0,10,10), id(o2)), ((0,0,11,11), id(o3))], nn.get_all())
        self.assertEqual(3, len(nn))
        
        nn.remove(((0,0,10,10), id(o2)))
        self.assertEqual(2, len(nn))        
        self.assertEqual([((0,0,1,1), id(o1)), ((0,0,11,11), id(o3))], nn.get_all())
        
    def test_nearest(self):
        nn = KDTree_4Int()

        nn_id = {}
        
        o1 = object()
        nn.add(((0,0,1,1), id(o1)))
        nn_id[id(o1)] = o1
        o2 = object()
        nn.add(((0,0,10,10), id(o2)))
        nn_id[id(o2)] = o2
        
        expected =  o1
        actual = nn.find_nearest((0,0,2,2))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

        expected = o2
        actual = nn.find_nearest((0,0,6,6))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

    def test_remove(self):
        class C:
            def __init__(self, i):
                self.i = i
                self.next = None

        nn = KDTree_4Int()

        k1, o1 = (0,0,1,1), C(7)
        self.assertFalse(nn.remove((k1, id(o1))), "This cannot be removed!")
        nn.add((k1, id(o1)))

        k2, o2 = (0,0,1,1), C(7)
        nn.add((k2, id(o2)))

        self.assertEqual(2, nn.size())
        self.assertTrue(nn.remove((k2, id(o2))))
        self.assertEqual(1, nn.size())        
        self.assertFalse(nn.remove((k2, id(o2))))        
        self.assertEqual(1, nn.size())

        nearest = nn.find_nearest(k1)
        self.assertTrue(nearest[1] == id(o1), "%s != %s"%(nearest[1], o1))
        #self.assertTrue(nearest[1] is o1, "%s,%s is not %s"%(str(nearest[0]), str(nearest[1]), str((k1,id(o1)))))

class KDTree_3FloatTestCase(unittest.TestCase):
    def test_empty(self):
        nn = KDTree_3Float()
        self.assertEqual(0, nn.size())
        
        actual = nn.find_nearest((2,3,0))
        self.assertTrue(None==actual, "%s != %s"%(str(None), str(actual)))

    def test_get_all(self):
        nn = KDTree_3Float()
        o1 = object()
        nn.add(((1,1,0), id(o1)))
        o2 = object()
        nn.add(((10,10,0), id(o2)))
        o3 = object()
        nn.add(((11,11,0), id(o3)))

        self.assertEqual([((1,1,0), id(o1)), ((10,10,0), id(o2)), ((11,11,0), id(o3))], nn.get_all())
        self.assertEqual(3, len(nn))
        
        nn.remove(((10,10,0), id(o2)))
        self.assertEqual(2, len(nn))        
        self.assertEqual([((1,1,0), id(o1)), ((11,11,0), id(o3))], nn.get_all())
        
    def test_nearest(self):
        nn = KDTree_3Float()

        nn_id = {}
        
        o1 = object()
        nn.add(((1,1,0), id(o1)))
        nn_id[id(o1)] = o1
        o2 = object()
        nn.add(((10,10,0), id(o2)))
        nn_id[id(o2)] = o2
        o3 = object()
        nn.add(((4.1, 4.1,0), id(o3)))
        nn_id[id(o3)] = o3
        
        expected =  o3
        actual = nn.find_nearest((2.9,2.9,0))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

        expected = o3
        actual = nn.find_nearest((6, 6,0))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

    def test_remove(self):
        class C:
            def __init__(self, i):
                self.i = i
                self.next = None

        nn = KDTree_3Float()

        k1, o1 = (1.1,1.1,0), C(7)
        self.assertFalse(nn.remove((k1, id(o1))), "This cannot be removed!")
        nn.add((k1, id(o1)))

        k2, o2 = (1.1,1.1,0), C(7)
        nn.add((k2, id(o2)))

        self.assertEqual(2, nn.size())
        self.assertTrue(nn.remove((k2, id(o2))))
        self.assertEqual(1, nn.size())        
        self.assertFalse(nn.remove((k2, id(o2))))        
        self.assertEqual(1, nn.size())

        nearest = nn.find_nearest(k1)
        self.assertTrue(nearest[1] == id(o1), "%s != %s"%(nearest[1], o1))
        #self.assertTrue(nearest[1] is o1, "%s,%s is not %s"%(str(nearest[0]), str(nearest[1]), str((k1,id(o1)))))
        
class KDTree_6FloatTestCase(unittest.TestCase):
    def test_empty(self):
        nn = KDTree_6Float()
        self.assertEqual(0, nn.size())
        
        actual = nn.find_nearest((2,3,0,0,0,0))
        self.assertTrue(None==actual, "%s != %s"%(str(None), str(actual)))

    def test_get_all(self):
        nn = KDTree_6Float()
        o1 = object()
        nn.add(((1,1,0,0,0,0), id(o1)))
        o2 = object()
        nn.add(((10,10,0,0,0,0), id(o2)))
        o3 = object()
        nn.add(((11,11,0,0,0,0), id(o3)))

        self.assertEqual([((1,1,0,0,0,0), id(o1)), ((10,10,0,0,0,0), id(o2)), ((11,11,0,0,0,0   ), id(o3))], nn.get_all())
        self.assertEqual(3, len(nn))
        
        nn.remove(((10,10,0,0,0,0), id(o2)))
        self.assertEqual(2, len(nn))        
        self.assertEqual([((1,1,0,0,0,0), id(o1)), ((11,11,0,0,0,0), id(o3))], nn.get_all())
        
    def test_nearest(self):
        nn = KDTree_6Float()

        nn_id = {}
        
        o1 = object()
        nn.add(((1,1,0,0,0,0), id(o1)))
        nn_id[id(o1)] = o1
        o2 = object()
        nn.add(((10,10,0,0,0,0), id(o2)))
        nn_id[id(o2)] = o2
        o3 = object()
        nn.add(((4.1, 4.1,0,0,0,0), id(o3)))
        nn_id[id(o3)] = o3
        
        expected =  o3
        actual = nn.find_nearest((2.9,2.9,0,0,0,0))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

        expected = o3
        actual = nn.find_nearest((6, 6,0,0,0,0))[1]
        self.assertTrue(expected==nn_id[actual], "%s != %s"%(str(expected), str(nn_id[actual])))

    def test_remove(self):
        class C:
            def __init__(self, i):
                self.i = i
                self.next = None

        nn = KDTree_6Float()

        k1, o1 = (1.1,1.1,0,0,0,0), C(7)
        self.assertFalse(nn.remove((k1, id(o1))), "This cannot be removed!")
        nn.add((k1, id(o1)))

        k2, o2 = (1.1,1.1,0,0,0,0), C(7)
        nn.add((k2, id(o2)))

        self.assertEqual(2, nn.size())
        self.assertTrue(nn.remove((k2, id(o2))))
        self.assertEqual(1, nn.size())        
        self.assertFalse(nn.remove((k2, id(o2))))        
        self.assertEqual(1, nn.size())

        nearest = nn.find_nearest(k1)
        self.assertTrue(nearest[1] == id(o1), "%s != %s"%(nearest[1], o1))
        #self.assertTrue(nearest[1] is o1, "%s,%s is not %s"%(str(nearest[0]), str(nearest[1]), str((k1,id(o1)))))

                
def suite():
    return unittest.defaultTestLoader.loadTestsFromModule(sys.modules.get(__name__))
    
if __name__ == '__main__':
    unittest.main()
