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

#include "engines/evaluator.h"
#include <Inventor/C/basic.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h> /* NULL */
#include <float.h> /* FLT_EPSILON */

/*
 * create node, initialize all values to default
 */
static so_eval_node *
create_node(int id)
{
  so_eval_node *node = (so_eval_node*) malloc(sizeof(so_eval_node));
  node->id = id;
  node->child1 = NULL;
  node->child2 = NULL;
  node->child3 = NULL;
  node->regidx = -1;
  node->regname[0] = 'x';
  node->regname[1] = 0;
  node->value = 0.0f;
  return node;
}

/*
 * convenience method that creates a unary node
 */
so_eval_node *
so_eval_create_unary(int id, so_eval_node *topnode)
{
  so_eval_node *node = create_node(id);
  node->child1 = topnode;
  return node;
}

/*
 * convenience method that creates a binary node
 */
so_eval_node *
so_eval_create_binary(int id, so_eval_node *lhs, so_eval_node *rhs)
{
  so_eval_node *node = create_node(id);
  node->child1 = lhs;
  node->child2 = rhs;
  return node;
}

/*
 * convenience method that creates a ternary node
 */
so_eval_node *
so_eval_create_ternary(int id, so_eval_node *cond, so_eval_node *branch1,
                       so_eval_node *branch2)
{
  so_eval_node *node = create_node(id);
  node->child1 = cond;
  node->child2 = branch1;
  node->child3 = branch2;
  return node;
}

/*
 * creates a node that "references" a field (register) in the SoCalculator node.
 */
so_eval_node *
so_eval_create_reg(const char *regname)
{
  so_eval_node *node = NULL;
  int idx;

  /* find where to look for field name (upper case means vectors) */
  if (regname[0] == 't' || regname[0] == 'o') {
    idx = 1;
  }
  else idx = 0;

  if (regname[idx] >= 'a' && regname[idx] <= 'h') {
    node = create_node(ID_FLT_REG);
  }
  else if (regname[idx] >= 'A' && regname[idx] <= 'H') {
    node = create_node(ID_VEC_REG);
  }
  else {
    assert(0 && "whoa!"); /* the lexical scanner should have stopped this */
  }
  if (node) {
    node->regname[0] = regname[0];
    node->regname[1] = regname[1];
    node->regname[2] = 0;
  }
  return node;
}

/*
 * creates a node that references a component in a SoCalculator vector field.
 */
so_eval_node *
so_eval_create_reg_comp(const char *regname, int index)
{
  so_eval_node *node = create_node(ID_VEC_REG_COMP);
  node->regname[0] = regname[0];
  node->regname[1] = regname[1];
  node->regname[2] = 0;
  node->regidx = index;
  return node;
}

/*
 * creates a node that holds a float value.
 */
so_eval_node *
so_eval_create_flt_val(float val)
{
  so_eval_node *node = create_node(ID_VALUE);
  node->value = val;
  return node;
}

/*
 * used for returning values from the traverse method.
 */
typedef union {
  int trueorfalse;
  float value;
  float vec[3];
} so_eval_param;

/*
 * clamp-function
 */
static float
clamp(float val, float minval, float maxval)
{
  if (val <= minval) return minval;
  else if (val >= maxval) return maxval;
  return val;
}

/*
 * returns the dot product of the two vectors.
 */
static float
dot_product(float *v0, float *v1)
{
  return v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
}

/*
 * treverses (evaluates) the tree structure.
 */
void
so_eval_traverse(so_eval_node *node, so_eval_param *result, const so_eval_cbdata *cbdata)
{
  so_eval_param param1, param2, param3;

  if (node->id != ID_FLT_COND && node->id != ID_VEC_COND &&
      node->id != ID_ASSIGN_FLT && node->id != ID_ASSIGN_VEC) {
    if (node->child1) so_eval_traverse(node->child1, &param1, cbdata);
    if (node->child2) so_eval_traverse(node->child2, &param2, cbdata);
    if (node->child3) so_eval_traverse(node->child3, &param3, cbdata);
  }

  switch (node->id) {
  case ID_ADD:
    result->value = param1.value + param2.value;
    break;
  case ID_ADD_VEC:
    result->vec[0] = param1.vec[0] + param2.vec[0];
    result->vec[1] = param1.vec[1] + param2.vec[1];
    result->vec[2] = param1.vec[2] + param2.vec[2];
    break;
  case ID_SUB:
    result->value = param1.value - param2.value;
    break;
  case ID_SUB_VEC:
    result->vec[0] = param1.vec[0] - param2.vec[0];
    result->vec[1] = param1.vec[1] - param2.vec[1];
    result->vec[2] = param1.vec[2] - param2.vec[2];
    break;
  case ID_MUL:
    result->value = param1.value * param2.value;
    break;
  case ID_DIV:
    /* FIXME: shouldn't just silently pad over this, but rather signal
       an error. perhaps also set result->value to NaN or inf?
       -mortene. */
    if (param2.value == 0.0f) {
      result->value = param1.value / FLT_EPSILON; /* FIXME: is this ok? */
    }
    else {
      result->value = param1.value / param2.value;
    }
    break;
  case ID_FMOD:
    /* FIXME: shouldn't just silently pad over this, but rather signal
       an error. perhaps also set result->value to NaN or inf?
       -mortene. */
    if (param2.value != 0.0f) {
      result->value = (float) fmod(param1.value, param2.value);
    }
    else result->value = 0.0f;
    break;
  case ID_NEG:
    result->value = - param1.value;
    break;
  case ID_NEG_VEC:
    result->vec[0] = -param1.vec[0];
    result->vec[1] = -param1.vec[1];
    result->vec[2] = -param1.vec[2];
    break;
  case ID_AND:
    result->trueorfalse = param1.trueorfalse && param2.trueorfalse;
    break;
  case ID_OR:
    result->trueorfalse = param1.trueorfalse || param2.trueorfalse;
    break;
  case ID_LEQ:
    result->trueorfalse = param1.value <= param2.value;
    break;
  case ID_GEQ:
    result->trueorfalse = param1.value >= param2.value;
    break;
  case ID_EQ:
    result->trueorfalse = param1.value == param2.value;
    break;
  case ID_NEQ:
    result->trueorfalse = param1.value != param2.value;
    break;
  case ID_COS:
    result->value = (float)cos(param1.value);
    break;
  case ID_SIN:
    result->value = (float)sin(param1.value);
    break;
  case ID_TAN:
    result->value = (float)tan(param1.value);
    break;
  case ID_ACOS:
    result->value = (float)acos(clamp(param1.value, -1.0f, 1.0f));
    break;
  case ID_ASIN:
    result->value = (float)asin(clamp(param1.value, -1.0f, 1.0f));
    break;
  case ID_ATAN:
    result->value = (float)atan(param1.value);
    break;
  case ID_ATAN2:
    /* FIXME: shouldn't just silently pad over this, but rather signal
       an error. perhaps also set result->value to NaN or inf?
       -mortene. */
    if (param2.value == 0.0) {
      result->value = (float) (param1.value >= 0.0f ? M_PI * 0.5 : - M_PI * 0.5);
    }
    else {
      result->value = (float)atan2(param1.value, param2.value);
    }
    break;
  case ID_COSH:
    result->value = (float) cosh(param1.value);
    break;
  case ID_SINH:
    result->value = (float) sinh(param1.value);
    break;
  case ID_TANH:
    result->value = (float) tanh(param1.value);
    break;
  case ID_SQRT:
    result->value = param1.value > 0.0f ? (float) sqrt(param1.value) : 0.0f;
    break;
  case ID_EXP:
    result->value = (float) exp(param1.value);
    break;
  case ID_LOG:
    /* as value gets close to 0, the log2 goes towards -128 */
    result->value = param1.value <= 0.0f ? -128.0f : (float) log(param1.value);
    break;
  case ID_LOG10:
    /* as value gets close to 0, the log10 goes towards -38 */
    result->value = param1.value <= 0.0f ? -38.0f : (float)log10(param1.value);
    break;
  case ID_CEIL:
    result->value = (float) ceil(param1.value);
    break;
  case ID_FLOOR:
    result->value = (float) floor(param1.value);
    break;
  case ID_FABS:
    result->value = (float) fabs(param1.value);
    break;
  case ID_RAND:
    result->value = ((float)rand()) / ((float)RAND_MAX); /* [0, 1] */
    result->value *= param1.value; /* [0, arg] */
    break;
  case ID_CROSS:
    result->vec[0] = param1.vec[1]*param2.vec[2] - param1.vec[2]*param2.vec[1];
    result->vec[1] = param1.vec[2]*param2.vec[0] - param1.vec[0]*param2.vec[2];
    result->vec[2] = param1.vec[0]*param2.vec[1] - param1.vec[1]*param2.vec[0];
    break;
  case ID_DOT:
    result->value = dot_product(param1.vec, param2.vec);
    break;
  case ID_LEN:
    result->value = (float)sqrt(dot_product(param1.vec, param1.vec));
    break;
  case ID_NORMALIZE:
    {
      float len = (float) sqrt(dot_product(param1.vec, param1.vec));
      /* FIXME: shouldn't just silently pad over this, but rather
         signal an error. perhaps also set result->vec to NaNs or inf?
         -mortene. */
      if (len > 0.0f) {
        result->vec[0] = param1.vec[0] / len;
        result->vec[1] = param1.vec[1] / len;
        result->vec[2] = param1.vec[2] / len;
      }
      else {
        result->vec[0] = result->vec[1] = result->vec[2] = 0.0f;
      }
    }
    break;
  case ID_TEST_FLT:
    result->trueorfalse = param1.value != 0.0f;
    break;
  case ID_TEST_VEC:
    result->trueorfalse =
      param1.vec[0] != 0.0f ||
      param1.vec[1] != 0.0f ||
      param1.vec[2] != 0.0f;
    break;
  case ID_VEC3F:
    result->vec[0] = param1.value;
    result->vec[1] = param2.value;
    result->vec[2] = param3.value;
    break;
  case ID_FLT_REG:
    cbdata->readfieldcb(node->regname, &result->value, cbdata->userdata);
    break;
  case ID_VEC_REG:
    cbdata->readfieldcb(node->regname, result->vec, cbdata->userdata);
    break;
  case ID_VEC_REG_COMP:
    {
      float tmp[3];
      assert(node->regidx >= 0 && node->regidx <= 2);
      cbdata->readfieldcb(node->regname, tmp, cbdata->userdata);
      result->value = tmp[node->regidx];
    }
    break;
  case ID_FLT_COND:
    so_eval_traverse(node->child1, &param1, cbdata);
    so_eval_traverse(param1.trueorfalse ? node->child2 : node->child3,
                     &param2, cbdata);
    result->value = param2.value;
    break;
  case ID_VEC_COND:
    so_eval_traverse(node->child1, &param1, cbdata);
    so_eval_traverse(param1.trueorfalse ? node->child2 : node->child3,
                     &param2, cbdata);
    result->vec[0] = param2.vec[0];
    result->vec[1] = param2.vec[1];
    result->vec[2] = param2.vec[2];
    break;
  case ID_VALUE:
    result->value = node->value;
    break;
  case ID_ASSIGN_FLT:
    /* this is safe, since regidx always will be -1 for other than vector components */
    so_eval_traverse(node->child2, &param1, cbdata);
    cbdata->writefieldcb(node->child1->regname, &param1.value,
                         node->child1->regidx, cbdata->userdata);
    break;
  case ID_ASSIGN_VEC:
    so_eval_traverse(node->child2, &param1, cbdata);
    cbdata->writefieldcb(node->child1->regname, param1.vec, -1, cbdata->userdata);
    break;
  case ID_NOT:
    result->trueorfalse = ! param1.trueorfalse;
    break;
  case ID_LT:
    result->trueorfalse = param1.value < param2.value;
    break;
  case ID_GT:
    result->trueorfalse = param1.value > param2.value;
    break;
  case ID_POW:
    /* FIXME: shouldn't just silently pad over this, but rather signal
       an error. perhaps also set result->value to NaN or inf?
       -mortene. */
    if (param1.value == 0.0f) result->value = 0.0f;
    else if (param1.value > 0.0f) {
      result->value = (float) pow(param1.value, param2.value);
    }
    else { /* param1.value < 0.0, param2.value must be an integral value */
      result->value = (float) pow(param1.value, floor(param2.value + 0.5));
    }
    break;
  case ID_MUL_VEC_FLT:
    result->vec[0] = param1.vec[0] * param2.value;
    result->vec[1] = param1.vec[1] * param2.value;
    result->vec[2] = param1.vec[2] * param2.value;
    break;
  case ID_DIV_VEC_FLT:
    {
      float div = param2.value;
      /* FIXME: shouldn't just silently pad over this, but rather signal
         an error. perhaps also set result->vec to NaNs or inf?
         -mortene. */
      if (div == 0.0f) div = FLT_EPSILON;
      result->vec[0] = param1.vec[0] / div;
      result->vec[1] = param1.vec[1] / div;
      result->vec[2] = param1.vec[2] / div;
    }
    break;
  case ID_SEPARATOR:
    /* do nothing, both children have been traversed */
    break;
  default:
    assert(0 && "Whoops. Unknown node id!\n");
    break;
  }
}

/*
 * evaluates the tree structure
 */
void
so_eval_evaluate(so_eval_node *node, const so_eval_cbdata *cbdata)
{
  so_eval_param dummy;
  if (node == NULL) return;
  so_eval_traverse(node, &dummy, cbdata);
}


void
so_eval_delete(so_eval_node *node)
{
  if (node != NULL) {
    if (node->child1) so_eval_delete(node->child1);
    if (node->child2) so_eval_delete(node->child2);
    if (node->child3) so_eval_delete(node->child3);
    free(node);
  }
}
