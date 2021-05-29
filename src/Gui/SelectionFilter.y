/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

/* Parser for the FreeCAD selection filter language */

/*  bison -o SelectionFilter.tab.c  SelectionFilter.y  */


/* Represents the many different ways we can access our data */
%union {
    std::string *string;
    Node_Object *object;
    Node_Slice  *slice;
    Node_Block  *block;
    int          token;
    int          number;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */
%token <string> TIDENTIFIER
%token <token> TSUB TSELECT TCOUNT TSLICE TNAMESPACE
%token <number> TNUMBER

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above.
 */
%type <slice> count
%type <object> matchline
%type <string> type subname
%type <block>  matchlines block

%start filter

%%



type  : TSELECT TIDENTIFIER							{ $$ = $2; }
       | TSELECT TIDENTIFIER TNAMESPACE TIDENTIFIER { $$ = StringFactory::New(*$2 + "::" + *$4); }

subname :								 { $$ = 0;  }
         | TSUB TIDENTIFIER				 { $$ = $2; }

count :									 { $$ = 0;                     }
      | TCOUNT TNUMBER TSLICE TNUMBER    { $$ = new Node_Slice($2,$4); }
      | TCOUNT TNUMBER TSLICE            { $$ = new Node_Slice($2);    }
      | TCOUNT TNUMBER                   { $$ = new Node_Slice($2,$2); }

matchline  : type subname count                { $$ = new Node_Object($1,$2,$3); }

matchlines : matchline            { $$ = new Node_Block($1);  }
           | matchlines matchline { $$ = $1 ; $$->Objects.emplace_back($2); }

block : matchlines                { $$ = $1; }

filter:   block					  { TopBlock = $1; }
;


%%
