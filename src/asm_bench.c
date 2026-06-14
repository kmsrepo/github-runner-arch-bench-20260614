#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__x86_64__)
#define ASM_KERNEL_NAME "x86_64_sse2"
#elif defined(__aarch64__)
#define ASM_KERNEL_NAME "aarch64_neon"
#else
#define ASM_KERNEL_NAME "unsupported"
#endif

extern uint64_t asm_integer_kernel(uint64_t iterations, uint64_t seed);

typedef struct {
  uint64_t iterations;
  uint64_t seed;
  uint64_t result;
} worker_args;

static void *run_worker(void *raw_args) {
  worker_args *args = (worker_args *)raw_args;
  args->result = asm_integer_kernel(args->iterations, args->seed);
  return NULL;
}

static double monotonic_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

int main(int argc, char **argv) {
  int threads = argc > 1 ? atoi(argv[1]) : 4;
  uint64_t iterations = argc > 2 ? strtoull(argv[2], NULL, 10) : 300000000;

  if (threads < 1 || threads > 128) {
    fprintf(stderr, "threads must be between 1 and 128\n");
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
    args[thread].iterations = iterations;
    args[thread].seed = UINT64_C(0x123456789abcdef0) + (uint64_t)thread;
    if (pthread_create(&thread_ids[thread], NULL, run_worker, &args[thread]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 2;
    }
  }

  uint64_t checksum = 0;
  for (int thread = 0; thread < threads; thread++) {
    pthread_join(thread_ids[thread], NULL);
    checksum ^= args[thread].result;
  }

  double elapsed = monotonic_seconds() - start;
  double lane_updates = (double)threads * (double)iterations * 2.0;

  printf("{\"kernel\":\"%s\",\"threads\":%d,\"iterations_per_thread\":%" PRIu64 ",\"lane_updates\":%.0f,\"seconds\":%.6f,\"lane_updates_per_second\":%.3f,\"checksum\":\"%016" PRIx64 "\"}\n",
         ASM_KERNEL_NAME, threads, iterations, lane_updates, elapsed,
         lane_updates / elapsed, checksum);

  free(thread_ids);
  free(args);
  return 0;
}
