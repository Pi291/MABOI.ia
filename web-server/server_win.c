#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

//Директивы
#pragma comment(lib, "ws2_32.lib")

//Константы
#define PORT 8080
#define BUFFER_SIZE 4096
#define MAX_RESPONSE_SIZE 32768

typedef struct {
  char method[16];
  char path[256];
  char protocol{16};
  char* body;
  int body_length;
} http_request_t;

//Получаем mime файла
const char* git_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (ext != NULL) {
        ext++;
    //Подбираем расширение
        if (strcmp(ext, "html") == 0) return "text/html";
        if (strcmp(ext, "css") == 0) return "text/css";
        if (strcmp(ext, "js") == 0) return "text/javascript";
        if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) return "text/jpeg";
        if (strcmp(ext, "png") == 0) return "text/png";
        if (strcmp(ext, "gif") == 0) return "text/gif";
        if (strcmp(ext, "ico") == 0) return "text/x-icon";
    }
    return "text/plain";
}

char* read_file(const char* filename, long* size) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }

    //Перемещаем указатель в конец чтобы узнать длину файла
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //Выделяем память (из кучи) для содержимого файла + 1 для /0
    char* content = (char*)malloc(*size + 1);
    if (content == NULL) {
        fclose(file);
        return NULL;
    }

    //Закидываеи содержимое файла в выделенную память
    size_t bytes_read = fread(content, 1, *size, file);
    if (bytes_read != *size) {
        free(content);
        fclose(file);
        return NULL;
    }
    //Закрываем
    fclose(file);
    return content;
  }


    int main() {
    //Инициализация Winsock
    WSADATA wsaData;
    int wsaStartupResult = WSAStartup(AKEWORD(2, 2), &wsaData);

    //Проверка инициализации
    if (wsaStartupResult != 0) {
        printf("Error iniselize Winsock: %d\n", wsaStartupResult);
        return 1;
    }

    //Серверный сокет
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Can't create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    //Разрешаем повторное использование адреса
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        printf("Error setsockopt: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    //Настраеваем адрес сервера
    struct sockaddr_in address;
    //Заполняем структуру нулями
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //Привязываем сокет к адресу и порту
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Error connect (bind): %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    //Слушаем соеденения
    if (listen(server_fd, 10) == SOCKET_ERROR) {
        printf("Error listening (listen): %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Server start and listenning on port %d\n", PORT);
    printf("Open browser and go to lonk http://localhost:%d\n", PORT);

    while (1) {
        //Принимаем соеденение
        SOCKET client_socket;
        int addrlen = sizeof(address);
        //Для каждого клиента создаём свой соект
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);

        if (client_socket == INVALID_SOCKET) {
            printf("Error get  connect (accept): %d\n", WSAGetLastError());
            continue;
        }

        //Чекаем данные пользователя
        //Создайм бункер, инициализируем нулями
        char buffer[BUFFER_SIZE] = {0};
        //Читаем
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read == SOCKET_ERROR) {
            printf("Errpr read from socket: %d\n", WSAGetLastError());
            closesocket(client_socket);
            continue;
        }

        //Нулевой терминал для безопасности
        buffer[bytes_read] = '\0';
        printf("Get HTTP-request: \n%s\n", buffer);

        //Анализ запроса
        char method[16], path[256], protocol[16];

        //Расчленяем первую строку
        sscanf(buffer, "%15s %255s %15s", method, path, protocol);
        printf("Method: %s, path: %s, protocol: %s\n", method, path, protocol);


        //Буфер для ответа
        char filepath[512] = {0};

        //Проверяем запрошенный путь
        if (strcmp(path, "/") == 0) {
            strcpy(filepath, "index.html");
        } else {
            strcpy(filepath, &path[1]);
        }

        printf("Asked file: %s\n", filepath);

        //Читаем файл
        long file_size = 0;
        char* file_content = read_file(filepath, &file_size);

        char response[MAX_RESPONSE_SIZE];
        int response_length;
        if (file_content != NULL) {
            const char* mime_type = git_mime_type(filepath);
            printf("MIME-type: %s, file size: %ld bytes\n", mime_type, file_size);
            response_length = sprintf(
                response, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s; charset=utf-8\r\n"
                "Content-Length: %ld\r\n"
                "Connection: close\r\n"
                "\r\n",
                mime_type, file_size
            );
            memcpy(response + response_length, file_content, file_size);
            response_length += file_size;
            free(file_content);
        } else {
          const char* not_found_body =
            "<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>";
          response_length = sprintf(
              response, 
              "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %zu\r\n"
                "Connection: close\r\n"
                "\r\n"
                "%s",
                strlen(not_found_body), not_found_body
              );
        }

        //Отпровляем ответ
        int bytes_sent = send(client_socket, response, response_length, 0);
        if (bytes_sent == SOCKET_ERROR) {
            printf("Error send answer: %d\n", WSAGetLastError());
        } else {
            printf("Answer send correctly (%d bytes)\n", bytes_sent);
        }

        //Обрубаем соеденение
        closesocket(client_socket);
        printf("Connected on close\n\n");
        }
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
