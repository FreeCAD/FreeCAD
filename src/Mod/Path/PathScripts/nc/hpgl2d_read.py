import num_reader
import sys
import math

class Parser(num_reader.NumReader):

    def __init__(self, writer):
        num_reader.NumReader.__init__(self, writer)
        self.i = 0
        self.j = 0
        self.x = 0
        self.y = 0
        self.down_z = 0
        self.up_z = 20
        self.up = True
        self.units_to_mm = 0.01

    def ParsePuOrPd(self, up):
        self.line_index = self.line_index + 1
        x = self.get_number()
        if len(x) > 0:
            y = self.get_number()
            if len(y) > 0:
                if up: color = "rapid"
                else: color = "feed"
                self.add_word(color)
                self.writer.begin_path(color)
                if up: z = self.up_z
                else: z = self.down_z
                if self.up != up:
                    self.writer.add_line(self.x * self.units_to_mm, self.y * self.units_to_mm, z)
                self.writer.add_line(int(x) * self.units_to_mm, int(y) * self.units_to_mm, z)
                self.writer.end_path()
                self.up = up
                self.x = int(x)
                self.y = int(y)
    
    def ParseAA(self):
        self.line_index = self.line_index + 1
        cx = self.get_number()
        if len(cx) > 0:
            cy = self.get_number()
            if len(cy) > 0:
                a = self.get_number()
                if len(a) > 0:
                    self.add_word("feed")
                    self.writer.begin_path("feed")
                    z = self.down_z
                    if self.up:
                        self.writer.add_line(self.x * self.units_to_mm, self.y * self.units_to_mm, z)

                    sdx = self.x - int(cx)
                    sdy = self.y - int(cy)

                    start_angle = math.atan2(sdy, sdx)

                    end_angle = start_angle + int(a) * math.pi/180

                    radius = math.sqrt(sdx*sdx + sdy*sdy)

                    ex = int(cx) + radius * math.cos(end_angle)
                    ey = int(cy) + radius * math.sin(end_angle)
                    
                    if int(a) > 0: d = 1
                    else: d = -1

                    self.writer.add_arc(ex * self.units_to_mm, ey * self.units_to_mm, 0.0, i = int(-sdx) * self.units_to_mm, j = int(-sdy) * self.units_to_mm, d = d)
                    self.writer.end_path()
                    self.up = False
                    self.x = int(ex)
                    self.y = int(ey)
    
    def ParseFromFirstLetter(self, c):
        if c == 'P':
            self.line_index = self.line_index + 1
            if self.line_index < self.line_length:
                c1 = self.line[self.line_index]
                self.parse_word += c1
                if c1 == 'U': # PU
                    self.ParsePuOrPd(True)
                elif c1 == 'D': # PD
                    self.ParsePuOrPd(False)
        elif c == 'A':
            self.line_index = self.line_index + 1
            if self.line_index < self.line_length:
                c1 = self.line[self.line_index]
                self.parse_word += c1
                if c1 == 'A': # AA, arc absolute
                    self.ParseAA()

