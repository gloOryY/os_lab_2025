#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "revert_string.h"

int main(int argc, char *argv[])
{
	// Проверка аргументов
	if (argc != 2)
	{
		printf("Usage: %s string_to_revert\n", argv[0]); //Если аргументов не 2 (программа + строка), выводится сообщение об использовании
		return -1;
	}
	// Выделение памяти
	char *reverted_str = malloc(sizeof(char) * (strlen(argv[1]) + 1)); // Выделяется память для копии строки (+1 для нулевого терминатора)
	// Копирование строки
	strcpy(reverted_str, argv[1]); // Создается копия оригинальной строки для работы

	//  Вызов функции переворота
	RevertString(reverted_str);

	printf("Reverted: %s\n", reverted_str);
	free(reverted_str);
	return 0;
}

