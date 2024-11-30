# -*- coding: utf8 -*-

#***************************************************************************
#*   Copyright (c) 2012 Keith Sloan <keith@sloan-home.co.uk>               *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************
__title__ = "FreeCAD OpenSCAD Workbench - CSG importer Version 0.5c"
__author__ = "Keith Sloan <keith@sloan-home.co.uk>"
__url__ = ["http://www.sloan-home.co.uk/ImportCSG"]

# Reserved words
reserved = (
    'group',
    'sphere',
    'cylinder',
    'cube',
    'multmatrix',
    'intersection',
    'difference',
    'union',
    'rotate_extrude',
    'linear_extrude',
    'true',
    'false',
    'circle',
    'square',
    'text',
    'polygon',
    'paths',
    'points',
    'undef',
    'polyhedron',
    'triangles',
    'faces',
    'render',
    'surface',
    'subdiv',
    'glide',
    'hull',
    'minkowski',
    'projection',
    'import',
    'color',
    'offset',
    'resize',
    )

# List of token names.   This is always required
tokens = reserved + (
   'WORD',
   'NUMBER',
   'LPAREN',
   'RPAREN',
   'OBRACE',
   'EBRACE',
   'OSQUARE',
   'ESQUARE',
   'COMMA',
   'SEMICOL',
   'EQ',
   'STRING',
   'ID',
   'DOT',
   'MODIFIERBACK',
   'MODIFIERDEBUG',
   'MODIFIERROOT',
   'MODIFIERDISABLE'
)

# Regular expression rules for simple tokens
t_WORD    = r'[$]?[a-zA-Z_]+[0-9]*'
t_NUMBER  = r'[-]?[0-9]*[\.]*[0-9]+([eE][+-]?[0-9]+)*'
t_LPAREN  = r'\('
t_RPAREN  = r'\)'
t_OBRACE  = r'{'
t_EBRACE  = r'\}'
t_OSQUARE = r'\['
t_ESQUARE = r'\]'
t_COMMA   = r','
t_SEMICOL = r';'
t_EQ      = r'='
t_DOT     = r'\.'
t_STRING  = r'"[^"]*"'
#t_STRING  = r'["]+[a-zA-Z.]+["]+'
t_MODIFIERBACK    = r'%'
t_MODIFIERDEBUG   = r'\#'
t_MODIFIERROOT    = r'!'
t_MODIFIERDISABLE = r'\*'
# Deal with Reserved words
reserved_map = { }
for r in reserved:
    reserved_map[r.lower()] = r


# Deal with Comments
def t_comment1(t):
    r'//[^\r\n]*((\r\n)|<<EOF>>)'
    pass


def t_comment2(t):
    r'//[^\n]*((\n)|<<EOF>>)'
    pass


def t_ID(t):
    r'[$]?[a-zA-Z_]+[0-9]*'
    t.type = reserved_map.get(t.value, "ID")
    return t


# Define a rule so we can track line numbers
def t_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)


# A string containing ignored characters (spaces and tabs)
t_ignore = " \t\r"


# Error handling rule
def t_error(t):
    print("Illegal character '%s'" % t.value[0])
    t.lexer.skip(1)
