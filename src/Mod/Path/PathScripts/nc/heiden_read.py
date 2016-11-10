################################################################################
# heiden_read.py
#
# Simple ISO NC code parsing
#

import nc_read as nc
import re
import sys
import math

################################################################################
class Parser(nc.Parser):

    def __init__(self, writer):
        nc.Parser.__init__(self, writer)

        self.pattern_main = re.compile('([(!;].*'
                                       '|\s+|[a-zA-Z0-9_:](?:[+-])?\d*(?:\.\d*)?'
                                       '|\w\#\d+|\(.*?\)'
                                       '|\#\d+\=(?:[+-])?\d*(?:\.\d*)? )')
        self.pattern_tool = re.compile('([(!;].*'
                                       '|\S+'
                                       '|\s+|\d)')
        self.oldx = 0
        self.oldy = 0
        self.oldz = 150
        self.olda = 0
        self.oldb = 0
        self.oldc = 0
        
    def ParseTool(self, word):
        # parse the first numeric parameter that comes after 'tool call'
        try:
            if (word[0:] == 'TOOL'):
                self.col = "tool"
                self.move = False
            elif (word[0:] == 'CALL'):
                self.col = "tool call"
                self.move = False
            elif self.col == 'tool call':
                if not self.t > 0:
                    if word[0] >= '0' and word[0] <= '9':
                        self.t = eval(word[0:])
                        self.col = 'tool no'
        except:
            pass
        
    def change_tool(self, t):
        pass
       
    def ParseWord(self, word):
        try:
            if (word[0] == 'A' or word[0] == 'a'):
                self.col = "axis"
                self.a = eval(word[1:])
                self.move = True
            elif (word[0] == 'B' or word[0] == 'b'):
                self.col = "axis"
                self.b = eval(word[1:])
                self.move = True
            elif (word[0] == 'C' or word[0] == 'c'):
                self.col = "axis"
                self.c = eval(word[1:])
                self.move = True
            elif (word[0] == 'F' or word[0] == 'f'):
                self.col = "axis"
                if word[1:] == 'FMAX':
                    self.rapid = True
                    self.path_col = "rapid"
                    self.col = "rapid"
                else:
                    self.f = eval(word[1:])
                    self.rapid = False
                    self.path_col = "feed"
                    self.col = "feed"
                self.move = True
            elif (word == 'L' or word == 'l'):
                self.arc = 0
                self.move = True
            elif (word[0] == 'X' or word[0] == 'x'):
                self.col = "axis"
                self.x = eval(word[1:])
                self.move = True
            elif (word[0] == 'Y' or word[0] == 'y'):
                self.col = "axis"
                self.y = eval(word[1:])
                self.move = True
            elif (word[0] == 'Z' or word[0] == 'z'):
                self.col = "axis"
                self.z = eval(word[1:])
                self.move = True
                self.t = eval(word[1:])
        except:
            pass
        
    def Parsey(self, name):
        self.files_open(name)
        
        for_full_machine_sim = True # to do, make derived class to do this
        #for_full_machine_sim = False
        
        self.f = None
        self.arc = 0
        self.rapid = True
        
        while (self.readline()):
            
            self.a = None
            self.b = None
            self.c = None
            self.h = None
            self.i = None
            self.j = None
            self.k = None
            self.p = None
            self.q = None
            self.r = None
            self.s = None
            self.x = None
            self.y = None
            self.z = None
            self.t = 0

            self.move = False
            self.no_move = False
            
            words = self.pattern_tool.findall(self.line)
            for word in words:
                self.ParseTool(word)

            if not self.t:
                words = self.pattern_main.findall(self.line)
                for word in words:
                    self.ParseWord(word)

            if (self.move and not self.no_move):
                if (self.arc==0):
                    self.add_line(self.x, self.y, self.z, self.a, self.b, self.rapid)
                else:
                    self.add_arc(self.x, self.y, self.z, self.i, self.j, self.k, self.r, self.arc)
                if self.x != None: self.oldx = self.x
                if self.y != None: self.oldy = self.y
                if self.z != None: self.oldz = self.z
                if self.a != None: self.olda = self.a
                if self.b != None: self.oldb = self.b
                if self.c != None: self.oldc = self.c
                
            elif (self.t):
                self.change_tool(self.t)

        self.files_close()
