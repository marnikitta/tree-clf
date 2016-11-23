#include <stdlib.h>
#include <inttypes.h>
#include "tree.h"
#include <stdbool.h>

typedef struct {
  size_t position;
  double *value;
} SortingEntry;

SortingEntry **argsort(double **array, size_t height, size_t width);
SortingEntry **allocSortingMatrix(size_t height, size_t width);

TreeClfNode *fit (double **XByColumn, const int8_t *y, size_t height, size_t width, TreeClfParams params) {
  const size_t maxNodes = height * 2;

  TreeClfNode * const nodes = (TreeClfNode *)calloc(maxNodes, sizeof(TreeClfNode));
  SortingEntry **featureSort = argsort(XByColumn, width, height);
  bool *isActive = (bool *)calloc(maxNodes, sizeof(bool));
  size_t aliveCount = 0;
  TreeClfNode *lastAllocated = nodes;

  TreeClfNode **leafId = (TreeClfNode **)calloc(height, sizeof(TreeClfNode *));
  return NULL;
};

/**
* [leftFeatureId, rightFeatureId)
* Height of buffers should be gte rightFeatureId - leftFeatureId
* Buffers should be initialized with relevant statistics for previous layer
*/
void _iterateForFeature (
  size_t leftFeatureId, 
  size_t rightFeatureId, 
  TreeClfNode **buffers,
  const int32_t *isActive,
  TreeClfNode **nodesByLeaf,
  size_t maxNodes,
  SortingEntry **featureSort,
  size_t height) {

  for (size_t i = leftFeatureId; i < rightFeatureId; ++i) {
    for (size_t count = 0; count < height; ++count) {
      const size_t index = featureSort[i][count].position;
    }
  }
}

void predict(const double * const *XByColumn, size_t depth, int64_t *result) {
};

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
