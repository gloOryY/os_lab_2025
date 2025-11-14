# Настройки компилятора

CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c11
LDFLAGS = -pthread

# Имена файлов и директорий
# Исходные файлы
UTILS_SRC = utils.c
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Объектные файлы
UTILS_OBJ = $(UTILS_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Имя статической библиотеки
# Префикс 'lib' и расширение '.a' - стандарт для статических библиотек в Linux
LIBRARY = libutils.a

# Исполняемые файлы
SERVER = server
CLIENT = client

# Основные цели

# Цель по умолчанию: собрать библиотеку, клиент и сервер
all: $(LIBRARY) $(SERVER) $(CLIENT)
	@echo "==================================="
	@echo "Сборка завершена успешно!"
	@echo "Библиотека: $(LIBRARY)"
	@echo "Серверы: ./$(SERVER) --port 20001 --tnum 4"
	@echo "Клиент: ./$(CLIENT) --k 100 --mod 1000000007 --servers servers.txt"
	@echo "==================================="

# Создание статической библиотеки

# Статическая библиотека создается из объектного файла utils.o
# Команда 'ar' (archiver) создает архив объектных файлов
# Флаги:
#   r - заменить существующие или добавить новые объектные файлы
#   c - создать архив, если он не существует (без предупреждения)
#   s - создать индекс символов (аналог ranlib)
$(LIBRARY): $(UTILS_OBJ)
	@echo "Создание статической библиотеки $(LIBRARY)..."
	ar rcs $(LIBRARY) $(UTILS_OBJ)
	@echo "Библиотека $(LIBRARY) создана успешно"

# Сборка исполняемых файлов

# Сервер зависит от своего объектного файла и библиотеки
# При линковке указываем библиотеку после объектных файлов
$(SERVER): $(SERVER_OBJ) $(LIBRARY)
	@echo "Линковка $(SERVER)..."
	$(CC) $(SERVER_OBJ) -o $(SERVER) $(LIBRARY) $(LDFLAGS)

# Клиент зависит от своего объектного файла и библиотеки
$(CLIENT): $(CLIENT_OBJ) $(LIBRARY)
	@echo "Линковка $(CLIENT)..."
	$(CC) $(CLIENT_OBJ) -o $(CLIENT) $(LIBRARY) $(LDFLAGS)

# Правила компиляции объектных файлов
# Паттерн-правило для компиляции .c -> .o
# $< - имя первой зависимости (исходный .c файл)
# $@ - имя цели (объектный .o файл)
%.o: %.c
	@echo "Компиляция $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Служебные цели

# Удаление всех сгенерированных файлов
clean:
	@echo "Очистка проекта..."
	rm -f $(SERVER) $(CLIENT) $(LIBRARY) *.o
	@echo "Проект очищен"

# Полная пересборка проекта
rebuild: clean all

# Установка библиотеки в систему (опционально)
# Требует прав суперпользователя: sudo make install
install: $(LIBRARY)
	@echo "Установка библиотеки в систему..."
	install -d /usr/local/lib
	install -m 644 $(LIBRARY) /usr/local/lib/
	install -d /usr/local/include
	install -m 644 utils.h /usr/local/include/
	@echo "Библиотека установлена в /usr/local/lib/"
	@echo "Заголовочный файл установлен в /usr/local/include/"

# Удаление библиотеки из системы (опционально)
uninstall:
	@echo "Удаление библиотеки из системы..."
	rm -f /usr/local/lib/$(LIBRARY)
	rm -f /usr/local/include/utils.h
	@echo "Библиотека удалена"

# Запуск тестовых серверов
run-servers: all
	@echo "Запуск серверов..."
	@./$(SERVER) --port 20001 --tnum 4 &
	@./$(SERVER) --port 20002 --tnum 4 &
	@./$(SERVER) --port 20003 --tnum 4 &
	@echo "Серверы запущены"

# Остановка серверов
stop-servers:
	@echo "Остановка серверов..."
	@pkill -f "./$(SERVER)" || echo "Серверы не запущены"

# Справка по доступным командам
help:
	@echo "Доступные команды:"
	@echo "  make          - собрать библиотеку, клиент и сервер"
	@echo "  make all      - то же самое"
	@echo "  make clean    - удалить скомпилированные файлы"
	@echo "  make rebuild  - пересобрать проект с нуля"
	@echo "  make install  - установить библиотеку в систему (требует sudo)"
	@echo "  make uninstall - удалить библиотеку из системы"
	@echo "  make run-servers - запустить тестовые серверы"
	@echo "  make stop-servers - остановить серверы"
	@echo "  make help     - показать эту справку"

# Указываем фиктивные цели (не являются файлами)
.PHONY: all clean rebuild install uninstall run-servers stop-servers help
