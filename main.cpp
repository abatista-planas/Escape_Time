#include <algorithm>
#include <complex>
#include <execution>
#include <fstream>
#include <iostream>

constexpr double TH_NORM = 1.0;
constexpr unsigned MAX_ITER = 1000;

using namespace std;
using cmplx = complex<double>;

unsigned CalculateIterations(const cmplx z, const cmplx &c) {
  unsigned iter = 0;

  double x = z.real();
  double y = z.imag();

  for (;;) {
    const double tmp1 = x;
    const double tmp2 = y;
    x *= x;
    y *= y;
    if (x + y >= TH_NORM || iter >= MAX_ITER) break;
    x += -y + c.real();
    y = 2 * tmp1 * tmp2 + c.imag();
    ++iter;
  }
  return iter;
}

vector<unsigned> CreateFractalPal(const double cx, const double cy,
                                  const unsigned size) {
  // The domain consists of the region [-2,2] \times [-2,2]
  const double dp = 4.0 / size;

  vector<unsigned> data(size * size);
  for_each(execution::par_unseq, data.begin(), data.end(),
           [&](unsigned &pixel) {
             const ptrdiff_t position = &pixel - &(data[0]);
             const int col_index = position % size;
             const int row_index = position / size;

             double x = -2.0 + col_index * dp;
             double y = -2.0 + row_index * dp;

             pixel = CalculateIterations({x, y}, {cx, cy});
           });
  return data;
}

vector<unsigned> CreateFractalSeq(const double cx, const double cy,
                                  const unsigned size) {
  // The domain consists of the region [-2,2] \times [-2,2]
  const double dp = 4.0 / size;

  vector<unsigned> data(size * size);
  for (auto &&pixel : data) {
    const ptrdiff_t position = &pixel - &(data[0]);
    const int col_index = position % size;
    const int row_index = position / size;

    double x = -2.0 + col_index * dp;
    double y = -2.0 + row_index * dp;

    pixel = CalculateIterations({x, y}, {cx, cy});
  }
  return data;
}

int main(int argc, char *argv[]) {
  if (argc != 6) {
    cerr << "Wrong input parameters" << endl;
    cerr << "Usage: ./Escape_Time <cx> <cy> <size> <output_file> <0-seq or 1-pal>"
         << endl;
    cerr << "   Ex: ./Escape_Time 0.2932 -0.6843 4096 fractal.dat 1" << endl;
    return -1;
  }

  const double cx = stod(argv[1]);
  const double cy = stod(argv[2]);
  const unsigned size = stoul(argv[3]);
  const string output_file = argv[4];
  const bool use_pal = stoi(argv[5]);
  auto data =
      use_pal ? CreateFractalPal(cx, cy, size) : CreateFractalSeq(cx, cy, size);

  ofstream ofile(output_file, ios::binary);
  ofile.write(reinterpret_cast<char *>(data.data()), data.size() * sizeof(unsigned));
  ofile.close();
  return 0;
}
