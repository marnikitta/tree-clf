#include "tree.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

int_fast32_t *allocVector(const size_t size);
double **allocDoubleMatrix(size_t height, size_t width);

int main() {
  size_t width;
  size_t height;

  FILE * const fp = fopen("./train_X.tsv", "r");
  fscanf(fp, "%zu %zu", &height, &width);
  double **X = (double **)allocDoubleMatrix(width, height); //store columwise
  int8_t *y = (int8_t *)calloc(height, sizeof(int8_t));

  double tmp;

  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width + 1; ++j) {
      if (j == width) {
        fscanf(fp, "%lf", &tmp);
        y[i] = tmp;
      } else {
        fscanf(fp, "%lf", &X[j][i]);
      }
    }
  }

  fclose(fp);

  TreeClfParams params = {2, 1, 5};
  TreeClfNode *root = fit(X, y, height, width, params);
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
