#ifndef ARITH_TREEFIX_H
#define ARITH_TREEFIX_H

#include "arith-parser.h"

/* Fix an arithmetic tree generated by the LL parser */
struct Tree *fix(struct Tree *tree);

#endif
