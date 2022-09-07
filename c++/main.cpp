#include "stackallocator.cpp"
#include <iostream>

int main() {
  List<int> l;
  l.push_back(4);
  List<int> o(3);
  o = l;
  for (auto i : o) {
    std::cout << i << ' ';
  }
  return 0;
}
