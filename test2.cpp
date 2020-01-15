extern "C" {
#include <mkl.h>
}

#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>

void loadQuant(const std::vector<float>& a,
               float* scale, int64_t* zero_point) {
  const float* a_d = a.data();

  float min = *std::min_element(a_d, a_d + a.size());
  float max = *std::max_element(a_d, a_d + a.size());
  min = std::min(min, 0.f);
  max = std::max(max, 0.f);

  const int q_min = std::numeric_limits<int8_t>::min(),
        q_max = std::numeric_limits<int8_t>::max();
  const float q_min_f = q_min, q_max_f = q_max;

  *scale = (max - min) / (q_max_f - q_min_f);
  float zero_point_from_min = q_min_f;
  if (*scale != 0) zero_point_from_min = q_min_f - min / *scale;

  if (zero_point_from_min < q_min_f) *zero_point = min;
  else if (zero_point_from_min > q_max_f) *zero_point = max;
  else *zero_point = std::round(zero_point_from_min);
}

void quantize(const std::vector<float>& a,
              std::vector<int8_t>& a_q,
              const float& scale, const int64_t& zero_point) {
  a_q.resize(a.size());
  const double inv_scale = 1. / scale;
  for (std::size_t i = 0; i < a.size(); ++i) {
    double scaled;
    if (scale == 0) scaled = zero_point;
    else scaled = zero_point + inv_scale * a[i];
    a_q[i] = std::round(scaled);
  }
}

int main() {
  auto begin = std::chrono::steady_clock::now();

  std::vector<float> a = {1.2, 2.4, 3.6, 
                          4.8, 2.3, 3.4,
                          -4.5, 5.6, 6.7,
                          7.8, 8.9, 9.1};
  float a_scale;
  int64_t a_zero_point;
  std::vector<int8_t> a_q;
  loadQuant(a, &a_scale, &a_zero_point);
  quantize(a, a_q, a_scale, a_zero_point);

/*  std::cout << "scale : " << a_scale <<
    " zero point : " << a_zero_point << std::endl;
  for (int i = 0; i < a.size(); ++i) {
    std::cout << a[i] << " -> " <<
      (a_q[i] - a_zero_point) * a_scale << std::endl;
  }*/

  std::vector<float> b = {-2.1, 4.2, 6.3, 
                          8.4, 1.3, 2.5,
                          3.7, -4.9, 1.1};
  float b_scale;
  int64_t b_zero_point;
  std::vector<int8_t> b_q;
  loadQuant(b, &b_scale, &b_zero_point);
  quantize(b, b_q, b_scale, b_zero_point);

/*  std::cout << "scale : " << b_scale <<
    " zero point : " << b_zero_point << std::endl;
  for (int i = 0; i < b.size(); ++i) {
    std::cout << b[i] << " -> " <<
      (b_q[i] - b_zero_point) * b_scale << std::endl;
  }*/

  int md = 4, nd = 3, kd = 3;

  // cs stands for row-wise sum
  std::vector<int> a_qrs(md, 0);
  for (int i = 0; i < md; ++i) {
    for (int j = 0; j < kd; ++j)
      a_qrs[i] += a_q[i * kd + j];
    a_qrs[i] *= -b_zero_point;
  }

  // rs stands for column-wise sum
  std::vector<int> b_qcs(nd, 0);
  for (int j = 0; j < nd; ++j) {
    for (int i = 0; i < kd; ++i)
      b_qcs[j] += b_q[i * nd + j];
    b_qcs[j] *= -a_zero_point;
  }

  int ab_qz = a_zero_point * b_zero_point * kd;

  std::vector<int> ab_q(12, 0);
  for (int i = 0; i < md; ++i) {
    for (int j = 0; j < nd; ++j) {
      int ab_idx = i * nd + j;
      for (int k = 0; k < kd; ++k)
        ab_q[ab_idx] += a_q[i * kd + k] *
          b_q[k * nd + j];

      ab_q[ab_idx] += a_qrs[i];
      ab_q[ab_idx] += b_qcs[j];
      ab_q[ab_idx] += ab_qz;
    }
  }

  float ab_scale = a_scale * b_scale;
  std::vector<float> ab(12, 0.0);
  for (int i = 0; i < md; ++i) {
    for (int j = 0; j < nd; ++j) {
      int ab_idx = i * nd + j;
      ab[ab_idx] = ab_q[ab_idx] * ab_scale;
    }
  }

  auto end = std::chrono::steady_clock::now();
  std::cout << "Elapsed = " << std::chrono::duration_cast<
    std::chrono::milliseconds>(end - begin).count() << "(ms)" << std::endl;

  begin = std::chrono::steady_clock::now();

//  for (const auto& abe : ab)
//    std::cout << abe << std::endl;

  std::vector<float> c(12, 0.0);
  cblas_sgemm(CblasRowMajor,
              CblasNoTrans,
              CblasNoTrans,
              4, 3, 3,
              1.f,
              a.data(), 3,
              b.data(), 3,
              0.f,
              c.data(), 3);

//  for (const auto& ce : c)
//    std::cout << ce << std::endl;

  end = std::chrono::steady_clock::now();
  std::cout << "Elapsed = " << std::chrono::duration_cast<
    std::chrono::milliseconds>(end - begin).count() << "(ms)" << std::endl;

  std::cout << "MATMUL RESULT:" << std::endl;
  for (int i = 0; i < c.size(); ++i) {
    std::cout << c[i] << " -> " << ab[i] << std::endl;
  }

  return 0;
}
