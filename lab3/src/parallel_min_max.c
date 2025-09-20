#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files|-f]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Создаем пайпы для каждого процесса, если не используем файлы
  int **pipes = NULL;
  if (!with_files) {
    pipes = malloc(pnum * sizeof(int*));
    for (int i = 0; i < pnum; i++) {
      pipes[i] = malloc(2 * sizeof(int));
      if (pipe(pipes[i]) == -1) {
        perror("pipe failed");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // успешный форк
      active_child_processes += 1;
      if (child_pid == 0) {
        // дочерний процесс
        unsigned int begin = i * array_size / pnum;
        unsigned int end = (i + 1) * array_size / pnum;
        
        // Для последнего процесса убедимся, что end = array_size
        if (i == pnum - 1) {
          end = array_size;
        }

        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          // use files here
          char filename_min[256], filename_max[256];
          sprintf(filename_min, "min_%d.txt", i);
          sprintf(filename_max, "max_%d.txt", i);
          
          FILE *fp_min = fopen(filename_min, "w");
          FILE *fp_max = fopen(filename_max, "w");
          if (fp_min && fp_max) {
            fprintf(fp_min, "%d", local_min_max.min);
            fprintf(fp_max, "%d", local_min_max.max);
            fclose(fp_min);
            fclose(fp_max);
          }
        } else {
          // use pipe here
          close(pipes[i][0]); // закрываем чтение в дочернем процессе
          write(pipes[i][1], &local_min_max.min, sizeof(int));
          write(pipes[i][1], &local_min_max.max, sizeof(int));
          close(pipes[i][1]);
        }

        free(array);
        if (!with_files) {
          for (int j = 0; j < pnum; j++) {
            free(pipes[j]);
          }
          free(pipes);
        }
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // В родительском процессе закрываем ненужные концы пайпов
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]); // закрываем запись в родительском процессе
    }
  }

  while (active_child_processes > 0) {
    wait(NULL); // ждем завершения всех дочерних процессов
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // read from files
      char filename_min[256], filename_max[256];
      sprintf(filename_min, "min_%d.txt", i);
      sprintf(filename_max, "max_%d.txt", i);
      
      FILE *fp_min = fopen(filename_min, "r");
      FILE *fp_max = fopen(filename_max, "r");
      if (fp_min && fp_max) {
        fscanf(fp_min, "%d", &min);
        fscanf(fp_max, "%d", &max);
        fclose(fp_min);
        fclose(fp_max);
        
        // Удаляем временные файлы
        remove(filename_min);
        remove(filename_max);
      }
    } else {
      // read from pipes
      read(pipes[i][0], &min, sizeof(int));
      read(pipes[i][0], &max, sizeof(int));
      close(pipes[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      free(pipes[i]);
    }
    free(pipes);
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}