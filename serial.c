#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

HANDLE open_serial_port(const char *port_name)
{
    HANDLE hSerial = CreateFile(
        port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hSerial == INVALID_HANDLE_VALUE)
        printf("Ошибка открытия %s\n", port_name);

    return hSerial;
}

int configure_serial(HANDLE hSerial, DWORD baudRate)
{
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        printf("Ошибка GetCommState\n");
        return 0;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        printf("Ошибка SetCommState\n");
        return 0;
    }

    return 1;
}

int set_timeouts(HANDLE hSerial)
{
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        printf("Ошибка SetCommTimeouts\n");
        return 0;
    }

    return 1;
}

void send_data(HANDLE hSerial, const char *data)
{
    DWORD bytesWritten;
    WriteFile(hSerial, data, strlen(data), &bytesWritten, NULL);
    WriteFile(hSerial, "\r\n", 2, &bytesWritten, NULL);
    printf("Отправлено: %s\n", data);
}

void receive_data(HANDLE hSerial)
{
    char buffer[256];
    DWORD bytesRead;

    if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        printf("Получено: %s\n", buffer);
    }
}

const char *choose_port()
{
    int choice;
    printf("Выберите COM порт:\n");
    for (int i = 1; i <= 10; i++)
        printf("%d - COM%d\n", i, i);
    printf("11 - Ввести вручную\n> ");

    scanf("%d", &choice);
    getchar(); 

    if (choice >= 1 && choice <= 10)
    {
        static char port[10];
        sprintf(port, "COM%d", choice);
        return port;
    }
    else if (choice == 11)
    {
        static char customPort[20];
        printf("Введите имя порта (например COM5): ");
        fgets(customPort, sizeof(customPort), stdin);
        customPort[strcspn(customPort, "\n")] = 0;
        return customPort;
    }
    else
    {
        printf("Неверный выбор, используется COM1\n");
        return "COM1";
    }
}

DWORD choose_baud()
{
    int choice;
    printf("Выберите скорость:\n");
    printf("1 - 9600\n2 - 19200\n3 - 38400\n4 - 57600\n5 - 115200\n6 - Ввести вручную\n> ");
    scanf("%d", &choice);
    getchar();

    switch (choice)
    {
    case 1:
        return CBR_9600;
    case 2:
        return CBR_19200;
    case 3:
        return CBR_38400;
    case 4:
        return CBR_57600;
    case 5:
        return CBR_115200;
    case 6:
    {
        unsigned long customBaud;
        printf("Введите скорость передачи: ");
        scanf("%lu", &customBaud);
        getchar();
        return customBaud;
    }
    default:
        return CBR_115200;
    }
}

int main()
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    HANDLE hSerial = INVALID_HANDLE_VALUE;

    while (1)
    {
        const char *port = choose_port();
        DWORD baud = choose_baud();

        hSerial = open_serial_port(port);
        if (hSerial == INVALID_HANDLE_VALUE)
            continue;

        if (!configure_serial(hSerial, baud) || !set_timeouts(hSerial))
        {
            CloseHandle(hSerial);
            continue;
        }

        printf("%s открыт с скоростью %lu. Введите текст для отправки (exit для выхода, stop для смены порта/скорости):\n", port, baud);

        char input[256];
        while (1)
        {
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "exit") == 0)
            {
                CloseHandle(hSerial);
                return 0;
            }

            if (strcmp(input, "stop") == 0)
            {
                CloseHandle(hSerial);
                break; 
            }

            send_data(hSerial, input);
            receive_data(hSerial);
        }
    }

    return 0;
}
