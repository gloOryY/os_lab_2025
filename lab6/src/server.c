#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include "utils.h"  // Подключаем заголовочный файл библиотек

// Структура для передачи аргументов в поток
struct FactorialArgs {
    uint64_t begin;  // Начало диапазона для вычисления
    uint64_t end;    // Конец диапазона для вычисления
    uint64_t mod;    // Модуль для вычисления
};

// Функция для безопасного умножения по модулю
// Использует алгоритм двоичного умножения, чтобы избежать переполнения
//uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    //uint64_t result = 0;
    //a = a % mod;  // Приводим первый операнд по модулю
    
    //while (b > 0) {
        // Если текущий бит b равен 1, добавляем a к результату
        //if (b % 2 == 1)
            //result = (result + a) % mod;
        
        // Удваиваем a и делим b пополам (сдвиг вправо)
        //a = (a * 2) % mod;
        //b /= 2;
    //}
    
    //return result % mod;
//}

// Функция вычисления частичного факториала в заданном диапазоне
// Вычисляет произведение всех чисел от begin до end по модулю mod
uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;
    
    // Последовательно умножаем все числа в диапазоне [begin, end]
    for (uint64_t i = args->begin; i <= args->end; i++) {
        // Используем безопасное умножение по модулю
        ans = MultModulo(ans, i, args->mod);
    }
    
    return ans;
}

// Обёртка для запуска Factorial в отдельном потоке
// pthread требует функцию с сигнатурой void* (*)(void*)
void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    // Возвращаем результат как указатель (небезопасное приведение типов)
    return (void *)(uint64_t)Factorial(fargs);
}

int main(int argc, char **argv) {
    int tnum = -1;   // Количество потоков для параллельных вычислений
    int port = -1;   // Порт, на котором сервер будет слушать подключения
    
    // Парсинг аргументов командной строки с помощью getopt_long
    while (true) {
        int current_optind = optind ? optind : 1;
        
        // Определяем длинные опции (--port, --tnum)
        static struct option options[] = {
            {"port", required_argument, 0, 0},  // --port <номер_порта>
            {"tnum", required_argument, 0, 0},  // --tnum <количество_потоков>
            {0, 0, 0, 0}  // Завершающий элемент массива
        };
        
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        
        if (c == -1)  // Больше нет аргументов
            break;
        
        switch (c) {
            case 0: {
                switch (option_index) {
                    case 0:  // --port
                        port = atoi(optarg);
                        // Проверяем валидность порта (должен быть > 1024 для обычных пользователей)
                        if (port <= 1024 || port > 65535) {
                            fprintf(stderr, "Port must be between 1024 and 65535\n");
                            return 1;
                        }
                        break;
                    case 1:  // --tnum
                        tnum = atoi(optarg);
                        // Проверяем, что количество потоков положительное
                        if (tnum <= 0) {
                            fprintf(stderr, "Thread number must be positive\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
            } break;
            
            case '?':
                printf("Unknown argument\n");
                break;
            default:
                fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }
    
    // Проверяем, что все обязательные параметры переданы
    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
        return 1;
    }
    
    // Создаём TCP-сокет для сервера
    // AF_INET - IPv4, SOCK_STREAM - TCP, 0 - протокол по умолчанию
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Can not create server socket!\n");
        return 1;
    }
    
    // Настраиваем адрес сервера
    struct sockaddr_in server;
    server.sin_family = AF_INET;                    // IPv4
    server.sin_port = htons((uint16_t)port);        // Порт (преобразуем в сетевой порядок байтов)
    server.sin_addr.s_addr = htonl(INADDR_ANY);     // Слушаем на всех интерфейсах
    
    // Устанавливаем опцию SO_REUSEADDR для повторного использования адреса
    // Позволяет перезапустить сервер сразу после остановки
    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
    
    // Привязываем сокет к адресу и порту
    int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    if (err < 0) {
        fprintf(stderr, "Can not bind to socket!\n");
        return 1;
    }
    
    // Переводим сокет в режим прослушивания
    // 128 - размер очереди ожидающих подключений
    err = listen(server_fd, 128);
    if (err < 0) {
        fprintf(stderr, "Could not listen on socket\n");
        return 1;
    }
    
    printf("Server listening at %d\n", port);
    
    // Бесконечный цикл обработки подключений клиентов
    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        
        // Принимаем входящее подключение (блокирующий вызов)
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
        if (client_fd < 0) {
            fprintf(stderr, "Could not establish new connection\n");
            continue;  // Продолжаем ждать следующее подключение
        }
        
        // Обрабатываем запросы от этого клиента
        while (true) {
            unsigned int buffer_size = sizeof(uint64_t) * 3;  // begin, end, mod
            char from_client[buffer_size];
            
            // Получаем данные от клиента
            int read = recv(client_fd, from_client, buffer_size, 0);
            
            if (!read)  // Клиент закрыл соединение
                break;
            
            if (read < 0) {
                fprintf(stderr, "Client read failed\n");
                break;
            }
            
            if (read < buffer_size) {
                fprintf(stderr, "Client send wrong data format\n");
                break;
            }
            
            // Извлекаем данные из буфера
            uint64_t begin = 0;
            uint64_t end = 0;
            uint64_t mod = 0;
            memcpy(&begin, from_client, sizeof(uint64_t));
            memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
            memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));
            
            fprintf(stdout, "Receive: %llu %llu %llu\n", begin, end, mod);
            
            // Создаём массивы для потоков и их аргументов
            pthread_t threads[tnum];
            struct FactorialArgs args[tnum];
            
            // Вычисляем размер подзадачи для каждого потока
            uint64_t range = end - begin + 1;           // Общее количество чисел
            uint64_t chunk = range / tnum;               // Размер куска на поток
            uint64_t remainder = range % tnum;           // Остаток для распределения
            
            // Распределяем работу между потоками
            uint64_t current_begin = begin;
            for (uint32_t i = 0; i < tnum; i++) {
                args[i].begin = current_begin;
                
                // Первым потокам даём на 1 элемент больше, если есть остаток
                uint64_t current_chunk = chunk + (i < remainder ? 1 : 0);
                args[i].end = current_begin + current_chunk - 1;
                args[i].mod = mod;
                
                // Следующий поток начнёт с конца текущего + 1
                current_begin = args[i].end + 1;
                
                // Создаём поток для вычисления частичного факториала
                if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
                    printf("Error: pthread_create failed!\n");
                    return 1;
                }
            }
            
            // Собираем результаты от всех потоков
            uint64_t total = 1;
            for (uint32_t i = 0; i < tnum; i++) {
                uint64_t result = 0;
                
                // Ждём завершения потока и получаем его результат
                pthread_join(threads[i], (void **)&result);
                
                // Умножаем текущий общий результат на результат потока
                total = MultModulo(total, result, mod);
            }
            
            printf("Total: %llu\n", total);
            
            // Отправляем результат клиенту
            char buffer[sizeof(total)];
            memcpy(buffer, &total, sizeof(total));
            
            err = send(client_fd, buffer, sizeof(total), 0);
            if (err < 0) {
                fprintf(stderr, "Can't send data to client\n");
                break;
            }
        }
        
        // Закрываем соединение с клиентом
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }
    
    return 0;
}
