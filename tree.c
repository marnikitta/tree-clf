#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "tree.h"
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include <stdio.h>

typedef struct {
  size_t position;
  double *value;
} SortingEntry;

typedef struct {
  int64_t featureId;
  double threshold;
  size_t leftChild;
  size_t rightChild;
  size_t positiveCount;
  size_t count;
  double gini;
  bool isAlive;
} LearnNode;

typedef struct {
  TreeClfParams *params,
  LearnNode *nodes,
  size_t end,
  size_t maxNodes
  SortingEntry **featureSort,
  const int8_t *y,
  size_t height,
  size_t *leafs,
  size_t aliveCount,
  bool *isAlive
} CARTClf;

SortingEntry **argsort(double **array, size_t height, size_t width);

SortingEntry **allocSortingMatrix(size_t height, size_t width);

LearnNode *allocBuffer(size_t size);

void iterateForFeature (
  const CARTClf *clf
  size_t leftFeatureId, 
  size_t rightFeatureId, 
  LearnNode *result,
  LearnNode *tmpLeafs,
);

void updateNode(LearnNode *root, size_t leafId, int8_t cls);

double weightedGini(const LearnNode *root, size_t leafId);

size_t initChildLeaves(LearnNode *nodes, size_t end);

void copyStats(LearnNode *toRoot, LearnNode *fromRoot, size_t leafId);

TreeClfNode *fit (double **XByColumn, const int8_t *y, size_t height, size_t width, TreeClfParams params) {
  CARTClf clf;

  clf.maxNodes = height * 2 + 1;
  clf.featureSort = argsort(XByColumn, width, height);
  clf.nodes = allocBuffer(maxNodes);
  clf.end = 0;
  clf.aliveCount = 0;
  clf.leafs = (size_t *)calloc(height, sizeof(size_t));
  clf.height = height;
  clf.widht = width;
  clf.y = y;

  LearnNode *root = clf.nodes + clf.end;
  clf.end++;

  root->isAlive = true;
  root->count = height;

  for (size_t i = 0; i < height; ++i) {
    if (y[i] == 1) {
      root->positiveCount += 1;
    }
  }

  for (size_t i = 0; i < height; ++i) {
    clf.leafs[i] = 0;
  }

  clf.end = initChildLeaves(clf.nodes, clf.end);

  LearnNode *result = allocBuffer(maxNodes);
  memcpy(result, nodes, end * sizeof(LearnNode)); // optimaze

  LearnNode *tmpLeafs = allocBuffer(maxNodes);
  iterateForFeature(0, width, nodes, result, tmpLeafs, featureSort, y, height, leafs, maxNodes);

  printf("%zu %lf", result[0].featureId, result[0].threshold);

  return NULL;
};

/**
* [leftFeatureId, rightFeatureId)
* Height of buffers should be gte rightFeatureId - leftFeatureId
* Buffers should be initialized with relevant statistics for previous layer
*/
void iterateForFeature (
  const CARTClf *clf
  size_t leftFeatureId, 
  size_t rightFeatureId, 
  LearnNode *result,
  LearnNode *tmpLeafs,
) {

  size_t index;
  double value, prevValue = NAN;
  size_t leafId;
  LearnNode *tmpLeaf;
  LearnNode *resultLeaf;
  double gini;

  for (size_t i = leftFeatureId; i < rightFeatureId; ++i) {
    memcpy(memLeafs, )
    for (size_t count = 0; count < height; ++count) {
      index = featureSort[i][count].position;
      leafId = leafs[index];

      assert(leafId >= 0 && leafId < height * 2);

      tmpLeaf = tmpLeafs + leafId;
      resultLeaf = result + leafId;

      if (resultLeaf->isActive) {
        value = *featureSort[i][count].value;

        if (isnan(prevValue) || prevValue != value) {
          gini = weightedGini(tmpLeafs, leafId);
          if (gini < resultLeaf->gini) {
            copyStats(result, tmpLeafs, leafId);
            printf("%lf\n", gini);
          }
        }
        updateNode(tmpLeaf, leafId, y[index]);
        prevValue = value;
      }
    }
  }
}

void copyStats(LearnNode *toRoot, LearnNode *fromRoot, size_t leafId) {
  const LearnNode *fromLeaf = fromRoot + leafId;
  const LearnNode *fromLeft = fromRoot + fromLeaf->leftChild;

  size_t leftCount = fromLeft->count;
  size_t rightCount = fromLeaf->count - leftCount;
  size_t leftPositive = fromLeft->positiveCount;
  size_t rightPositive = fromLeaf->count - leftPositive;

  LearnNode *toLeaf = toRoot + leafId;
  LearnNode *toRight = toRoot + toLeaf->rightChild;
  LearnNode *toLeft = toRoot + toLeaf->leftChild;

  toLeaf->threshold = fromLeaf->threshold;
  toLeaf->featureId = fromLeaf->featureId;
  toLeaf->gini = fromLeaf->gini;
//  *toLeaf = *fromLeaf;

  toLeft->count = leftCount;
  toLeft->positiveCount = leftPositive;
  toRight->count = rightCount;
  toRight->positiveCount = rightPositive;
}

void copyLeftStats(LearnNode *toRoot, LearnNode *fromRoot, size_t leafId) {
  const LearnNode *fromLeaf = fromRoot + leafId;
  const LearnNode *fromLeft = fromRoot + fromLeaf->leftChild;

  LearnNode *toLeaf = toRoot + leafId;
  LearnNode *toLeft = toRoot + toLeaf->leftChild;

  *toLeaf = *fromLeaf;
  *toLeft = *fromLeft;
}

size_t initChildLeaves(LearnNode *nodes, size_t end) {
  size_t newEnd = end;
  for (size_t i = 0; i < end; ++i) {
    LearnNode *leaf = nodes + i;
    if (leaf->isActive) {
      leaf->gini = INFINITY;
      leaf->leftChild = newEnd++;
      leaf->rightChild = newEnd++;
    }
  }
  return newEnd;
}

double weightedGini(const LearnNode *root, size_t leafId) {
  const LearnNode *leaf = root + leafId;
  const LearnNode *leftChild = root + leaf->leftChild;
  size_t leftCount = leftChild->count;
  size_t rightCount = leaf->count - leftCount;
  size_t leftPositive = leftChild->positiveCount;
  size_t rightPositive = leaf->count - leftPositive;
  double leftP = (double)leftPositive / leftCount;
  double rightP = (double) rightPositive / rightCount;
  return leftCount * leftP * (1 - leftP) + rightCount * rightP * (1 - rightP);
}

void updateNode(LearnNode *root, size_t leafId, int8_t cls) {
  const LearnNode *leaf = root + leafId;
  LearnNode *leftChild = root + leaf->leftChild;

  leftChild->count++;
  leftChild->positiveCount += cls;
}

void predict(const double * const *XByColumn, size_t depth, int64_t *result) {
};

LearnNode *allocBuffer(size_t size) {
  return (LearnNode *)calloc(size, sizeof(LearnNode));
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
