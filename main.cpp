#include <stdlib.h>
#include <iostream>
#include "matrix.hpp"

int main() {
  srand(42);

  Matrix A = Matrix(3, 2);
  A.setUniform(-1, 1);
  std::cout << A << std::endl;

  return 0;
}
