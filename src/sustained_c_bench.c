#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  int thread_index;
  double duration_seconds;
  uint64_t blocks;
  uint64_t iterations;
  uint64_t checksum;
  double fp_checksum;
} worker_args;

static double monotonic_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

static uint64_t mix64(uint64_t value) {
  value ^= value >> 30;
  value *= UINT64_C(0xbf58476d1ce4e5b9);
  value ^= value >> 27;
  value *= UINT64_C(0x94d049bb133111eb);
  value ^= value >> 31;
  return value;
}

static void *run_worker(void *raw_args) {
  worker_args *args = (worker_args *)raw_args;
  const uint64_t block_iterations = 4096;
  const double deadline = monotonic_seconds() + args->duration_seconds;

  uint64_t a = UINT64_C(0x123456789abcdef0) + (uint64_t)args->thread_index;
  uint64_t b = UINT64_C(0xfedcba9876543210) ^ ((uint64_t)args->thread_index << 32);
  uint64_t c = UINT64_C(0x9e3779b97f4a7c15);
  uint64_t d = UINT64_C(0xd6e8feb86659fd93);

  double x = 0.125 + (double)args->thread_index;
  double y = 0.25 + (double)args->thread_index * 0.5;
  double z = 0.5 + (double)args->thread_index * 0.25;
  double energy = 1.0;

  while (monotonic_seconds() < deadline) {
    for (uint64_t index = 0; index < block_iterations; index++) {
      a += c;
      b ^= mix64(a + d);
      c += (b | UINT64_C(1));
      d = (d << 13) | (d >> 51);
      d ^= a + b;

      double force = (double)((a >> 11) & UINT64_C(0xfffff)) * 0x1.0p-20;
      double drag = (double)((b >> 17) & UINT64_C(0xfffff)) * 0x1.0p-22;
      x = x + (y * 0.000013) - (z * 0.000007) + force;
      y = y + (z * 0.000011) - (x * 0.000005) + drag;
      z = z + (x * 0.000009) - (y * 0.000003) + 0.000001;
      energy += (x * x + y * y + z * z) * 0.0000000001;

      if (x > 1024.0 || y > 1024.0 || z > 1024.0) {
        x *= 0.0009765625;
        y *= 0.0009765625;
        z *= 0.0009765625;
      }
    }

    args->blocks++;
    args->iterations += block_iterations;
  }

  args->checksum = a ^ b ^ c ^ d ^ mix64(args->iterations);
  args->fp_checksum = energy + x + y + z;
  return NULL;
}

int main(int argc, char **argv) {
  int threads = argc > 1 ? atoi(argv[1]) : 4;
  double duration_seconds = argc > 2 ? atof(argv[2]) : 60.0;

  if (threads < 1 || threads > 128) {
    fprintf(stderr, "threads must be between 1 and 128\n");
    return 2;
  }
  if (duration_seconds < 1.0 || duration_seconds > 3600.0) {
    fprintf(stderr, "duration_seconds must be between 1 and 3600\n");
    return 2;
  }

  pthread_t *thread_ids = calloc((size_t)threads, sizeof(pthread_t));
  worker_args *args = calloc((size_t)threads, sizeof(worker_args));
  if (thread_ids == NULL || args == NULL) {
    fprintf(stderr, "allocation failed\n");
    return 2;
  }

  double start = monotonic_seconds();
  for (int thread = 0; thread < threads; thread++) {
    args[thread].thread_index = thread;
    args[thread].duration_seconds = duration_seconds;
    if (pthread_create(&thread_ids[thread], NULL, run_worker, &args[thread]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 2;
    }
  }

  uint64_t total_blocks = 0;
  uint64_t total_iterations = 0;
  uint64_t checksum = 0;
  double fp_checksum = 0.0;
  for (int thread = 0; thread < threads; thread++) {
    pthread_join(thread_ids[thread], NULL);
    total_blocks += args[thread].blocks;
    total_iterations += args[thread].iterations;
    checksum ^= args[thread].checksum;
    fp_checksum += args[thread].fp_checksum;
  }
  double elapsed = monotonic_seconds() - start;

  printf("{\"threads\":%d,\"target_seconds\":%.3f,\"elapsed_seconds\":%.6f,\"blocks\":%" PRIu64 ",\"iterations\":%" PRIu64 ",\"iterations_per_second\":%.3f,\"checksum\":\"%016" PRIx64 "\",\"fp_checksum\":%.17g}\n",
         threads, duration_seconds, elapsed, total_blocks, total_iterations,
         (double)total_iterations / elapsed, checksum, fp_checksum);

  free(thread_ids);
  free(args);
  return 0;
}
