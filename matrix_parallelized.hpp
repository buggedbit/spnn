#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <spdlog/spdlog.h>
#include <math.h>
#include <stdlib.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <utility>

bool USE_MATRIX_NAMES = true;
#define THREADS_PER_BLOCK 1024

__global__ void matmul( float *a, float *b, float *c , int num_rows, int num_cols,int int_dim) {
    int index = threadIdx.x + blockIdx.x * blockDim.x;
    int x_ind = index/(num_cols), y_ind = index%(num_cols);
    if(x_ind < num_rows && y_ind < num_cols){
        float sum = 0;
        for(int i=0; i < int_dim; i++){
            sum += a[x_ind*(int_dim) + i]*b[i*(num_cols) + y_ind];
        }
        c[index] = sum;
    }

}

class Matrix {
  std::vector<std::vector<float> > values;
  friend std::ostream& operator<<(std::ostream&, const Matrix&);

public:
  std::string name;
  const int nR, nC;

  Matrix(int r, int c, std::string name = USE_MATRIX_NAMES ? "<unnamed-matrix>" : ""): nR(r), nC(c), name(name) {
    spdlog::debug("Matrix {}: constructor called", name.c_str());
    values.resize(nR);
    for (auto& row: values) {
      row.resize(nC);
    }
  }

  Matrix(const Matrix& m) : nR(m.nR), nC(m.nC), name(USE_MATRIX_NAMES ? "(" + m.name + ")_copy" : "") {
    spdlog::debug("Matrix {}: copy constructor called", name.c_str());
    values.resize(nR);
    for (auto& row: values) {
      row.resize(nC);
    }
    // deep copy the values variable
    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        values[i][j] = m.values[i][j];
      }
    }
  }

  void operator=(Matrix const &m) {
    if (m.nC != nC || m.nR != nR) {
      std::stringstream ss;
      ss <<  "Invalid operation: Attempt to assign a matrix with different dimensions: Trying to assign "
         << m.name
         << "(" << m.nR << ", " << m.nC << ")"
         "to matrix"
         << name
         << "(" << nR << ", " << nC << ")";
      throw ss.str();
    }

    name = USE_MATRIX_NAMES ? "(" + m.name + ")_copy" : "";
    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        values[i][j] = m.values[i][j];
      }
    }
  }

  ~Matrix() {
    spdlog::debug("Matrix {}: destructor called", name.c_str());
  }

  int getNumElements() const { return nR * nC; }

  void printDims() const { spdlog::info("nR = {}, nC = {}",nR,nC); }

  float get(const int& i, const int& j) const {
    return values[i][j];
  }

  float get(const std::pair<int, int>& index) const {
    return this->get(index.first, index.second);
  }

  float& at(const int& i, const int& j) {
    return values[i][j];
  }

  std::pair<int, int> argmax() const {
    std::vector<int> argmaxCols;
    std::vector<float> rowwiseMaxs;
    for (int i = 0; i < nR; ++i) {
      auto row = values[i];
      int argmaxCol = std::max_element(row.begin(), row.end()) - row.begin();
      float maxInRow = *std::max_element(row.begin(), row.end());
      argmaxCols.push_back(argmaxCol);
      rowwiseMaxs.push_back(maxInRow);
    }
    int argmaxRow = std::max_element(rowwiseMaxs.begin(), rowwiseMaxs.end()) - rowwiseMaxs.begin();
    int argmaxCol = argmaxCols[argmaxRow];

    return std::pair<int, int>(argmaxRow, argmaxCol);
  }

  std::pair<int, int> colmax(int col) const {
    float ans = -10000000;
    int max_ind = -1;
    for (int i = 0; i < nR; ++i) {
      if(values[i][col] > ans){
        max_ind = i;
        ans = values[i][col];
      }
    }
    return std::pair<int, int>(max_ind, col);
  }

  Matrix* setZeros() {
    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        values[i][j] = 0;
      }
    }
    return this;
  }

  Matrix* setOnes() {
    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        values[i][j] = 1;
      }
    }
    return this;
  }

  Matrix* setIdentity() {
    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        values[i][j] = (i == j);
      }
    }
    return this;
  }

  Matrix* setUniform(float low, float high) {
    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        float randomNumber = (float) rand() / RAND_MAX;
        randomNumber = low + randomNumber * (high - low);
        values[i][j] = randomNumber;
      }
    }
    return this;
  }

  Matrix sigmoid() const {
    std::stringstream ss;
    ss << "(" << name << ")_SigmoidActivation";
    Matrix result(nR, nC, ss.str());

    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        result.values[i][j] = 1 / (1 + exp(-this->values[i][j]));
      }
    }

    return result;
  }

  Matrix sigmoidDerivative() const {
    std::stringstream ss;
    ss << "(" << name << ")_SigmoidDerivative";
    Matrix result(nR, nC, USE_MATRIX_NAMES ? ss.str() : "");

    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        result.values[i][j] = 1 / (1 + exp(-this->values[i][j]));
        result.values[i][j] = result.values[i][j] - (result.values[i][j]*result.values[i][j]);
      }
    }

    return result;
  }

  Matrix softmax() const {
    std::stringstream ss;
    ss << "(" << name << ")_Softmax";
    Matrix result(nR, nC, USE_MATRIX_NAMES ? ss.str() : "");

    for (int j = 0; j < nC; ++j) {
      float sum = 0;
      for (int i = 0; i < nR; ++i) {
        sum += exp(this->values[i][j]);
      }
      for (int i = 0; i < nR; ++i) {
        result.values[i][j] = exp(this->values[i][j]) / sum;
      }
    }

    return result;
  }

  Matrix operator~() const {
    std::stringstream ss;
    ss << "(" << name << ")_Transpose";
    Matrix result(nC, nR, USE_MATRIX_NAMES ? ss.str() : "");

    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        result.values[j][i] = this->values[i][j];
      }
    }

    return result;
  }

  Matrix operator+(Matrix const &m) const {
    if (m.nR == 1 && nC == m.nC) {
      std::stringstream ss;
      ss << name << " + " << m.name;
      Matrix result(nR, nC, USE_MATRIX_NAMES ? ss.str() : "");

      for (int i = 0; i < nR; ++i) {
        for (int j = 0; j < nC; ++j) {
          result.values[i][j] = this->values[i][j] + m.values[0][j];
        }
      }
      return result;

    }

    if (nR != m.nR || nC != m.nC) {
      std::stringstream ss;
      ss <<  "Invalid dimensions for matrix addition: Candidates are matrices "
        << name << "(" << nR << "," << nC << ")"
        << " and "
        << m.name << "(" << m.nR << "," << m.nC << ")";
      throw ss.str();
    }

    std::stringstream ss;
    ss << name << " + " << m.name;
    Matrix result(nR, nC, USE_MATRIX_NAMES ? ss.str() : "");

    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        result.values[i][j] = this->values[i][j] + m.values[i][j];
      }
    }

    return result;
  }

  Matrix operator-(Matrix const &m) const {
    if (nR != m.nR || nC != m.nC) {
      std::stringstream ss;
      ss <<  "Invalid dimensions for matrix subtraction: Candidates are matrices "
        << name << "(" << nR << "," << nC << ")"
        << " and "
        << m.name << "(" << m.nR << "," << m.nC << ")";
      throw ss.str();
    }

    std::stringstream ss;
    ss << name << " - " << m.name;
    Matrix result(nR, nC, USE_MATRIX_NAMES ? ss.str() : "");

    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        result.values[i][j] = this->values[i][j] - m.values[i][j];
      }
    }

    return result;
  }


  Matrix operator*(Matrix const &m) const {
    if (nC != m.nR) {
      std::stringstream ss;
      ss <<  "Invalid dimensions for matrix multiplication: Candidates are matrices "
        << name << "(" << nR << "," << nC << ")"
        << " and "
        << m.name << "(" << m.nR << "," << m.nC << ")";
      throw ss.str();
    }

    float *a, *b, *c; // host copies of a, b, c
    float *dev_a, *dev_b, *dev_c; // device copies of a, b, c
    int sz1 = nR*nC * sizeof(float), sz2 = m.nR * m.nC * sizeof(float), sz3 = nR*m.nC * sizeof(float);

    // allocate device copies of a, b, c
    cudaMalloc( (void**)&dev_a, sz1 );
    cudaMalloc( (void**)&dev_b, sz2 );
    cudaMalloc( (void**)&dev_c, sz3 );
    a = (float *)malloc( sz1 );
    b = (float *)malloc( sz2 );
    c = (float *)malloc( sz3 );

    for(int i = 0; i < nR*m.nC; i++){c[i] = 0;}
    for(int i = 0; i < nR; i++){
      for(int j = 0; j < nC; j++){
        a[i*nC+j] = values[i][j];
      }
    }
    for(int i = 0; i < m.nR; i++){
      for(int j = 0; j < m.nC; j++){
        b[i*m.nC+j] = m.values[i][j];
      }
    }
    // copy inputs to device
    cudaMemcpy( dev_a, a, sz1,cudaMemcpyHostToDevice );
    cudaMemcpy( dev_b, b, sz2,cudaMemcpyHostToDevice );
    cudaMemcpy( dev_c, c, sz3,cudaMemcpyHostToDevice );

    int num_blocks = ( nR*m.nC + THREADS_PER_BLOCK)/THREADS_PER_BLOCK;
    matmul<<<num_blocks,THREADS_PER_BLOCK>>>(dev_a,dev_b,dev_c,nR,m.nC,nC);

    // copy device result back to host copy of c
    cudaMemcpy( c, dev_c, sz3 , cudaMemcpyDeviceToHost );
    
    cudaDeviceSynchronize();


    std::stringstream ss;
    ss << name << " * " << m.name;
    Matrix result(nR, m.nC, USE_MATRIX_NAMES ? ss.str() : "");

    // std::cout<<"\n";
    for(int i = 0; i < nR; i++){
      for(int j = 0; j < m.nC; j++){
        result.at(i,j) = c[i*m.nC + j];
        // std::cout<<c[i*m.nC + j]<<" ";
      }
      // std::cout<<"\n";
    }
    // std::cout<<"\n";

    free( a ); free(  b );
    cudaFree( dev_a );
    cudaFree( dev_b );
    cudaFree( dev_c );
    free(c);
    // exit(0);
    return result;
  }


  Matrix operator*(float const &value) const {
    std::stringstream ss;
    ss << name << " * " << "const(" << value << ")";
    Matrix result(nR, nC, USE_MATRIX_NAMES ? ss.str() : "");

    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        result.values[i][j] = this->values[i][j] * value;
      }
    }

    return result;
  }


  Matrix operator%(Matrix const &m) const {
    if (m.nC == 1 && nR == m.nR) {
      std::stringstream ss;
      ss << name << " % " << m.name;
      Matrix result(nR, nC, USE_MATRIX_NAMES ? ss.str() : "");

      for (int i = 0; i < nR; ++i) {
        for (int j = 0; j < nC; ++j) {
          result.values[i][j] = this->values[i][j] * m.values[i][0];
        }
      }

      return result;
    }

    if (nC != m.nC || nR != m.nR) {
      std::stringstream ss;
      ss <<  "Invalid dimensions for matrix element wise multiplication: Candidates are matrices "
        << name << "(" << nR << "," << nC << ")"
        << " and "
        << m.name << "(" << m.nR << "," << m.nC << ")";
      throw ss.str();
    }

    std::stringstream ss;
    ss << name << " % " << m.name;
    Matrix result(nR, nC, USE_MATRIX_NAMES ? ss.str() : "");

    for (int i = 0; i < nR; ++i) {
      for (int j = 0; j < nC; ++j) {
        result.values[i][j] = this->values[i][j] * m.values[i][j];
      }
    }

    return result;
  }

};

std::ostream& operator<<(std::ostream &out, const Matrix &m) {
  out << m.name << " of shape: (" << m.nR << "," << m.nC << ") is\n";
  for (int i = 0; i < m.nR; ++i) {
    for (int j = 0; j < m.nC; ++j) {
      out << m.values[i][j] << " ";
    }
    out << std::endl;
  }
  return out;
}

#endif
