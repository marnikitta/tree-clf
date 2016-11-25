#include "tree.h"
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <inttypes.h>

double **allocDoubleMatrix(size_t height, size_t width);

int main() {
  size_t width;
  size_t height;

  FILE * const fp = fopen("./train_X.tsv", "r");
  fscanf(fp, "%zu %zu", &height, &width);

  double **X = allocDoubleMatrix(width, height); //store columwise
  int8_t *y = (int8_t *)calloc(height, sizeof(int8_t));

  double tmp;
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width + 1; ++j) {
      if (j == width) {
        fscanf(fp, "%lf", &tmp);
        y[i] = (int8_t) tmp;
      } else {
        fscanf(fp, "%lf", X[j] + i);
      }
    }
  }

  fclose(fp);

  TreeClfParams params = {2, 1, 100};

  timer("start");
  TreeClfNode *root = fit(X, y, height, width, params);
  timer("stop");

  free(y);
  free(X[0]);
  free(X);
  return 0;
}

double **allocDoubleMatrix(size_t height, size_t width) {
  double *tmp = (double *)calloc(height * width, sizeof(double));
  double **result = (double **)calloc(height, sizeof(double *));
  for (size_t i = 0; i < height; ++i) {
    result[i] = tmp + i * width;
  }
  
  return result;
}
