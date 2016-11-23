#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "tree.h"
#include <stdbool.h>
#include <math.h>
#include <assert.h>

typedef struct {
  size_t position;
  double *value;
} SortingEntry;

SortingEntry **argsort(double **array, size_t height, size_t width);

SortingEntry **allocSortingMatrix(size_t height, size_t width);

TreeClfNode *allocBuffer(size_t size);

void iterateForFeature (
  size_t leftFeatureId, 
  size_t rightFeatureId, 
  TreeClfNode *buffer,
  SortingEntry **featureSort,
  size_t height,
  TreeClfNode **leafs);

void updateNode(const TreeClfNode *leaf, int8_t cls);

double weightedGini(const TreeClfNode *leaf);

TreeClfNode *initChildLeaves(TreeClfNode *nodes, TreeClfNode *nodesEnd);

TreeClfNode *fit (double **XByColumn, const int8_t *y, size_t height, size_t width, TreeClfParams params) {
  const size_t maxNodes = height * 2;
  SortingEntry **featureSort = argsort(XByColumn, width, height);

  TreeClfNode * const nodes = (TreeClfNode *)calloc(maxNodes, sizeof(TreeClfNode));
  TreeClfNode *nodesEnd = nodes;
  size_t aliveCount = 0;

  TreeClfNode **leafs = (TreeClfNode **)calloc(height, sizeof(TreeClfNode *));

  TreeClfNode *root = nodesEnd++;

  root->isActive = true;
  root->gini = INFINITY;
  root->count = height;
  for (size_t i = 0; i < height; ++i) {
    if (y[i] == 1) {
      root->positiveCount += 1;
    }
  }

  for (size_t i = 0; i < height; ++i) {
    leafs[i] = root;
  }

  TreeClfNode *buffer = allocBuffer(maxNodes);

  nodesEnd = initChildLeaves(nodes, nodesEnd);
  iterateForFeature(0, width, buffer, featureSort, height, leafs);

  return NULL;
};

/**
* [leftFeatureId, rightFeatureId)
* Height of buffers should be gte rightFeatureId - leftFeatureId
* Buffers should be initialized with relevant statistics for previous layer
*/
void iterateForFeature (
  size_t leftFeatureId, 
  size_t rightFeatureId, 
  TreeClfNode *buffer,
  SortingEntry **featureSort,
  size_t height,
  TreeClfNode **leafs) {
  size_t index;
  double value, prevValue = NAN;
  TreeClfNode leaf;
  double gini;

  for (size_t i = leftFeatureId; i < rightFeatureId; ++i) {
    for (size_t count = 0; count < height; ++count) {
      index = featureSort[i][count].position;
      leaf = buffer[index];
      if (leaf.isActive) {
        value = *featureSort[i][count].value;
          if (isnan(prevValue) || prevValue != value) {
            gini = weightedGini(&leaf);
          }
      }
    }
  }
}

TreeClfNode *initChildLeaves(TreeClfNode *nodes, TreeClfNode *nodesEnd) {
  TreeClfNode *newEnd = nodesEnd;
  for (TreeClfNode *i = nodes; i != nodesEnd; i += 1) {
    if (nodes->isActive) {
      i->leftChild = newEnd++;
      i->rightChild = newEnd++;
    }
  }
  return newEnd;
}

double weightedGini(const TreeClfNode *leaf) {
  const TreeClfNode *leftChild = leaf->leftChild;
  size_t leftCount = leftChild->count;
  size_t rightCount = leaf->count - leftCount;
  size_t leftPositive = leftChild->positiveCount;
  size_t rightPositive = leaf->count - leftPositive;
  double leftP = (double)leftPositive / leftCount;
  double rightP = (double) rightPositive / rightCount;
  return leftCount * leftP * (1 - leftP) + rightCount * rightP * (1 - rightP);
}

void updateNode(const TreeClfNode *leaf, int8_t cls) {
  leaf->leftChild->count++;
  leaf->leftChild->positiveCount += cls;
}

void predict(const double * const *XByColumn, size_t depth, int64_t *result) {
};

TreeClfNode *allocBuffer(size_t size) {
  return (TreeClfNode *)calloc(size, sizeof(TreeClfNode));
}

int _sortingEntryComp(const void *arg1, const void *arg2) {
  SortingEntry *e1 = (SortingEntry *)arg1;
  SortingEntry *e2 = (SortingEntry *)arg2;
  if (*(e1->value) < *(e2->value)) {
    return -1;
  } else if (*(e1->value) == *(e2->value)) {
    return 0;
  } else {
    return 1;
  }
}

SortingEntry **argsort(double **array, size_t height, size_t width) {
  SortingEntry **result = (SortingEntry **)allocSortingMatrix(height, width);

  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      result[i][j].position = j;
      result[i][j].value = &array[i][j];
    }
  }

  for (int i = 0; i < height; ++i) {
    qsort(result[i], width, sizeof(SortingEntry), &_sortingEntryComp);
  }

  return result;
}

SortingEntry **allocSortingMatrix(size_t height, size_t width) {
  SortingEntry *tmp = (SortingEntry *)calloc(height * width, sizeof(SortingEntry));
  SortingEntry **result = (SortingEntry **)calloc(height, sizeof(SortingEntry *));
  for (size_t i = 0; i < height; ++i) {
    result[i] = tmp + i * width;
  }
  
  return result;
}
