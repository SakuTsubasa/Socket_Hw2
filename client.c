#include <stdio.h>          // 匯入標準輸入輸出庫
#include <stdlib.h>         // 匯入標準庫
#include <unistd.h>         // 提供訪問 POSIX 操作系統 API 的功能
#include <sys/socket.h>     // 匯入套接字接口
#include <netinet/in.h>     // 匯入用於網際網路操作的結構和函式
#include <string.h>         // 匯入字串處理函式
#include <arpa/inet.h>      // 匯入用於網際網路操作的函式

#define PORT 8080           // 定義要連接的伺服器端口號

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";  // 客戶端要發送的訊息
    char buffer[1024] = {0};            // 設定一個接收資料的緩衝區

    // 創建套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;     // 設置地址族為 AF_INET (IPv4)
    serv_addr.sin_port = htons(PORT);   // 將端口號轉換為網絡字節順序

    // 將 IPv4 地址從文字轉換為二進制形式
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // 連接到伺服器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // 向伺服器發送訊息
    send(sock, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    // 從伺服器讀取回應
    valread = read(sock, buffer, 1024);
    printf("%s\n", buffer);

    return 0;
}
