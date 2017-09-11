#pragma once

#include "../container/Array.h"

namespace L {
  namespace AI {
    class Perceptron {
      private:
        int inputSize;
        float rate;
        Array<float> weight;

      public:
        Perceptron(int inputSize, float rate);
        bool learn(const Array<bool>& input, bool result);
        bool answer(const Array<bool>& input);
    };
  }
}
