#! python
# -*- coding: utf-8 -*-
# (c) 2007 Jürgen Riegel 

from __future__ import print_function # this allows py2 to print(str1,str2) correctly

import os, errno


def ensureDir(path,mode=0o777):
	try: 
		os.makedirs(path,mode)
	except OSError(err):
		#  raise an error unless it's about an already existing directory
		print("Dir Exist")
		#if errno != 17 or not os.path.isdir(path):
		#	raise
			
def convertMultilineString(str):
	str = str.replace('\n','\\n')
	str = str.replace('"','\\"')
	return str	
	
"Yet Another Python Templating Utility, Version 1.2"

import sys

# utility stuff to avoid tests in the mainline code
class _nevermatch:
    "Polymorphic with a regex that never matches"
    def match(self, line):
        return None
_never = _nevermatch()     # one reusable instance of it suffices
def identity(string, why):
    "A do-nothing-special-to-the-input, just-return-it function"
    return string
def nohandle(string):
    "A do-nothing handler that just re-raises the exception"
    raise

# and now the real thing
class copier:
    "Smart-copier (YAPTU) class"
    def copyblock(self, i=0, last=None):
        "Main copy method: process lines [i,last) of block"
        def repl(match, self=self):
            "return the eval of a found expression, for replacement"
            # uncomment for debug: print ('!!! replacing',match.group(1))
            expr = self.preproc(match.group(1), 'eval')
            try: return str(eval(expr, self.globals, self.locals))
            except: return str(self.handle(expr))
        block = self.locals['_bl']
        if last is None: last = len(block)
        while i<last:
            line = block[i]
            match = self.restat.match(line)
            if match:   # a statement starts "here" (at line block[i])
                # i is the last line to _not_ process
                stat = match.string[match.end(0):].strip()
                j=i+1   # look for 'finish' from here onwards
                nest=1  # count nesting levels of statements
                while j<last:
                    line = block[j]
                    # first look for nested statements or 'finish' lines
                    if self.restend.match(line):    # found a statement-end
                        nest = nest - 1     # update (decrease) nesting
                        if nest==0: break   # j is first line to _not_ process
                    elif self.restat.match(line):   # found a nested statement
                        nest = nest + 1     # update (increase) nesting
                    elif nest==1:   # look for continuation only at this nesting
                        match = self.recont.match(line)
                        if match:                   # found a contin.-statement
                            nestat = match.string[match.end(0):].strip()
                            stat = '%s _cb(%s,%s)\n%s' % (stat,i+1,j,nestat)
                            i=j     # again, i is the last line to _not_ process
                    j=j+1
                stat = self.preproc(stat, 'exec')
                stat = '%s _cb(%s,%s)' % (stat,i+1,j)
                # for debugging, uncomment...: print("-> Executing: {"+stat+"}")
                exec(stat, self.globals, self.locals)
                i=j+1
            else:       # normal line, just copy with substitution
                try:
                    self.ouf.write(self.regex.sub(repl, line).encode("utf8"))
                except TypeError:
                    self.ouf.write(self.regex.sub(repl, line))
                i=i+1
    def __init__(self, regex=_never, dict={},
            restat=_never, restend=_never, recont=_never, 
            preproc=identity, handle=nohandle, ouf=sys.stdout):
        "Initialize self's attributes"
        self.regex   = regex
        self.globals = dict
        self.globals['sys'] = sys
        self.locals  = { '_cb':self.copyblock }
        self.restat  = restat
        self.restend = restend
        self.recont  = recont
        self.preproc = preproc
        self.handle  = handle
        self.ouf     = ouf
    def copy(self, block=None, inf=sys.stdin):
        "Entry point: copy-with-processing a file, or a block of lines"
        if block is None: block = inf.readlines()
        self.locals['_bl'] = block
        self.copyblock()

def replace(template,dict,file):
  "Test: copy a block of lines, with full processing"
  import re
  rex=re.compile('@([^@]+)@')
  rbe=re.compile('\+')
  ren=re.compile('-')
  rco=re.compile('= ')
  x=23 # just a variable to try substitution
  cop = copier(rex, dict, rbe, ren, rco)
  lines_block = [line+'\n' for line in template.split('\n')]
  cop.ouf = file
  cop.copy(lines_block)

if __name__=='__main__':
    "Test: copy a block of lines, with full processing"
    import re
    rex=re.compile('@([^@]+)@')
    rbe=re.compile('\+')
    ren=re.compile('-')
    rco=re.compile('= ')
    x=23 # just a variable to try substitution
    cop = copier(rex, globals(), rbe, ren, rco)
    lines_block = [line+'\n' for line in """
A first, plain line -- it just gets copied.
A second line, with @x@ substitutions.
+ x+=1   # non-block statements MUST end with comments
-
Now the substitutions are @x@.
+ if x>23:
After all, @x@ is rather large!
= else:
After all, @x@ is rather small!
-
+ for i in range(3):
  Also, @i@ times @x@ is @i*x@.
-
One last, plain line at the end.""".split('\n')]
    print("*** input:")
    print(''.join(lines_block))
    print("*** output:")
    cop.copy(lines_block)

