
#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>

class SplitMix64Rng {

private:
    uint64_t _state {0};

public:
    SplitMix64Rng() = default;
    SplitMix64Rng(uint64_t seed) : _state(seed) {}

    uint64_t operator()() {
        uint64_t z = (_state += UINT64_C(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        return z ^ (z >> 31);
    }

    double randomInRange(double low, double high) {
        static constexpr double maxNum = (double) std::numeric_limits<uint64_t>::max();
        if (low == high) return low;
        assert(low < high);
        double result = high;
        while (result >= high) {
            uint64_t r = (*this)();
            result = low + (high - low) * (r / maxNum);
        }
        assert(result >= low);
        assert(result < high);
        return result;
    }

    uint64_t max() const {
        return std::numeric_limits<uint64_t>::max();
    }
};

/*
choose k elements i.i.d. from sequence of n elements A[0..n), in a single linear pass

P(A[0] picked)  = 1 - P(A[0] not picked)
                = 1 - P(elem. not picked k times from urn with n elements without returning)
                = 1 - ( (n-1)/n * (n-2)/(n-1) * ... * (n-k)/(n-k+1) )
                = 1 - (n-k)/n       // all other terms are cancelled

=> Pick first element A[0] with prob. 1 - (n-k)/n
=> Picked: Continue with A[1] for k'=k-1, n'=n-1
=> Not picked: Continue with A[1] for k'=k, n'=n-1
*/
template <typename T>
std::vector<T> random_choice_k_from_n(const T* array, size_t arraySize, int k, 
        std::function<float()> rngZeroToOne) {

    int numRemainingElems = arraySize;
    int numToSelect = k;

    std::vector<T> selectedElems;

    size_t pos = 0;
    while (numToSelect > 0) {
        assert(numRemainingElems >= numToSelect);
        // select array[pos] or not?
        auto probSelect = 1 - (numRemainingElems-numToSelect) / (double)numRemainingElems;
        // robustness towards floating-point shenanigans
        if (probSelect > 0 && (probSelect >= 1 || rngZeroToOne() < probSelect)) {
            // array[pos] selected
            numToSelect--;
            selectedElems.push_back(array[pos]);
        }
        numRemainingElems--;
        pos++;
    }

    return selectedElems;
}
