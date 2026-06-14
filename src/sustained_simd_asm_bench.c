#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__linux__) && defined(__aarch64__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif

#if defined(__x86_64__)
#define SIMD_KERNEL_NAME "x86_64_avx2"
#elif defined(__aarch64__)
#define SIMD_KERNEL_NAME "aarch64_sve"
#else
#define SIMD_KERNEL_NAME "unsupported"
#endif

extern uint64_t sustained_simd_kernel(uint64_t iterations, uint64_t seed);
extern uint64_t sustained_simd_lanes(void);

typedef struct {
  int thread_index;
  double duration_seconds;
  uint64_t block_iterations;
  uint64_t blocks;
  uint64_t vector_iterations;
  uint64_t checksum;
} worker_args;

static double monotonic_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

static int simd_supported(void) {
#if defined(__x86_64__)
  __builtin_cpu_init();
  return __builtin_cpu_supports("avx2") != 0;
#elif defined(__linux__) && defined(__aarch64__) && defined(HWCAP_SVE)
  return (getauxval(AT_HWCAP) & HWCAP_SVE) != 0;
#elif defined(__aarch64__)
  return 1;
#else
  return 0;
#endif
}

static void print_unsupported(int threads, double duration_seconds) {
  printf("{\"kernel\":\"%s\",\"supported\":false,\"threads\":%d,\"target_seconds\":%.3f,\"elapsed_seconds\":0.0,\"lanes\":0,\"blocks\":0,\"vector_iterations\":0,\"lane_updates\":0,\"lane_updates_per_second\":0.0,\"checksum\":\"0000000000000000\"}\n",
         SIMD_KERNEL_NAME, threads, duration_seconds);
}

static void *run_worker(void *raw_args) {
  worker_args *args = (worker_args *)raw_args;
  const double deadline = monotonic_seconds() + args->duration_seconds;

  uint64_t seed = UINT64_C(0x123456789abcdef0) + (uint64_t)args->thread_index;
  uint64_t checksum = seed;

  while (monotonic_seconds() < deadline) {
    checksum ^= sustained_simd_kernel(args->block_iterations, seed ^ checksum);
    seed += UINT64_C(0x9e3779b97f4a7c15) + checksum;
    args->blocks++;
    args->vector_iterations += args->block_iterations;
  }

  args->checksum = checksum;
  return NULL;
}

int main(int argc, char **argv) {
  int threads = argc > 1 ? atoi(argv[1]) : 4;
  double duration_seconds = argc > 2 ? atof(argv[2]) : 60.0;
  uint64_t block_iterations = argc > 3 ? strtoull(argv[3], NULL, 10) : 1000000;

  if (threads < 1 || threads > 128) {
    fprintf(stderr, "threads must be between 1 and 128\n");
    return 2;
  }
  if (duration_seconds < 1.0 || duration_seconds > 3600.0) {
    fprintf(stderr, "duration_seconds must be between 1 and 3600\n");
    return 2;
  }
  if (block_iterations < 1) {
    fprintf(stderr, "block_iterations must be positive\n");
    return 2;
  }

  if (!simd_supported()) {
    print_unsupported(threads, duration_seconds);
    return 0;
  }

  uint64_t lanes = sustained_simd_lanes();
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
    args[thread].block_iterations = block_iterations;
    if (pthread_create(&thread_ids[thread], NULL, run_worker, &args[thread]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 2;
    }
  }

  uint64_t total_blocks = 0;
  uint64_t total_vector_iterations = 0;
  uint64_t checksum = 0;
  for (int thread = 0; thread < threads; thread++) {
    pthread_join(thread_ids[thread], NULL);
    total_blocks += args[thread].blocks;
    total_vector_iterations += args[thread].vector_iterations;
    checksum ^= args[thread].checksum;
  }
  double elapsed = monotonic_seconds() - start;
  double lane_updates = (double)total_vector_iterations * (double)lanes;

  printf("{\"kernel\":\"%s\",\"supported\":true,\"threads\":%d,\"target_seconds\":%.3f,\"elapsed_seconds\":%.6f,\"lanes\":%" PRIu64 ",\"blocks\":%" PRIu64 ",\"vector_iterations\":%" PRIu64 ",\"lane_updates\":%.0f,\"lane_updates_per_second\":%.3f,\"checksum\":\"%016" PRIx64 "\"}\n",
         SIMD_KERNEL_NAME, threads, duration_seconds, elapsed, lanes, total_blocks,
         total_vector_iterations, lane_updates, lane_updates / elapsed, checksum);

  free(thread_ids);
  free(args);
  return 0;
}
