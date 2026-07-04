// Escape-time Julia set generator.
//
// For each pixel (x, y) in the square domain [-2, 2] x [-2, 2] we take the
// pixel as the starting value z0 of the orbit z_{n+1} = z_n^2 + c, where c is
// the fixed constant given on the command line. The stored value is the number
// of iterations until the orbit leaves the disk |z| <= 2 (escape radius 2),
// capped at MAX_ITER.
//
// NOTE: because z0 varies per pixel and c is fixed, this draws the *Julia set*
// for the constant c -- not the Mandelbrot set (that fixes z0 = 0 and varies c).
//
// Parallelism (all optional, mix them freely):
//   * MPI    -- the image is split into horizontal row-blocks, one per rank.
//   * OpenMP -- within a rank, threads share the pixels of its block  (mode 2).
//   * TBB    -- alternatively a rank uses the parallel STL             (mode 1).
// So "mpirun -n N ./Escape_Time ... 2" gives hybrid MPI + OpenMP.

#include <complex>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef USE_MPI
#include <mpi.h>
#endif

#ifdef USE_TBB
#include <algorithm>
#include <execution>
#endif

// The output is raw uint32 in host byte order; the Python readers use numpy
// '<u4' (little-endian). Fail loudly rather than silently emit swapped data.
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "Escape_Time output format assumes a little-endian host."
#endif

using cmplx = std::complex<double>;

constexpr double ESCAPE_RADIUS_SQ = 4.0;  // |z| > 2  =>  |z|^2 > 4  =>  diverges
constexpr unsigned MAX_ITER = 1000;
constexpr unsigned MAX_SIZE = 20000;      // guards memory / integer overflow

// Iterations of z_{n+1} = z^2 + c, starting at z, until |z| > 2 (or MAX_ITER).
uint32_t CalculateIterations(cmplx z, cmplx c) {
  double x = z.real();
  double y = z.imag();
  uint32_t iter = 0;
  while (x * x + y * y < ESCAPE_RADIUS_SQ && iter < MAX_ITER) {
    const double x_next = x * x - y * y + c.real();
    y = 2.0 * x * y + c.imag();
    x = x_next;
    ++iter;
  }
  return iter;
}

// Fill iteration counts for rows [row0, row0 + nrows) of a `size` x `size` grid.
std::vector<uint32_t> ComputeBlock(int row0, int nrows, unsigned size, cmplx c,
                                   int mode) {
  const double dp = 4.0 / size;  // pixel spacing across [-2, 2]
  const std::size_t count = static_cast<std::size_t>(nrows) * size;
  std::vector<uint32_t> block(count);

  auto value_at = [&](std::size_t k) -> uint32_t {
    const int row = row0 + static_cast<int>(k / size);
    const int col = static_cast<int>(k % size);
    const double x = -2.0 + (col + 0.5) * dp;  // sample pixel centres
    const double y = -2.0 + (row + 0.5) * dp;
    return CalculateIterations({x, y}, c);
  };

  if (mode == 2) {  // OpenMP (a no-op pragma if built without -fopenmp)
#pragma omp parallel for schedule(dynamic, 64)
    for (std::ptrdiff_t k = 0; k < static_cast<std::ptrdiff_t>(count); ++k) {
      const std::size_t idx = static_cast<std::size_t>(k);
      block[idx] = value_at(idx);
    }
  }
#ifdef USE_TBB
  else if (mode == 1) {  // parallel STL, TBB backend
    std::for_each(std::execution::par_unseq, block.begin(), block.end(),
                  [&](uint32_t &px) {
                    px = value_at(static_cast<std::size_t>(&px - block.data()));
                  });
  }
#endif
  else {  // mode 0 (and the fallback when a backend is unavailable)
    for (std::size_t k = 0; k < count; ++k) block[k] = value_at(k);
  }
  return block;
}

int main(int argc, char *argv[]) {
  int rank = 0, nprocs = 1;
#ifdef USE_MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
#endif

  auto fail = [&](const std::string &msg) {
    if (rank == 0) std::cerr << "Error: " << msg << "\n";
#ifdef USE_MPI
    MPI_Finalize();
#endif
    return 1;
  };

  // For a rank-0-only I/O failure (after the gather), abort the whole job so
  // every rank exits non-zero instead of ranks 1..N-1 reporting success.
  auto fail_io = [&](const std::string &msg) {
    std::cerr << "Error: " << msg << "\n";
#ifdef USE_MPI
    MPI_Abort(MPI_COMM_WORLD, 1);  // terminates all ranks; supersedes Finalize
#endif
    return 1;
  };

  if (argc != 6) {
    if (rank == 0) {
      std::cerr
          << "Usage: ./Escape_Time <cx> <cy> <size> <output_file> <mode>\n"
          << "  mode: 0 = sequential, 1 = TBB parallel, 2 = OpenMP\n"
          << "  Ex:   ./Escape_Time 0.2932 -0.6843 4096 fractal.dat 2\n"
          << "  Hybrid MPI+OpenMP: mpirun -n 4 ./Escape_Time 0.2932 -0.6843 "
             "4096 fractal.dat 2\n";
    }
#ifdef USE_MPI
    MPI_Finalize();
#endif
    return 1;
  }

  double cx, cy;
  long size_arg;
  int mode;
  try {
    cx = std::stod(argv[1]);
    cy = std::stod(argv[2]);
    size_arg = std::stol(argv[3]);
    mode = std::stoi(argv[5]);
  } catch (const std::exception &e) {
    return fail(std::string("could not parse numeric arguments (") + e.what() +
                ")");
  }

  const std::string output_file = argv[4];

  if (size_arg <= 0 || size_arg > static_cast<long>(MAX_SIZE))
    return fail("size must be between 1 and " + std::to_string(MAX_SIZE));
  if (mode < 0 || mode > 2)
    return fail("mode must be 0 (seq), 1 (TBB) or 2 (OpenMP)");
  const unsigned size = static_cast<unsigned>(size_arg);

#ifndef USE_TBB
  if (mode == 1 && rank == 0)
    std::cerr << "Note: built without TBB; mode 1 runs sequentially.\n";
#endif

  // Each rank owns a contiguous block of rows.
  const int row0 = static_cast<int>(static_cast<long long>(rank) * size / nprocs);
  const int row1 =
      static_cast<int>(static_cast<long long>(rank + 1) * size / nprocs);
  std::vector<uint32_t> block =
      ComputeBlock(row0, row1 - row0, size, {cx, cy}, mode);

  // Gather every rank's block onto rank 0, in row order.
  std::vector<uint32_t> image;
#ifdef USE_MPI
  std::vector<int> counts, displs;
  if (rank == 0) {
    counts.resize(nprocs);
    displs.resize(nprocs);
    int offset = 0;
    for (int r = 0; r < nprocs; ++r) {
      const int r0 = static_cast<int>(static_cast<long long>(r) * size / nprocs);
      const int r1 =
          static_cast<int>(static_cast<long long>(r + 1) * size / nprocs);
      counts[r] = (r1 - r0) * static_cast<int>(size);
      displs[r] = offset;
      offset += counts[r];
    }
    image.resize(static_cast<std::size_t>(size) * size);
  }
  MPI_Gatherv(block.data(), static_cast<int>(block.size()), MPI_UINT32_T,
              image.data(), counts.data(), displs.data(), MPI_UINT32_T, 0,
              MPI_COMM_WORLD);
#else
  image = std::move(block);
#endif

  // Rank 0 writes the grid as raw little-endian uint32 (row-major).
  if (rank == 0) {
    std::ofstream ofile(output_file, std::ios::binary);
    if (!ofile) return fail_io("cannot open output file '" + output_file + "'");
    ofile.write(reinterpret_cast<const char *>(image.data()),
                static_cast<std::streamsize>(image.size() * sizeof(uint32_t)));
    if (!ofile) return fail_io("failed while writing '" + output_file + "'");
    std::cout << "Wrote " << size << "x" << size << " grid to " << output_file
              << " (" << nprocs << " rank(s), mode " << mode << ")\n";
  }

#ifdef USE_MPI
  MPI_Finalize();
#endif
  return 0;
}
