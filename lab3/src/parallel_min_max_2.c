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
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для обработки сигналов
volatile sig_atomic_t timeout_occurred = 0;  // Флаг срабатывания таймаута
pid_t *child_pids = NULL;                    // Массив PID дочерних процессов
int pnum_global = 0;                         // Глобальная копия количества процессов

/**
 * Обработчик сигнала SIGALRM
 * Вызывается при срабатывании таймера
 */
void timeout_handler(int sig) {
    timeout_occurred = 1;  // Устанавливаем флаг таймаута
    printf("Timeout occurred! Sending SIGKILL to all child processes.\n");
    
    // Отправляем SIGKILL всем активным дочерним процессам
    for (int i = 0; i < pnum_global; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1;  // Таймаут в секундах (-1 означает отсутствие таймаута)
  bool with_files = false;

  // Блок обработки аргументов командной строки
  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "ft:", options, &option_index);

    if (c == -1) break;  // Все опции обработаны

    switch (c) {
      case 0:
        // Обработка длинных опций без короткого аналога
        switch (option_index) {
          case 0:  // --seed
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:  // --array_size
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:  // --pnum
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:  // --by_files
            with_files = true;
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':  // -f или --by_files
        with_files = true;
        break;
      case 't':  // -t или --timeout
        timeout = atoi(optarg);
        if (timeout <= 0) {
            printf("timeout must be a positive number\n");
            return 1;
        }
        break;
      case '?':  // Неизвестная опция
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  // Проверка на наличие лишних аргументов
  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  // Проверка обязательных параметров
  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files|-f] [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  // Блок инициализации глобальных переменных для обработки сигналов
  pnum_global = pnum;
  child_pids = malloc(pnum * sizeof(pid_t));
  if (child_pids == NULL) {
    perror("malloc failed");
    return 1;
  }
  for (int i = 0; i < pnum; i++) {
      child_pids[i] = 0;  // Инициализируем нулями
  }

  // Блок настройки обработки сигналов
  if (timeout > 0) {
      // Упрощенная установка обработчика сигнала
      if (signal(SIGALRM, timeout_handler) == SIG_ERR) {
          perror("signal failed");
          free(child_pids);
          return 1;
      }
      
      // Установка таймера
      alarm(timeout);
      printf("Timeout set to %d seconds\n", timeout);
  }

  // Создание и заполнение массива
  int *array = malloc(sizeof(int) * array_size);
  if (array == NULL) {
    perror("malloc failed");
    free(child_pids);
    return 1;
  }
  GenerateArray(array, array_size, seed);
  
  int active_child_processes = 0;

  // Блок создания пайпов (если не используются файлы)
  int **pipes = NULL;
  if (!with_files) {
    pipes = malloc(pnum * sizeof(int*));
    if (pipes == NULL) {
      perror("malloc failed");
      free(array);
      free(child_pids);
      return 1;
    }
    
    for (int i = 0; i < pnum; i++) {
      pipes[i] = malloc(2 * sizeof(int));
      if (pipes[i] == NULL) {
        perror("malloc failed");
        // Освобождаем уже выделенную память
        for (int j = 0; j < i; j++) {
          free(pipes[j]);
        }
        free(pipes);
        free(array);
        free(child_pids);
        return 1;
      }
      if (pipe(pipes[i]) == -1) {
        perror("pipe failed");
        for (int j = 0; j <= i; j++) {
          free(pipes[j]);
        }
        free(pipes);
        free(array);
        free(child_pids);
        return 1;
      }
    }
  }

  // Засекаем время начала выполнения
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Блок создания дочерних процессов
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    
    if (child_pid >= 0) {
      // Успешный fork
      active_child_processes += 1;
      child_pids[i] = child_pid;  // Сохраняем PID дочернего процесса
      
      if (child_pid == 0) {
        // Код выполняется в дочернем процессе
        
        // Вычисляем диапазон элементов для обработки
        unsigned int begin = i * array_size / pnum;
        unsigned int end = (i + 1) * array_size / pnum;
        
        // Для последнего процесса убедимся, что end = array_size
        if (i == pnum - 1) {
          end = array_size;
        }

        // Находим локальные min и max в своем диапазоне
        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          // Используем файлы для передачи результатов
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
          } else {
            perror("fopen failed");
          }
        } else {
          // Используем пайпы для передачи результатов
          close(pipes[i][0]);  // Закрываем чтение в дочернем процессе
          write(pipes[i][1], &local_min_max.min, sizeof(int));
          write(pipes[i][1], &local_min_max.max, sizeof(int));
          close(pipes[i][1]);  // Закрываем запись после отправки
        }

        // Освобождаем ресурсы в дочернем процессе
        free(array);
        if (!with_files) {
          for (int j = 0; j < pnum; j++) {
            free(pipes[j]);
          }
          free(pipes);
        }
        free(child_pids);
        return 0;  // Завершаем дочерний процесс
      }
      // Родительский процесс продолжает здесь

    } else {
      printf("Fork failed!\n");
      // Освобождаем ресурсы при ошибке
      free(array);
      free(child_pids);
      if (!with_files && pipes) {
        for (int j = 0; j < pnum; j++) {
          if (pipes[j]) free(pipes[j]);
        }
        free(pipes);
      }
      return 1;
    }
  }

  // Блок родительского процесса после создания всех дочерних процессов
  
  // Закрываем ненужные концы пайпов в родительском процессе
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);  // Закрываем запись в родительском процессе
    }
  }

  // Блок ожидания завершения дочерних процессов с таймаутом
  while (active_child_processes > 0) {
    int status;
    // Неблокирующее ожидание любого дочернего процесса
    pid_t finished_pid = waitpid(-1, &status, WNOHANG);
    
    if (finished_pid > 0) {
      // Найден завершенный процесс
      active_child_processes -= 1;
      
      // Находим и обнуляем PID завершенного процесса
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] == finished_pid) {
          child_pids[i] = 0;
          break;
        }
      }
      
      // Выводим информацию о завершении процесса
      if (WIFEXITED(status)) {
        printf("Child process %d exited normally\n", finished_pid);
      } else if (WIFSIGNALED(status)) {
        printf("Child process %d killed by signal %d\n", finished_pid, WTERMSIG(status));
      }
    } else if (finished_pid == 0) {
      // Нет завершенных процессов, проверяем таймаут
      if (timeout_occurred) {
        printf("Timeout handler has already killed child processes\n");
        break;
      }
      // Небольшая пауза перед следующей проверкой
      usleep(10000);  // 10ms
    } else {
      // Ошибка waitpid
      perror("waitpid failed");
      break;
    }
  }

  // Отменяем таймер, если он еще не сработал
  if (timeout > 0) {
    alarm(0);
  }

  // Блок сбора и обработки результатов
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  int results_received = 0;  // Счетчик полученных результатов
  
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    int result_available = 0;  // Флаг доступности результата

    if (with_files) {
      // Чтение результатов из файлов
      char filename_min[256], filename_max[256];
      sprintf(filename_min, "min_%d.txt", i);
      sprintf(filename_max, "max_%d.txt", i);
      
      FILE *fp_min = fopen(filename_min, "r");
      FILE *fp_max = fopen(filename_max, "r");
      if (fp_min && fp_max) {
        if (fscanf(fp_min, "%d", &min) == 1 && fscanf(fp_max, "%d", &max) == 1) {
          result_available = 1;
          results_received++;
        }
        fclose(fp_min);
        fclose(fp_max);
        
        // Удаляем временные файлы
        remove(filename_min);
        remove(filename_max);
      }
    } else {
      // Чтение результатов из пайпов
      // Используем неблокирующее чтение
      ssize_t bytes_read_min = read(pipes[i][0], &min, sizeof(int));
      ssize_t bytes_read_max = read(pipes[i][0], &max, sizeof(int));
      
      if (bytes_read_min == sizeof(int) && bytes_read_max == sizeof(int)) {
        result_available = 1;
        results_received++;
      }
      close(pipes[i][0]);  // Закрываем пайп после чтения
    }

    // Обновляем глобальные min и max
    if (result_available) {
      if (min < min_max.min) min_max.min = min;
      if (max > min_max.max) min_max.max = max;
    }
  }

  // Блок завершения программы
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  // Вычисление времени выполнения
  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  // Освобождение ресурсов
  free(array);
  free(child_pids);
  if (!with_files && pipes) {
    for (int i = 0; i < pnum; i++) {
      free(pipes[i]);
    }
    free(pipes);
  }

  // Вывод результатов
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Results received from %d out of %d processes\n", results_received, pnum);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  if (timeout_occurred) {
    printf("Program terminated due to timeout\n");
  }
  
  fflush(NULL);
  return 0;
}