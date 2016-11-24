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
  const double *value;
} SortingEntry;

typedef struct {
  size_t featureId;
  double threshold;
  size_t leftChild;
  size_t rightChild;
  size_t positiveCount;
  size_t count;
  double gini;
  int8_t isAlive;
} LearnNode;

typedef struct {
  TreeClfParams *params;
  LearnNode *nodes;
  size_t end;
  size_t maxNodes;
  SortingEntry **featureSort;
  const int8_t *y;
  const double **X;
  size_t height;
  size_t width;
  size_t *leafs;
  size_t aliveCount;
} CARTClf;

void debug(CARTClf clf, LearnNode *nodes) {
  for (size_t j = 0; j < clf.end; ++j) {
    printf("isAlive[%zu] = %d ", j, nodes[j].isAlive);
    printf("gini = %lf ", nodes[j].gini);
    printf("count = %zu ", nodes[j].count);
    printf("positive count = %zu ", nodes[j].positiveCount);
    printf("featureId = %zu ", nodes[j].featureId);
    printf("threshold = %lf ", nodes[j].threshold);
    printf("left = %zu ", nodes[j].leftChild);
    printf("right = %zu ", nodes[j].rightChild);
    int tmpCnt = 0;
    for (size_t m = 0; m < clf.height; ++m) {
      if (clf.leafs[m] == j) {
        tmpCnt++;
      }
    }
    printf("leaf = %d \n", tmpCnt);
  }
  printf("###\n");
  fflush(stdout);
}

SortingEntry **argsort(const double **array, size_t height, size_t width);

SortingEntry **allocSortingMatrix(size_t height, size_t width);

LearnNode *allocBuffer(size_t size);

void iterateForFeature(
        const CARTClf *clf,
        size_t leftFeatureId,
        size_t rightFeatureId,
        LearnNode *result,
        LearnNode *tmpLeafs
);

void updateNode(LearnNode *root, size_t leafId, int8_t cls);

double weightedGini(const LearnNode *root, size_t leafId);

size_t initChildLeaves(LearnNode *nodes, size_t end);

void copyStats(LearnNode *toRoot, LearnNode *fromRoot, size_t leafId);

void updateLeafLiveliness(CARTClf *clf);

void updateLeafIds(CARTClf *clf);

CARTClf init(const double **X, const int8_t *y, size_t height, size_t width, TreeClfParams *params) {
  CARTClf clf;

  clf.params = params;
  clf.maxNodes = height * 4 + 1;
  timer("Before sort");
  clf.featureSort = argsort(X, width, height);
  timer("After sort");
  clf.nodes = allocBuffer(clf.maxNodes);
  clf.end = 0;
  clf.aliveCount = 1;
  clf.leafs = (size_t *) calloc(height, sizeof(size_t));
  clf.height = height;
  clf.width = width;
  clf.y = y;
  clf.X = X;

  LearnNode *root = clf.nodes + clf.end;
  clf.end++;

  root->isAlive = 1;
  root->count = height;

  for (size_t i = 0; i < height; ++i) {
    if (y[i] == 1) {
      root->positiveCount += 1;
    }

    clf.leafs[i] = 0;
  }
  return clf;
}

TreeClfNode *fit(const double **XByColumn, const int8_t *y, size_t height, size_t width, TreeClfParams params) {
  CARTClf clf = init(XByColumn, y, height, width, &params);

  //###

  LearnNode *result = allocBuffer(clf.maxNodes);
  LearnNode *tmpLeafs = allocBuffer(clf.maxNodes);


  int depth = 1;
  while (clf.aliveCount > 0 && depth++ < clf.params->maxDepth) {
    clf.end = initChildLeaves(clf.nodes, clf.end);
    memcpy(result, clf.nodes, clf.end * sizeof(LearnNode));

    iterateForFeature(&clf, 0, width, result, tmpLeafs);

    memcpy(clf.nodes, result, clf.end * sizeof(LearnNode));

    updateLeafIds(&clf);
    updateLeafLiveliness(&clf);
  }

  //###

  return NULL;
};

void updateLeafLiveliness(CARTClf *clf) {
  LearnNode *leaf;
  LearnNode *leftChild, *rightChild;
  clf->aliveCount = 0;
  for (size_t i = 0; i < clf->end; ++i) {
    leaf = clf->nodes + i;
    if (leaf->isAlive == 1) {
      leaf->isAlive = 0;
      leftChild = clf->nodes + leaf->leftChild;
      rightChild = clf->nodes + leaf->rightChild;

      if (leftChild->count > clf->params->minSamplesLeaf) {
        clf->aliveCount++;
        leftChild->isAlive = 2;
      }
      if (rightChild->count > clf->params->minSamplesLeaf) {
        clf->aliveCount++;
        rightChild->isAlive = 2;
      }
    }
  }

  for (size_t i = 0; i < clf->end; ++i) {
    leaf = clf->nodes + i;
    if (leaf->isAlive == 2) {
      leaf->isAlive = 1;
    }
  }
}

void updateLeafIds(CARTClf *clf) {
  LearnNode *leaf;

  for (size_t i = 0; i < clf->height; ++i) {
    leaf = clf->nodes + clf->leafs[i];
    if (leaf->isAlive) {
      if (clf->X[leaf->featureId][i] < leaf->threshold) {
        clf->leafs[i] = leaf->leftChild;
      } else {
        clf->leafs[i] = leaf->rightChild;
      }
    }
  }
}

/**
* [leftFeatureId, rightFeatureId)
* Height of buffers should be gte rightFeatureId - leftFeatureId
* Buffers should be initialized with relevant statistics for previous layer
*/
void iterateForFeature(
        const CARTClf *clf,
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

  for (size_t featureId = leftFeatureId; featureId < rightFeatureId; ++featureId) {
    memcpy(tmpLeafs, clf->nodes, clf->end * sizeof(LearnNode));

    for (size_t elementId = 0; elementId < clf->height; ++elementId) {
      index = clf->featureSort[featureId][elementId].position;
      leafId = clf->leafs[index];

      assert(leafId >= 0 && leafId < clf->maxNodes);


      if ((clf->nodes + leafId)->isAlive) {
        value = *(clf->featureSort[featureId][elementId].value);
        if (isnan(prevValue) || prevValue != value) {
          tmpLeaf = tmpLeafs + leafId;
          gini = weightedGini(tmpLeafs, leafId);

          resultLeaf = result + leafId;
          if (!isnan(gini) && gini < resultLeaf->gini) {
            tmpLeaf->featureId = featureId;
            tmpLeaf->threshold = value;
            tmpLeaf->gini = gini;
            copyStats(result, tmpLeafs, leafId);
          }
        }

        updateNode(tmpLeafs, leafId, clf->y[index]);
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

SortingEntry **argsort(const double **array, size_t height, size_t width) {
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
