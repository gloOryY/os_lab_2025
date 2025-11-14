#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include "utils.h"  // Подключаем заголовочный файл библиотеки

// Структура для хранения информации о сервере
struct Server {
    char ip[255];    // IP-адрес сервера
    int port;        // Порт сервера
};

// Структура для передачи данных потоку, работающему с сервером
struct ServerTask {
    struct Server server;   // Информация о сервере
    uint64_t begin;         // Начало диапазона вычислений
    uint64_t end;           // Конец диапазона вычислений
    uint64_t mod;           // Модуль
    uint64_t result;        // Результат вычислений (заполняется потоком)
};

// Функция для безопасного умножения по модулю (аналогична серверной)
//uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    //uint64_t result = 0;
    //a = a % mod;
    
    //while (b > 0) {
        //if (b % 2 == 1)
            //result = (result + a) % mod;
        //a = (a * 2) % mod;
        //b /= 2;
    //}
    
    //return result % mod;
//}

// Конвертация строки в uint64_t с проверкой ошибок
bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    
    if (errno != 0)
        return false;
    
    *val = i;
    return true;
}

// Функция потока для работы с одним сервером
// Этот поток подключается к серверу, отправляет задачу и получает результат
void *ServerWorker(void *arg) {
    struct ServerTask *task = (struct ServerTask *)arg;
    
    // Получаем информацию о хосте по IP-адресу
    struct hostent *hostname = gethostbyname(task->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", task->server.ip);
        task->result = 1;  // При ошибке возвращаем нейтральный элемент
        return NULL;
    }
    
    // Настраиваем структуру адреса сервера
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(task->server.port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr_list[0]);
    
    // Создаём TCP-сокет для подключения к серверу
    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        task->result = 1;
        return NULL;
    }
    
    // Подключаемся к серверу
    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection failed to %s:%d\n", task->server.ip, task->server.port);
        close(sck);
        task->result = 1;
        return NULL;
    }
    
    // Формируем пакет с заданием: begin, end, mod
    char send_buffer[sizeof(uint64_t) * 3];
    memcpy(send_buffer, &task->begin, sizeof(uint64_t));
    memcpy(send_buffer + sizeof(uint64_t), &task->end, sizeof(uint64_t));
    memcpy(send_buffer + 2 * sizeof(uint64_t), &task->mod, sizeof(uint64_t));
    
    // Отправляем задание серверу
    if (send(sck, send_buffer, sizeof(send_buffer), 0) < 0) {
        fprintf(stderr, "Send failed\n");
        close(sck);
        task->result = 1;
        return NULL;
    }
    
    // Получаем результат от сервера
    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed\n");
        close(sck);
        task->result = 1;
        return NULL;
    }
    
    // Извлекаем результат из буфера
    memcpy(&task->result, response, sizeof(uint64_t));
    
    printf("Server %s:%d computed result: %llu\n", 
           task->server.ip, task->server.port, task->result);
    
    // Закрываем соединение
    close(sck);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = -1;              // Число k для вычисления k!
    uint64_t mod = -1;            // Модуль
    char servers[255] = {'\0'};   // Путь к файлу со списком серверов
                                   // 255 - максимальная длина пути в большинстве ФС
    
    // Парсинг аргументов командной строки
    while (true) {
        int current_optind = optind ? optind : 1;
        
        static struct option options[] = {
            {"k", required_argument, 0, 0},        // --k <число>
            {"mod", required_argument, 0, 0},      // --mod <модуль>
            {"servers", required_argument, 0, 0},  // --servers <путь_к_файлу>
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c) {
            case 0: {
                switch (option_index) {
                    case 0:  // --k
                        if (!ConvertStringToUI64(optarg, &k)) {
                            fprintf(stderr, "Invalid value for k\n");
                            return 1;
                        }
                        break;
                    case 1:  // --mod
                        if (!ConvertStringToUI64(optarg, &mod)) {
                            fprintf(stderr, "Invalid value for mod\n");
                            return 1;
                        }
                        break;
                    case 2:  // --servers
                        // Копируем путь к файлу серверов
                        memcpy(servers, optarg, strlen(optarg));
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
            } break;
            
            case '?':
                printf("Arguments error\n");
                break;
            default:
                fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }
    
    // Проверяем, что все аргументы переданы
    if (k == -1 || mod == -1 || !strlen(servers)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }
    
    // Читаем список серверов из файла
    FILE *servers_file = fopen(servers, "r");
    if (!servers_file) {
        fprintf(stderr, "Cannot open servers file: %s\n", servers);
        return 1;
    }
    
    // Подсчитываем количество серверов в файле
    unsigned int servers_num = 0;
    char line[256];
    while (fgets(line, sizeof(line), servers_file)) {
        if (strlen(line) > 1)  // Пропускаем пустые строки
            servers_num++;
    }
    
    if (servers_num == 0) {
        fprintf(stderr, "No servers found in file\n");
        fclose(servers_file);
        return 1;
    }
    
    // Выделяем память для массива серверов
    struct Server *to = malloc(sizeof(struct Server) * servers_num);
    
    // Читаем серверы из файла (формат: ip:port)
    rewind(servers_file);  // Возвращаемся в начало файла
    unsigned int idx = 0;
    while (fgets(line, sizeof(line), servers_file) && idx < servers_num) {
        // Убираем символ новой строки
        line[strcspn(line, "\n")] = 0;
        
        // Разделяем строку на IP и порт по символу ':'
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';  // Разделяем строку
            strncpy(to[idx].ip, line, sizeof(to[idx].ip) - 1);
            to[idx].port = atoi(colon + 1);
            idx++;
        }
    }
    fclose(servers_file);
    
    printf("Loaded %u servers\n", servers_num);
    
    // Распределяем работу между серверами
    uint64_t range = k;                       // Общий диапазон [1..k]
    uint64_t chunk = range / servers_num;     // Размер куска на сервер
    uint64_t remainder = range % servers_num; // Остаток
    
    // Создаём массивы для потоков и задач
    pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);
    struct ServerTask *tasks = malloc(sizeof(struct ServerTask) * servers_num);
    
    // Распределяем диапазоны между серверами и запускаем потоки
    uint64_t current_begin = 1;
    for (unsigned int i = 0; i < servers_num; i++) {
        tasks[i].server = to[i];
        tasks[i].begin = current_begin;
        
        // Первым серверам даём на 1 элемент больше, если есть остаток
        uint64_t current_chunk = chunk + (i < remainder ? 1 : 0);
        tasks[i].end = current_begin + current_chunk - 1;
        tasks[i].mod = mod;
        tasks[i].result = 1;  // Инициализируем нейтральным элементом
        
        current_begin = tasks[i].end + 1;
        
        printf("Assigning range [%llu, %llu] to server %s:%d\n",
               tasks[i].begin, tasks[i].end, to[i].ip, to[i].port);
        
        // Запускаем поток для работы с этим сервером
        // ПАРАЛЛЕЛЬНАЯ РАБОТА: каждый сервер обрабатывается в отдельном потоке
        if (pthread_create(&threads[i], NULL, ServerWorker, (void *)&tasks[i])) {
            fprintf(stderr, "Error: pthread_create failed for server %d\n", i);
            tasks[i].result = 1;
        }
    }
    
    // Ждём завершения всех потоков и собираем результаты
    uint64_t answer = 1;
    for (unsigned int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
        
        // Объединяем результат от этого сервера с общим результатом
        answer = MultModulo(answer, tasks[i].result, mod);
    }
    
    printf("Final answer: %llu\n", answer);
    
    // Освобождаем память
    free(threads);
    free(tasks);
    free(to);
    
    return 0;
}
