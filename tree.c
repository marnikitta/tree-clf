#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "tree.h"
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include <stdio.h>
#include <time.h>

typedef struct {
  size_t position;
  double *value;
} SortingEntry;

typedef struct {
  size_t featureId;
  double threshold;
  size_t leftChild;
  size_t rightChild;
  size_t positiveCount;
  size_t count;
  double gini;
  bool isAlive;
} LearnNode;

typedef struct {
  TreeClfParams *params;
  LearnNode *nodes;
  size_t end;
  size_t maxNodes;
  SortingEntry **featureSort;
  const int8_t *y;
  size_t height;
  size_t width;
  size_t *leafs;
  size_t aliveCount;
  bool *isAlive;
} CARTClf;

SortingEntry **argsort(double **array, size_t height, size_t width);

SortingEntry **allocSortingMatrix(size_t height, size_t width);

LearnNode *allocBuffer(size_t size);

void iterateForFeature(
        const CARTClf *self,
        size_t leftFeatureId,
        size_t rightFeatureId,
        LearnNode *result,
        LearnNode *tmpLeafs
);

void updateNode(LearnNode *root, size_t leafId, int8_t cls);

double weightedGini(const LearnNode *root, size_t leafId);

size_t initChildLeaves(LearnNode *nodes, size_t end);

void copyStats(LearnNode *toRoot, LearnNode *fromRoot, size_t leafId);

TreeClfNode *fit(double **XByColumn, const int8_t *y, size_t height, size_t width, TreeClfParams params) {
  CARTClf clf;

  clf.maxNodes = height * 2 + 1;
  timer("Before sort");
  clf.featureSort = argsort(XByColumn, width, height);
  timer("After sort");
  clf.nodes = allocBuffer(clf.maxNodes);
  clf.end = 0;
  clf.aliveCount = 0;
  clf.leafs = (size_t *) calloc(height, sizeof(size_t));
  clf.height = height;
  clf.width = width;
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

  LearnNode *result = allocBuffer(clf.maxNodes);
  memcpy(result, clf.nodes, clf.end * sizeof(LearnNode)); // optimaze

  LearnNode *tmpLeafs = allocBuffer(clf.maxNodes);

  timer("Before iterate");
  iterateForFeature(&clf, 0, width, result, tmpLeafs);

  printf("%zu %lf\n", result[0].featureId, result[0].threshold);

  return NULL;
};

/**
* [leftFeatureId, rightFeatureId)
* Height of buffers should be gte rightFeatureId - leftFeatureId
* Buffers should be initialized with relevant statistics for previous layer
*/
void iterateForFeature(
        const CARTClf *self,
        size_t leftFeatureId,
        size_t rightFeatureId,
        LearnNode *result,
        LearnNode *tmpLeafs
) {

  size_t index;
  double value, prevValue = NAN;
  size_t leafId;
  LearnNode *tmpLeaf;
  LearnNode *resultLeaf;
  double gini;

  for (size_t i = leftFeatureId; i < rightFeatureId; ++i) {
    memcpy(tmpLeafs, self->nodes, self->end * sizeof(LearnNode));

    for (size_t count = 0; count < self->height; ++count) {
      index = self->featureSort[i][count].position;
      leafId = self->leafs[index];

      assert(leafId >= 0 && leafId < self->maxNodes);

      tmpLeaf = tmpLeafs + leafId;

      if (self->nodes->isAlive) {
        value = *(self->featureSort[i][count].value);

        if (isnan(prevValue) || prevValue != value) {
          gini = weightedGini(tmpLeafs, leafId);

          resultLeaf = result + leafId;
          if (gini < resultLeaf->gini) {
            tmpLeaf->featureId = i;
            tmpLeaf->threshold = value;
            tmpLeaf->gini = gini;
            copyStats(result, tmpLeafs, leafId);
          }
        }
        updateNode(tmpLeaf, leafId, self->y[index]);
        prevValue = value;
      }
    }
  }
}

void copyStats(LearnNode *toRoot, LearnNode *fromRoot, size_t leafId) {
  const LearnNode *fromLeaf = fromRoot + leafId;
  const LearnNode *fromLeft = fromRoot + fromLeaf->leftChild;
  const LearnNode *fromRight = fromRoot + fromLeaf->rightChild;

  LearnNode *toLeaf = toRoot + leafId;
  LearnNode *toLeft = toRoot + toLeaf->leftChild;
  LearnNode *toRight = toRoot + toLeaf->rightChild;

  *toLeaf = *fromLeaf;
  *toLeft = *fromLeft;
  *toRight = *fromRight;
}

size_t initChildLeaves(LearnNode *nodes, size_t end) {
  LearnNode *rightChild;

  size_t newEnd = end;
  for (size_t i = 0; i < end; ++i) {
    LearnNode *leaf = nodes + i;
    if (leaf->isAlive) {
      leaf->gini = INFINITY;
      leaf->leftChild = newEnd++;
      leaf->rightChild = newEnd++;

      rightChild = nodes + leaf->rightChild;
      rightChild->positiveCount = leaf->positiveCount;
      rightChild->count = leaf->count;
    }
  }
  return newEnd;
}

double weightedGini(const LearnNode *root, size_t leafId) {
  const LearnNode *leaf = root + leafId;
  const LearnNode *leftChild = root + leaf->leftChild;
  const LearnNode *rightChild = root + leaf->rightChild;

  assert(leftChild->count + rightChild->count == leaf->count);
  assert(leftChild->positiveCount + rightChild->positiveCount == leaf->positiveCount);

  double leftP = (double) leftChild->positiveCount / leftChild->count;
  double rightP = (double) rightChild->positiveCount / rightChild->count;
  return leftChild->count * leftP * (1 - leftP) + rightChild->count * rightP * (1 - rightP);
}

void updateNode(LearnNode *root, size_t leafId, int8_t cls) {
  const LearnNode *leaf = root + leafId;
  LearnNode *leftChild = root + leaf->leftChild;
  LearnNode *rightChild = root + leaf->rightChild;

  leftChild->count++;
  leftChild->positiveCount += cls;

  rightChild->count--;
  rightChild->positiveCount -= cls;
}

void predict(const double *const *XByColumn, size_t depth, int64_t *result) {
};

LearnNode *allocBuffer(size_t size) {
  return (LearnNode *) calloc(size, sizeof(LearnNode));
}

int _sortingEntryComp(const void *arg1, const void *arg2) {
  SortingEntry *e1 = (SortingEntry *) arg1;
  SortingEntry *e2 = (SortingEntry *) arg2;
  if (*(e1->value) < *(e2->value)) {
    return -1;
  } else if (*(e1->value) == *(e2->value)) {
    return 0;
  } else {
    return 1;
  }
}

SortingEntry **argsort(double **array, size_t height, size_t width) {
  SortingEntry **result = allocSortingMatrix(height, width);

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
  SortingEntry *tmp = (SortingEntry *) calloc(height * width, sizeof(SortingEntry));
  SortingEntry **result = (SortingEntry **) calloc(height, sizeof(SortingEntry *));
  for (size_t i = 0; i < height; ++i) {
    result[i] = tmp + i * width;
  }

  return result;
}
