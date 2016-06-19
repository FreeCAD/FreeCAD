# -*- coding: utf-8 -*-
################################################################################
# iso_.py
#

import nc_read as nc
import re
import sys
import math

################################################################################
class Parser(nc.Parser):
     

    def __init__(self, writer):
        nc.Parser.__init__(self, writer)

        #self.pattern_main = re.compile('(\s+|\w(?:[+])?\d*(?:\.\d*)?|\w\#\d+|\(.*?\)|\#\d+\=(?:[+])?\d*(?:\.\d*)?)')
	
	#rada
	self.pattern_main = re.compile(r'(\s+|,|-?\w\+?\d*(?:\.\d*)?|\w#\d+|\(.*?\)|#\d+=\+?\d*(?:\.\d*)?)')
	#self.pattern_main = re.compile('\s+\w')
	#self.pattern_main = re.compile('(\s+|\w(?:[+])?[+-\w]\d*(?:\.\d*)?|\w\#[+-\w]\d+|\(.*?\)|[\#[+-\w]\d+\=(?:[+])?[+-\w]\d*(?:\.\d*)?)')
	#self.pattern_main = re.compile('\s\w[\S]\w,\w[+-\w]\d*\w,\w[+-\w]\d*\w,\w[+-\w]\d*')
	#self.pattern_main = re.compile('(\s|\w(?:)?\d*(?:\.\d*)?|\w\#\d+|\(.*?\)|\#\d+\=(?:)?\d*(?:\.\d*)?)')
	#self.pattern_main = re.compile(' ')

        self.a = 0
        self.b = 0
        self.c = 0
        self.f = 0
        self.i = 0
        self.j = 0
        self.k = 0
        self.p = 0
        self.q = 0
        self.r = 0
        self.s = 0
        self.x = 0
        self.y = 0
        self.z = 500
	self.FS = 1
	self.endx=0
	self.endy=0
	self.startx=0
	self.starty=0
	self.x1=0
	self.y1=0
	self.dy=0
	self.dx=0
	self.angle=0
	self.SPACE = ' '
 
    def add_text(self, s, col=None):
        s.replace('&', '&amp;')
        s.replace('"', '&quot;')
        s.replace('<', '&lt;')
        s.replace('>', '&gt;')
	s+=self.SPACE+'\n'
        if (col != None) : self.file_out.write('\t\t<text col="'+col+'">'+s+' </text>\n')
        else : self.file_out.write('\t\t<text>'+s+' </text>\n')
    #def add_text(self, s, col=None):
    #    if (col != None) : self.file_out.write('\t\t<text col="'+col+'">'+s+'</text>\n')
    #    else : self.file_out.write('\t\t<text>'+s+'</text>\n')

    def Parse(self, name, oname=None):
        self.files_open(name,oname)

        while (self.readline()):
            self.begin_ncblock()

            move = False;
            arc = 0;
            path_col = None
	    col=None
	    if(self.line[0]=='C'): col = "axis" 
	    if(self.line[0]=='M' and self.line[1]=='A'): col = "feed"
	    if (self.line[0] == "/" and self.line[1] == "/") : col = "comment"
	    
	    if (self.FS==1 and not (self.line[0]=='S' and self.line[1]=='S') and col=="feed" ): col="rapid"
	    self.add_text(self.line, col) 
            #words = self.pattern_main.findall(self.line)
            words=self.line.split()
	    #print self.line 
	    #print ' AAAA '
	    words[0]=words[0]+self.SPACE
	    print words 
	    for word in words:
                col = None
		#if (word[0] == 'A' or word[0] == 'a'):
                #    col = "axis"
                #    self.a = eval(word[1:])
                #    move = True
                #elif (word[0] == 'B' or word[0] == 'b'):
                #   col = "axis"
                #   self.b = eval(word[1:])
                 
		#   move = True
                if (word == ('C'+self.SPACE)):
                    #print words
		    col = "axis"
		    self.startx=self.x
		    self.starty=self.y
		    words[0]=words[0]+self.SPACE
		    words[2]=self.SPACE+words[2]+self.SPACE 
		    words[4]=self.SPACE+words[4]+self.SPACE

		    #print 'x,y'
		    #print self.x
		    #print self.y
		    #self.x1=self.x-eval(words[1])
		    self.x1=self.x-eval(words[1])
		    #j=self.y-eval(words[5])
		    #self.y1=self.y-eval(words[3])
                    self.y1=self.y-eval(words[3])
		    #self.c = eval(word[1:])
		    #print 'self x,y'
		    #print self.x1
		    #print self.y1
		    self.dx=(self.x1)*1
		    self.dy=(self.y1)*0
		    #print 'x1'
		    #print self.x1
		    #print 'y1'
		    #print self.y1
		    ssucin=self.dx+self.dy
		    r=math.sqrt(((self.x1)*(self.x1))+((self.y1)*(self.y1)))
		    #print 'skalarny sucin'
		    #print ssucin
		    #print 'r'
		    #print r
		    if (ssucin!=0):
		      ratio=ssucin/(r*1)
		      #print 'ratio'
		      #print ratio
		      angle= (math.acos(ratio) * 180 / math.pi)
		      if(self.y1<0):angle=360-angle
		    elif (self.y1>0): angle=+90
		    elif (self.y1<0): angle=-90
		    else: angle=0
		    #print words[8]
		    #print 'angles'
		    #print angle
		    #if (i<0 and j<0): angle=180+angle
		    #if (i<0 and j>0): angle=180-angle  
		    #if (j>0): angle=-angle
		    #print ('reverzacia')
		    #angle= angle+ eval(words[8])
		    #print angle
		    self.angle=+ angle+ eval(words[5]) 
		    #print self.angle
		    #if(angle>180): angle=360-angle 
		    angle=self.angle*math.pi/180
		    #print eval(words[8])
		    self.endx=eval(words[1])+(r*math.cos(angle))
		    #j=eval(words[5])+(r*math.sin(angle))
		    self.endy=eval(words[3])+(r*math.sin(angle))
                    self.x=self.endx
		    self.y=self.endy
		    path_col = "feed"
		    #arc=-eval(words[8])/math.fabs(eval(words[8]))
		    arc=eval(words[5])/math.fabs(eval(words[5]))
		    #if(arc==-1): arc=0
		    #arc=-1
		    		    #col = "feed"
		    move = True


                elif (word == 'P' and words[1]=='L' and words[4]=='F'):
                    self.FS=1
		elif (word == 'P' and words[1]=='L' and words[4]=='N'):
                    self.FS=0
		elif (word == ('FS'+self.SPACE)):
                    self.FS=1
		elif (word == ('SS'+self.SPACE)):
                    self.FS=0
                elif (word == ('MA'+self.SPACE)):
		    words[2]=self.SPACE+words[2]+self.SPACE
		    if (self.FS==1):
		      path_col = "rapid"
		      col = "rapid"
		    else:
		      path_col = "feed"
		      col = "feed"
		    self.x=eval(words[1])
		    #self.y=eval(words[6])
		    self.y=eval(words[3])
		    move=True
                #elif (word == 'G1' or word == 'G01' or word == 'g1' or word == 'g01'):
                #    path_col = "feed"
                #    col = "feed"
                #elif (word == 'G2' or word == 'G02' or word == 'g2' or word == 'g02' or word == 'G12' or word == 'g12'):
                #    path_col = "feed"
                #    col = "feed"
                #    arc = -1
                #elif (word == 'G3' or word == 'G03' or word == 'g3' or word == 'g03' or word == 'G13' or word == 'g13'):
                #    path_col = "feed"
                 #   col = "feed"
                 #   arc = +1
                #elif (word == 'G10' or word == 'g10'):
		#    move = False
                #elif (word == 'L1' or word == 'l1'):
		#    move = False
                #elif (word == 'G20'):
                #    col = "prep"
                #    self.set_mode(units=25.4)
                #elif (word == 'G21'):
                #    col = "prep"
                #    self.set_mode(units=1.0)
                #elif (word == 'G81' or word == 'g81'):
                #    path_col = "feed"
                #   col = "feed"
                #elif (word == 'G82' or word == 'g82'):
                #    path_col = "feed"
                #    col = "feed"
                #elif (word == 'G83' or word == 'g83'):
                #    path_col = "feed"
                #    col = "feed"
                #elif (word[0] == 'G') : col = "prep"
                #elif (word[0] == 'I' or word[0] == 'i'):
                #    col = "axis"
                #    self.i = eval(word[1:])
                #    move = True
                #elif (word[0] == 'J' or word[0] == 'j'):
                #    col = "axis"
                #    self.j = eval(word[1:])
                #    move = True
                #elif (word[0] == 'K' or word[0] == 'k'):
                #    col = "axis"
                #    self.k = eval(word[1:])
                #    move = True
                #elif (word[0] == 'M') : col = "misc"
                #elif (word[0] == 'N') : col = "blocknum"
                #elif (word[0] == 'O') : col = "program"
                #elif (word[0] == 'P' or word[0] == 'p'):
                #    col = "axis"
                #    self.p = eval(word[1:])
                #    move = True
                #elif (word[0] == 'Q' or word[0] == 'q'):
                #    col = "axis"
                #    self.q = eval(word[1:])
                #    move = True
                #elif (word[0] == 'R' or word[0] == 'r'):
                #    col = "axis"
                #    self.r = eval(word[1:])
                #    move = True
                #elif (word[0] == 'S' or word[0zypp] == 's'):
                #    col = "axis"
                #    self.s = eval(word[1:])
                #   move = True
                #elif (word[0] == 'T') : col = "tool"
                #elif (word[0] == 'X' or word[0] == 'x'):
                #    col = "axis"
                #     = eval(word[1:])
                #    move = True
                #elif (word[0] == 'Y' or word[0] == 'y'):
                #    col = "axis"
                #    self.y = eval(word[1:])
                #    move = True
                #elif (word[0] == 'Z' or word[0] == 'z'):
                #    col = "axis"
                #    self.z = eval(word[1:])
                #    move = True
                elif (word[0] == '(') : col = "comment"
                elif (word[0] == '#') : col = "variable"
		elif (words[0] == ("//"+self.SPACE)) : col = "comment"
                elif (words[0] == ("/*"+self.SPACE)) : col = "comment"
		#self.add_text(word, col)
		
            if (move):
                self.begin_path(path_col)
                if (arc) : 
		  #self.add_arc(self.x, self.y, 0.0000, self.i, self.j, 0.0000, arc)
		  #self.add_arc(self.i, self.j, 0.0000, eval(words[2])-self.x, eval(words[5])-self.y, 0.0000, arc)
		  #print ''
		  #print eval(words[2])-self.startx
		  #print eval(words[6])-self.starty
		  #self.add_arc(self.endx, self.endy, 0.0000, eval(words[1])-self.startx, eval(words[3])-self.starty, 0.0000, arc)
		  print arc
		  self.add_arc(self.endx, self.endy, 0.0000,eval(words[1])-self.startx, eval(words[3])-self.starty, 0.0000,0.0000, arc)
		
		  #self.add_arc(self.x, self.y, 0.0000, self.i, self.j, 0.0000, arc)
		  #self.x=self.i
		  #self.y=self.j
                else     : 
		  self.add_line(self.x, self.y)
		self.end_path()

            self.end_ncblock()

        self.files_close()
