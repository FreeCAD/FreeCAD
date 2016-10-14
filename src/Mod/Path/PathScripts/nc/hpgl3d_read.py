import num_reader
import sys
import math

class Parser(num_reader.NumReader):

    def __init__(self, writer):
        num_reader.NumReader.__init__(self, writer)
        self.x = 0
        self.y = 0
        self.z = 10000
        self.f = 0
        self.units_to_mm = 0.01

    def ParseV(self):
        self.line_index = self.line_index + 1
        f = self.get_number()
        if len(f) > 0:
            self.f = float(f)
            self.add_word("prep")

    def ParseZ(self):
        self.line_index = self.line_index + 1
        x = self.get_number()
        if len(x) > 0:
            y = self.get_number()
            if len(y) > 0:
                z = self.get_number()
                if len(z) > 0:
                    if self.f > 40: color = "rapid"
                    else: color = "feed"
                    self.add_word(color)
                    self.writer.begin_path(color)
                    self.writer.add_line(int(x) * self.units_to_mm, int(y) * self.units_to_mm, int(z) * self.units_to_mm)
                    self.writer.end_path()
                    self.x = int(x)
                    self.y = int(y)
                    self.z = int(z)

    def ParseFromFirstLetter(self, c):
        if c == 'Z':
            self.ParseZ()
        elif c == 'V':
            self.ParseV()
                    
