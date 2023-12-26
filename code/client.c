#include <stdio.h>          // 標準輸入輸出函式庫
#include <stdlib.h>         // 標準庫，用於常用函式如 exit()
#include <unistd.h>         // 提供訪問POSIX操作系統API
#include <sys/socket.h>     // 套接字接口函式
#include <netinet/in.h>     // 定義網際網路協議地址結構
#include <string.h>         // 字串處理函式庫
#include <arpa/inet.h>      // 定義網際網路操作函式

#define PORT 8080           // 定義要連接的服務器端口

void read_message(int socket, char *buffer, size_t max_length) {
    char *ptr = buffer;
    size_t left = max_length;
    while (left > 1) {
        int nread = read(socket, ptr, 1);
        if (nread <= 0) {
            break; // 斷開連接或錯誤
        }
        if (*ptr == '\n') { // 消息結束
            break;
        }
        ptr++;
        left--;
    }
    *ptr = '\0'; // 確保字符串結束
}

int main() {
    int clientSocket;                         // 定義客戶端套接字
    struct sockaddr_in serverAddress;         // 定義服務器地址結構
    char *message = "Hello from client";      // 定義要發送的消息
    char buffer[1024] = {0};                  // 定義數據緩衝區
    char username[50];
    char password[50];
    char server_reply[100];

    // 創建套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        printf("Socket creation error \n");
        return -1;
    }

    serverAddress.sin_family = AF_INET;                   // 設置地址族為IPv4
    serverAddress.sin_port = htons(PORT);                 // 將端口號轉換為網絡字節順序

    // 將IP地址從文本轉換為二進制
    if(inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported \n");
        return -1;
    }

    // 連接到服務器
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("Connection Failed \n");
        return -1;
    }

    printf("輸入帳號:");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // 去除換行符
    send(clientSocket, username, strlen(username), 0);
    printf("\n輸入密碼:");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0; // 去除換行符
    send(clientSocket, password, strlen(password), 0);
    
    read(clientSocket, server_reply, sizeof(server_reply));
    printf("Server response: %s\n", server_reply);
    if (strcmp(server_reply,"Login failed") !=0 ){
        while(1) {
        
        printf("Enter message: ");
        fgets(buffer, 1024, stdin);  // 從標準輸入讀取輸入

        // 發送消息到伺服器
        send(clientSocket, buffer, strlen(buffer), 0);

        // 如果輸入的是 "exit\n"，則跳出循環
        if (strcmp(buffer, "exit\n") == 0) {
            break;
        }
        //下載文件
        if (strcmp(buffer, "read\n") == 0) {
            char deny[1024];
            char permission[1024];
            char status[1024];
            memset(buffer, 0, sizeof(buffer));
            printf("Enter File nane: ");
            fgets(buffer, 1024, stdin);
            //去除\n
            buffer[strcspn(buffer,"\n")]=0;
            //A傳送想要的文件名稱給Server
            send(clientSocket, buffer, strlen(buffer),0);
            //B接收檔案的讀寫狀態
            memset(status, 0, sizeof(status));
            read_message(clientSocket, status, 1024);
            if (strcmp(status,"safe")==0){
                memset(permission, 0, sizeof(permission));
                memset(status, 0, sizeof(status));
                //C接收檔案的權限狀態
                read(clientSocket, permission, 7);
                if (strcmp(permission, "accept") == 0){
                    printf("Permission accept\n");
                    printf("正在下載檔案...\n");
                    FILE *fp = fopen(buffer,"wb");
                    while(1){
                        unsigned char recvBuff[1024];
                        memset(recvBuff, 0, 1024);  
                        int nread = read(clientSocket, recvBuff, 1024);
                        if(nread > 0){
                            //B 收到文件
                            fwrite(recvBuff, 1, nread, fp);
                        }
                        if(nread < 1024){
                            if (strcmp(recvBuff, " ") == 0) {
                            break;
                        }   
                            if (feof(fp))
                                printf("End of file\n");
                            if (ferror(fp))
                                printf("Error reading\n");
                            break;
                        }
                        
                        break;
                    }
                    printf("下載完成\n");
                    memset(status, 0, 1024);
                    fclose(fp);
                }else if((strcmp(permission, "reject") == 0)){
                    printf("Permission reject\n");
                }else{
                    printf("Error C: permission:%s\n",permission);
                }
            }else if (strcmp(status,"lock")==0){
                printf("檔案正在被使用,晚點再試\n");
                memset(status, 0, 1024);
            }else{
                //read(clientSocket, permission, 6);
                printf("Error B: status:%s\n",status);
                memset(status, 0, 1024);
            }
            
            
        }
        //上傳文件
        if (strcmp(buffer, "write\n") == 0) {
            char permission[1024];
            char status[1024];
            memset(buffer, 0, sizeof(buffer));
            //輸入文件名稱
            printf("Enter File nane and mode(overwrite/append): ");
            fgets(buffer, 1024, stdin);
            //去除\n
            buffer[strcspn(buffer,"\n")]=0;
            //A傳送文件名稱與模式
            send(clientSocket, buffer, strlen(buffer), 0);
            
            //接收檔案的讀寫狀態
            memset(status, 0, sizeof(status));
            read_message(clientSocket, status, 1024);
            if (strcmp(status,"safe")==0){
                memset(permission, 0, sizeof(permission));
                //B接收檔案的權限狀態
                read(clientSocket, permission, 6);
                if(strcmp(permission,"accept") ==0){
                    printf("請輸入內容:\n");
                    //開始寫檔案
                    memset(buffer, 0, sizeof(buffer));
                    fgets(buffer, 1024, stdin);
                    //去除\n
                    buffer[strcspn(buffer,"\n")]=0;
                    //C傳送文件內容
                    send(clientSocket, buffer, strlen(buffer), 0);
                }else{
                    printf("不符合權限\n");
                }
                
            }else if(strcmp(status,"lock") ==0){
                printf("檔案正在被使用,晚點再試\n");
            }else{
                printf("Error B: status:%s\n",status);
            }
            //printf("%s\n",status);
            
            
        }
        //建立文件
        if (strcmp(buffer, "create\n") == 0) {
            memset(buffer, 0, 1024);
            //輸入文件名稱
            printf("請輸入文件名稱與讀寫權限: ");
            fgets(buffer, 1024, stdin);
            //去除\n
            buffer[strcspn(buffer,"\n")]=0;
            send(clientSocket, buffer, strlen(buffer), 0);            
            memset(buffer, 0, 1024);
            
        }
        //改變權限
        if (strcmp(buffer, "change\n") == 0) {
            memset(buffer, 0, 1024);
            //輸入文件名稱與mode
            printf("Enter File nane and mode: ");
            fgets(buffer, 1024, stdin);
            //去除\n
            buffer[strcspn(buffer,"\n")]=0;
            send(clientSocket, buffer, strlen(buffer), 0);            
            memset(buffer, 0, 1024);
            read(clientSocket, buffer, 1024);
            printf("%s\n",buffer);
        }

        memset(buffer, 0, 1024);  // 清空緩衝區
        // 從伺服器接收消息
        // read(clientSocket, buffer, 1024);
        // printf("Message from server: %s\n", buffer);
    }
    }
// 關閉套接字
    close(clientSocket);

    return 0;
}

