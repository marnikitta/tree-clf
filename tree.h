#ifndef TREE_H
#define TREE_H
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#ifndef timer
#define timer(msg) printf("%s %ld\n", msg, clock())
#endif

typedef struct _treeNode {
  size_t featureId;
  double threshold;
  struct _treeNode *leftChild;
  struct _treeNode *rightChild;
  size_t positiveCount;
  size_t count;
} TreeClfNode;

typedef struct {
  size_t minSampleSplit;
  size_t minSamplesLeaf;
  size_t maxDepth;
} TreeClfParams;

TreeClfNode *fit(const double ** XByColumn, const int8_t *y, size_t height, size_t width, TreeClfParams params);

void predict (const TreeClfNode *root, const double **XByColumn, size_t depth, int8_t *result);
#endif
