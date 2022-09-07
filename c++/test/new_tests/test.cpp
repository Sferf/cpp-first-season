#include "DequeTests.hpp"

#include <iostream>

int main() {
    bool res = DequeTests::RunAll();
    if (res) {
        std::cout << "All tests passed\n";
    } else {
        std::cout << "Tests failed\n";
    }

    return !res;
}
