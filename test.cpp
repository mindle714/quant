extern "C" {
#include <mkl.h>
}

#include <vector>
#include <iostream>
#include <chrono>

int main() {
  std::vector<float> a = {1.2, 2.4, 3.6, 
                          4.8, 2.3, 3.4,
                          4.5, 5.6, 6.7,
                          7.8, 8.9, 9.1};
  std::vector<float> b = {2.1, 4.2, 6.3, 
                          8.4, 1.3, 2.5,
                          3.7, 4.9, 1.1};
  std::vector<float> c(12, 0.0);

  auto begin = std::chrono::steady_clock::now();
  cblas_sgemm(CblasRowMajor,
              CblasNoTrans,
              CblasNoTrans,
              4, 3, 3,
              1.f,
              a.data(), 3,
              b.data(), 3,
              0.f,
              c.data(), 3);
  auto end = std::chrono::steady_clock::now();
  std::cout << "Elapsed = " << std::chrono::duration_cast<
    std::chrono::milliseconds>(end - begin).count() << "(ms)" << std::endl;

  for(const auto& ce : c)
    std::cout << ce << std::endl;

  return 0;
}
