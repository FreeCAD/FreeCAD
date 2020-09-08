
from LibLathe.LLPoint import Point
import math

class Vector:
    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.X = x
        self.Y = y
        self.Z = z
        
    def add(self, vector):
        pass
        
    def sub(self, vector):
        pass
        
    def dot(self, vector):
        pass
 
    def cross(self, vector):
       pass
             
    def length(self, vector):
        pass

    #TODO: Why rotate_x? maybe rotate about y?    
    def rotate_x(self, angle):
        #x = self.Z * math.sin(angle) + self.X * math.cos(angle)
        #z = self.Z * math.cos(angle) + self.X * math.sin(angle)
        x = self.X * math.cos(angle) - self.Z * math.sin(angle)
        z = self.X * math.sin(angle) + self.Z * math.cos(angle)
        return Vector(x, self.Y, z)
    
    #TODO why return a Point?    
    def multiply(self, val):
        v = Point(self.X * val , self.Y * val, self.Z * val)
        return v    
        
    def normalise(self, p1, p2):
        p = p2.sub(p1)
        m = math.sqrt(p.X**2 + p.Y**2 + p.Z**2)
        if m == 0:
            return Vector(0.0, 0.0, 0.0)
        else:
            return Vector(p.X/m, p.Y/m, p.Z/m)
