#include <stdlib.h>
#include <inttypes.h>
typedef struct {
  int64_t featureId;
  double threshold;
  struct _treeNode *leftChild;
  struct _treeNode *rightChild;
  size_t positiveCount;
  size_t count;
  double gini;
} TreeClfNode;

typedef struct {
  size_t minSampleSplit;
  size_t minSamplesLeaf;
  size_t maxDepth;
} TreeClfParams;

#ifndef TREE_H
#define TREE_H
TreeClfNode *fit(double ** XByColumn, const int8_t *y, size_t height, size_t widht, TreeClfParams params);

void predict (const double * const *XByColumn, size_t depth, int64_t *result);
#endif