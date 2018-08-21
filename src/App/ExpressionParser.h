#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

namespace App {

namespace ExpressionParser {

#define YYSTYPE semantic_type
#include "App/ExpressionParser.tab.h"
#undef YYTOKENTYPE
#undef YYSTYPE
#undef YYSTYPE_ISDECLARED

}
}

#endif //EXPRESSION_PARSER_H


