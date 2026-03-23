#ifndef COIN_EVALUATOR_H
#define COIN_EVALUATOR_H

/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */


/*
 * expression parser/evaluator for SoCalculator expressions.
 *
 * I chose to implement this "module" in pure C, since I think
 * mixing flex/bison code and C++ is a bad idea.
 *
 * The bison parser builds a tree structure which is traversed
 * each time the expression needs to be evaluated. I guess
 * the inputs will change more often than the expression, so I
 * think this is better than parsing the expression every time.
 *
 * Call so_eval_parse() to build the tree structure. This method
 * returns NULL if an error occurred. The actual error message
 * can be found by using so_eval_error().
 *
 * Call so_eval_evaluate() to evaluate the expression. You should
 * supply callback pointers to read and write the registers.
 * so_eval_evaluate() should never fail. If you supply an illegal
 * value to some function, it will be clamped or the result will
 * be set to some (hopefully) useful value.
 *                                              pederb, 20000307
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /* the expression node structure */
  typedef struct so_eval_node {
    int id;              /* node id */
    float value;         /* used only by value nodes */
    char regname[4];     /* register/field name in SoCalculator */
    int regidx;          /* index into vector register */
    struct so_eval_node *child1, *child2, *child3;
  } so_eval_node;

  /* callbacks are needed to read/write values from/to fields.
     When returning a float field value, write to data[0]. When
     returning a SbVec3f, set data[0], data[1] and data[2] */
  typedef struct {
    void (*readfieldcb)(const char *reg, float *data, void *userdata);

    /* 'component' only applies when setting vectors. If component < 0, copy
       all three components from data, otherwise only copy data[0] into the
       specified component (data only contains a single float */
    void (*writefieldcb)(const char *reg, float *data, int component, void *userdata);
    void *userdata;
  } so_eval_cbdata;

  /* create a tree structure from expression string */
  so_eval_node *so_eval_parse(const char *buffer); /* defined in epsilon.y */

  /* free memory used by tree structure */
  void so_eval_delete(so_eval_node *node);

  /* evaluates the tree structure */
  void so_eval_evaluate(so_eval_node *node, const so_eval_cbdata *cbdata);

  /* returns current error message, or NULL if none.
     check this after calling so_eval_parse() */
  char * so_eval_error(void); /* defined in epsilon.y */

  /* methods to create misc nodes */
  so_eval_node *so_eval_create_unary(int id, so_eval_node *topnode);
  so_eval_node *so_eval_create_binary(int id, so_eval_node *lhs, so_eval_node *rhs);
  so_eval_node *so_eval_create_ternary(int id, so_eval_node *cond,
                                       so_eval_node *branch1, so_eval_node *branch2);
  so_eval_node *so_eval_create_reg(const char *regname);
  so_eval_node *so_eval_create_reg_comp(const char *regname, int index);
  so_eval_node *so_eval_create_flt_val(float val);


/* node ids */
enum {
  ID_ADD,
  ID_ADD_VEC,
  ID_SUB,
  ID_SUB_VEC,
  ID_MUL,
  ID_DIV,
  ID_NEG,
  ID_NEG_VEC,
  ID_AND,
  ID_OR,
  ID_LEQ,
  ID_GEQ,
  ID_EQ,
  ID_NEQ,
  ID_COS,
  ID_SIN,
  ID_TAN,
  ID_ACOS,
  ID_ASIN,
  ID_ATAN,
  ID_ATAN2,
  ID_COSH,
  ID_SINH,
  ID_TANH,
  ID_SQRT,
  ID_EXP,
  ID_LOG,
  ID_LOG10,
  ID_CEIL,
  ID_FLOOR,
  ID_FABS,
  ID_FMOD,
  ID_RAND,
  ID_CROSS,
  ID_DOT,
  ID_LEN,
  ID_NORMALIZE,
  ID_TEST_FLT,
  ID_TEST_VEC,
  ID_VEC3F,
  ID_FLT_REG,
  ID_VEC_REG,
  ID_VEC_REG_COMP,
  ID_FLT_COND,
  ID_VEC_COND,
  ID_VALUE,
  ID_ASSIGN_FLT,
  ID_ASSIGN_VEC,
  ID_SEPARATOR,
  ID_NOT,
  ID_LT,
  ID_GT,
  ID_POW,
  ID_MUL_VEC_FLT,
  ID_DIV_VEC_FLT
};

/* lexical tokens */
/* do not uncomment. Bison will define these
enum {
  LEX_FLTFUNC,
  LEX_COMPARE,
  LEX_AND,
  LEX_OR,
  LEX_ATAN2,
  LEX_POW,
  LEX_FMOD,
  LEX_VEC3F,
  LEX_VALUE,
  LEX_TMP_FLT_REG,
  LEX_OUT_FLT_REG,
  LEX_IN_FLT_REG,
  LEX_TMP_VEC_REG,
  LEX_OUT_VEC_REG,
  LEX_IN_VEC_REG,
  LEX_LEN,
  LEX_ERROR
  };
*/


#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* COIN_EVALUATOR_H */
