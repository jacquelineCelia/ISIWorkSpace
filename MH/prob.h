#include <cassert>
#include <vector>
#include <random>
#include <cmath>
#include <mutex>
#include <iostream>


/* Utility functions for generating random numbers */

namespace prob {

template<typename Engine> 
double random(Engine& engine) {
    return std::uniform_real_distribution<>(0, 1)(engine);
}

template<typename Engine> 
double randint(Engine& engine, int a, int b) {
    return std::uniform_int_distribution<>(a, b)(engine);
}

}
