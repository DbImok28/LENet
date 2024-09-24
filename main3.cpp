#include <windows.h>
#include <iostream>

int main()
{
	// Создаем порт завершения ввода-вывода
	HANDLE ioCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	// Создаем файл для записи
	HANDLE fileHandle = CreateFile(TEXT("output.txt"), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	// Добавляем дескриптор файла в порт завершения ввода-вывода
	CreateIoCompletionPort(fileHandle, ioCompletionPort, 0, 0);

	// Сообщение для записи
	const char message[] = "Hello, world!";

	// Создаем буфер для записи
	DWORD bytesWritten = 0;
	DWORD bufferSize = sizeof(message);
	char* buffer = new char[bufferSize];
	memcpy(buffer, message, bufferSize);

	OVERLAPPED woverlapped{};
	// Записываем данные в файл
	WriteFile(fileHandle, buffer, bufferSize, &bytesWritten, &woverlapped);

	// Ожидаем завершения операции записи
	LPOVERLAPPED overlapped = nullptr;
	DWORD bytesTransferred = 0;
	ULONG_PTR key;
	BOOL result = GetQueuedCompletionStatus(ioCompletionPort, &bytesTransferred, &key, &overlapped, INFINITE);

	// Проверяем результат операции
	std::cout << "Bytes transferred: " << bytesTransferred << std::endl;

	// Закрываем дескрипторы
	CloseHandle(fileHandle);
	CloseHandle(ioCompletionPort);

	// Освобождаем память
	delete[] buffer;

	return 0;
}