#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <utility>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

#include "cpu_serial/matrix.hpp"
#include "cpu_serial/nn.hpp"

using namespace std;
using namespace thrust;

const string TRAIN_FILE_PATH = "data/mnist/train.csv";
const string TEST_FILE_PATH = "data/mnist/test.csv";

const int FEATURES_LEN = 28 * 28;
const int FEATURE_MAX_VALUE = 255;
const int NUM_CLASSES = 10;

const int NUM_EPOCHS = 2;
const int BATCH_SIZE = 16;
const float LEARNING_RATE = 0.01;

vector<string> split(const string& s, char delimiter) {
  vector<string> tokens;
  string token;
  istringstream tokenStream(s);
  while (getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

std::pair< host_vector<float>, host_vector<int>  > parseFile(const string& filepath) {
  host_vector<float> X;
  host_vector<int> y;

  string line;
  ifstream stream;
  stream.open(filepath);
  while(getline(stream, line)) {
    vector<string> tokens = split(line, ',');
    /* pixels */
    for(int i = 0; i < FEATURES_LEN; i++) {
      float pixel = (float) stoi(tokens[i]) / FEATURE_MAX_VALUE;
      X.push_back(pixel);
    }

    vector<int> label(NUM_CLASSES, 0);
    label[stoi(tokens[FEATURES_LEN])] = 1;

    for (int i = 0; i < NUM_CLASSES; ++i) {
      y.push_back(label[i]);
    }
  }
  stream.close();

  return std::pair< host_vector<float>, host_vector<int> >(X, y);
}

int main(int argc, char* argv[]) {
  srand(42);
  USE_MATRIX_NAMES = false;

  printf("\nConfig:\n\tTRAIN_FILE_PATH: %s\n\tTEST_FILE_PATH: %s\n\tFEATURES_LEN: %d\n\tFEATURE_MAX_VALUE: %d\n\tNUM_CLASSES: %d\n\tNUM_EPOCHS: %d\n\tBATCH_SIZE: %d\n\tLEARNING_RATE: %f\n", TRAIN_FILE_PATH.c_str(), TEST_FILE_PATH.c_str(), FEATURES_LEN, FEATURE_MAX_VALUE, NUM_CLASSES, NUM_EPOCHS, BATCH_SIZE, LEARNING_RATE);

  try {
    auto fnn = FullyConnectedNetwork();
    fnn.addLayer(FEATURES_LEN);
    fnn.addLayer(1024);
    fnn.addLayer(512);
    fnn.addLayer(256);
    fnn.addLayer(128);
    fnn.addLayer(64);
    fnn.addLayer(32);
    fnn.addLayer(NUM_CLASSES);
    fnn.compile();

    host_vector<float> train_X;
    host_vector<int> train_y;
    auto train_Xy = parseFile(TRAIN_FILE_PATH);
    train_X = train_Xy.first;
    train_y = train_Xy.second;
    printf("Shape of train_X : (%d, )\n", train_X.size());
    printf("Shape of train_y : (%d, )\n", train_y.size());

    host_vector<float> test_X;
    host_vector<int> test_y;
    auto test_Xy = parseFile(TEST_FILE_PATH);
    test_X = test_Xy.first;
    test_y = test_Xy.second;
    printf("Shape of test_X  : (%d, )\n", test_X.size());
    printf("Shape of test_y  : (%d, )\n", test_y.size());

    printf("Training start\n");
    const int NUM_TRAINING_SAMPLES = train_X.size() / FEATURES_LEN;
    const int NUM_BATCHES = NUM_TRAINING_SAMPLES / BATCH_SIZE + 1;
    /* Mini batch SGD */
    for(int epochNum = 0; epochNum < NUM_EPOCHS; epochNum++) {
      /* shuffling data indices */
      vector<int> seq(NUM_TRAINING_SAMPLES);
      for(int i = 0; i < NUM_TRAINING_SAMPLES; i++) { seq[i] = i; }
      random_shuffle(seq.begin(), seq.end());

      for(int batchNum = 0; batchNum < NUM_BATCHES; batchNum++) {
        Matrix train_X_miniBatch(FEATURES_LEN, BATCH_SIZE, "train_X minibatch");
        for(int batch_i = 0 ; batch_i < BATCH_SIZE; batch_i++ ) {
          int randomBatch_i = seq[(batchNum * BATCH_SIZE + batch_i) % NUM_TRAINING_SAMPLES];
          for(int feature_i = 0 ; feature_i < FEATURES_LEN; feature_i++ ) {
            train_X_miniBatch.set(feature_i, batch_i, train_X[randomBatch_i * FEATURES_LEN + feature_i]);
          }
        }

        Matrix train_y_miniBatch(NUM_CLASSES, BATCH_SIZE, "train_y minibatch");
        for(int batch_i = 0 ; batch_i < BATCH_SIZE; batch_i++ ){
          int randomBatch_i = seq[(batchNum * BATCH_SIZE + batch_i) % NUM_TRAINING_SAMPLES];
          for(int feature_i = 0 ; feature_i < NUM_CLASSES; feature_i++ ){
            train_y_miniBatch.set(feature_i, batch_i, train_y[randomBatch_i * NUM_CLASSES + feature_i]);
          }
        }

        fnn.fit(train_X_miniBatch, train_y_miniBatch, LEARNING_RATE);

        cout << "Epoch : (" << epochNum + 1 << "/" << NUM_EPOCHS << ") Batch: [" << batchNum + 1 << "/" << NUM_BATCHES << "]\r";
        cout.flush();
      }
    }
    cout << "\r\n";

    /* setting up confusion matrix */
    Matrix confusionMatrix(NUM_CLASSES, NUM_CLASSES, "confusion matrix");
    confusionMatrix.setZeros();

    printf("Testing start\n");
    const int NUM_TESTING_SAMPLES = test_X.size() / FEATURES_LEN;
    for (int testSample_i = 0; testSample_i < NUM_TESTING_SAMPLES; ++testSample_i) {
      Matrix testSample(FEATURES_LEN, 1, "testSample");
      for(int j = 0; j < FEATURES_LEN; j++){
        testSample.set(j, 0, test_X[testSample_i * FEATURES_LEN + j]);
      }

      int actual = -1;
      for(int j = 0; j < NUM_CLASSES; j++){
        if (test_y[testSample_i * NUM_CLASSES + j] == 1) { actual = j; }
      }

      int prediction = fnn.predictClass(testSample);

      /* printf("Prediction %s\tactual: %d\tpredicted: %d", actual == prediction ? "Correct" : "Wrong", actual, prediction); */
      confusionMatrix.set(prediction, actual, confusionMatrix.get(prediction, actual) + 1);

      cout << "Testing : (" << testSample_i + 1 << "/" << NUM_TESTING_SAMPLES << ")\r";
      cout.flush();
    }
    cout << "\r\n";

    /* prediction analysis */
    cout << confusionMatrix;
    int numCorrect = 0;
    int total = NUM_TESTING_SAMPLES;
    for (int i = 0; i < NUM_CLASSES; ++i) {
      numCorrect += confusionMatrix.get(i, i);
    }
    printf("Accuracy: %f\n", (float) numCorrect / total );

  } catch (string e) {
    printf("%s", e.c_str());
  }

  return 0;
}
