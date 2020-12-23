/* Parser for the FreeCAD selection filter language */
/* (c) 2010 Juergen Riegel  LGPL   

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

      
		
type  : TSELECT TIDENTIFIER							{ $$ = $2 }
       | TSELECT TIDENTIFIER TNAMESPACE TIDENTIFIER { $$ = new std::string(*$2 + "::" + *$4) }

subname :								 { $$ = 0  }
		 | TSUB TIDENTIFIER				 { $$ = $2 }
		 
count :									 { $$ = 0                     }
      | TCOUNT TNUMBER TSLICE TNUMBER    { $$ = new Node_Slice($2,$4) }
      | TCOUNT TNUMBER TSLICE            { $$ = new Node_Slice($2)    }
      | TCOUNT TNUMBER                   { $$ = new Node_Slice($2,$2) }
		 
matchline  : type subname count                { $$ = new Node_Object($1,$2,$3) }

matchlines : matchline            { $$ = new Node_Block($1);  }
           | matchlines matchline { $$ = $1 ; $$->Objects.push_back($2); }
           
block : matchlines                { $$ = $1 }

filter:   block					  { TopBlock = $1 }
;


%%
