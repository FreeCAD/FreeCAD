import math 

class Point:
    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.X = x
        self.Y = y
        self.Z = z
        
    def distance_to(self, pt):
        '''
        Returns the distance between two points
        '''
        return math.sqrt((pt.X - self.X)**2 + (pt.Y - self.Y)**2 + (pt.Z - self.Z)**2)
        
    def angle_to(self, pt):
        '''
        Returns the distance between two points in degrees
        '''
        dX = self.X - pt.X
        #dY = pt.Y - self.Y
        dZ = self.Z - pt.Z       
        angle = math.degrees(math.atan2(dZ, dX) + math.pi)
        #print('angle of the dangle:', angle)
        return angle
    
    def nearest(self, pts):
        nearest = Point()
        distance = float('inf')
        for pt in pts:
            if self.distance_to(pt) < distance:
                distance = self.distance_to(pt)
                nearest = pt
        #print('nearest', nearest.X, nearest.Z)
        return nearest

    def is_same(self, pt):
        '''
        Returns True is the coordinates are the same
        '''
        if pt:
            if self.X == pt.X:
                if self.Y == pt.Y:
                    if self.Z == pt.Z:
                        return True
        
        return False
        
    def sub(self, pt):
        p = Point(self.X - pt.X, self.Y - pt.Y, self.Z - pt.Z)
        return p
        
    def add(self, pt):
        p = Point(self.X + pt.X, self.Y + pt.Y, self.Z + pt.Z)
        return p
        
    def multiply(self, val):
        p = Point(self.X * val , self.Y * val, self.Z * val)
        return p

    def lerp (self, pt, t):
        p = Point(self.X + (pt.X - self.X) * t, self.Y + (pt.Y - self.Y) * t, self.Z + (pt.Z - self.Z) * t)
        return p

    def rotate(self, angle):
        x = self.Z * math.sin(angle) + self.X * math.cos(angle)
        z = self.Z * math.cos(angle) + self.X * math.sin(angle)
        return Point(x, self.Y, z)

    def mid(self, pt):
        x = (self.X + pt.X) / 2
        y = (self.Y + pt.Y) / 2
        z = (self.Z + pt.Z) / 2
        p = Point(x, y, z)
        return p

