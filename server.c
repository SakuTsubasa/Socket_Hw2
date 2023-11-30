#include <stdio.h>          // 匯入標準輸入輸出庫
#include <stdlib.h>         // 匯入標準庫，包含常用函式如 exit()
#include <string.h>         // 匯入字串處理函式
#include <unistd.h>         // 提供訪問 POSIX 操作系統 API 的功能
#include <sys/socket.h>     // 匯入套接字接口
#include <netinet/in.h>     // 匯入用於網際網路操作的結構和函式

#define PORT 8080           // 定義要使用的端口號

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};  // 設定一個接收資料的緩衝區
    char *hello = "Hello from server";  // 回應客戶端的訊息

    // 創建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 附加套接字到端口8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;         // 設置地址族為 AF_INET (IPv4)
    address.sin_addr.s_addr = INADDR_ANY; // 監聽來自任何網路地址的連接
    address.sin_port = htons(PORT);       // 將端口號轉換為網絡字節順序

    // 將套接字綁定到端口8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 監聽來自客戶端的連接請求
    if (listen(server_fd, 3) < 0) {  // 設置最大的等待連接數為3
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 接受來自客戶端的連接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // 讀取客戶端發送的資料
    valread = read(new_socket, buffer, 1024);
    printf("%s\n", buffer);

    // 向客戶端發送訊息
    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    return 0;
}
