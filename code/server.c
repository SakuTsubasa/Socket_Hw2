#include <stdio.h>          // 標準輸入輸出函式庫
#include <stdlib.h>         // 標準庫，用於常用函式如 exit()
#include <string.h>         // 字串處理函式庫
#include <unistd.h>         // 提供訪問POSIX操作系統API
#include <sys/socket.h>     // 套接字接口函式
#include <netinet/in.h>     // 定義網際網路協議地址結構
#include <pthread.h> // 引入pthread庫

#define PORT 8080           // 定義服務器要監聽的端口
int clientID = 0;

struct file_permission {
    char filename[1024];
    char permission[10];
};

struct file_permission file_permissions[100];  // 假設我們最多只處理100個文件
int file_count = 0;  // 當前已經處理的文件數量

void *handle_client(void *client_socket) {
    int clientSocket = *(int*)client_socket;
    char buffer[1024];
    int bytesRead;
    int ID = clientID;
    int member;
    FILE *fp = NULL;
    printf("client%d connect!\n",ID);
    printf("請賦予身分組(1為AOS-members,2為CSE-members):\n");
    scanf("%d",&member);
    // 處理客戶端通訊的代碼
    // ...
    while (1) {
        //輸出字串給client
        // printf("Enter message: ");
        // fgets(buffer, 1024, stdin);
        // send(clientSocket, buffer, strlen(buffer), 0);

        memset(buffer, 0, 1024);  // 清空緩衝區

        bytesRead = read(clientSocket, buffer, 1024);  // 從客戶端讀取數據
        if (bytesRead < 0) {
            perror("Read");
            break;
        }
        printf("Message from client%d: %s\n",ID,buffer);

        // 檢查是否收到特定退出命令，例如 "exit"
        if (strcmp(buffer, "exit\n") == 0) {
            printf("client%d disconnect!\n",ID);
            break;
        }
        //客戶端下載文件
        if (strcmp(buffer, "read\n") == 0) {

            memset(buffer, 0, 1024);
            printf("client%d is trying to read something\n",ID);
            //A client想要的文件名稱
            read(clientSocket, buffer, 1024);
            FILE *fp = fopen(buffer,"r");
            if (fp ==NULL){
                printf("File %s not found!\n",buffer);
            }
            while(1){
                unsigned char buff[1024]={0};
                int nread = fread(buff,1,1024,fp);
                if(nread > 0){
                    //B 傳送文件給client         
                    write(clientSocket, buff, nread);
                }
                if(nread < 1024){
                    if (feof(fp))
                        printf("End of file\n");
                    if (ferror(fp))
                        printf("Error reading\n");
                    break;
                }
            }
            fclose(fp);

        }
        //客戶端上傳文件
        if (strcmp(buffer, "write\n") == 0) {
            memset(buffer, 0, 1024);
            //讀取客戶端的文件名稱
            read(clientSocket, buffer, 1024);
            FILE *fp;
            fp = fopen(buffer, "wb");
            memset(buffer, 0, 1024);
            if(NULL == fp){
                printf("Error opening file");
                return NULL;
            }

            while(1){
                unsigned char recvBuff[1024];
                int nread = read(clientSocket, recvBuff, 1024);
                if(nread > 0){
                    fwrite(recvBuff, 1, nread, fp);
                }
                if(nread < 1024){
                    if (feof(fp))
                        printf("End of file\n");
                    if (ferror(fp))
                        printf("Error reading\n");
                    break;
                }
                
            }
            printf("client%d的文件上傳成功\n",ID);
            fclose(fp);
        }
        //客戶端建立文件
        if (strcmp(buffer, "create\n") == 0) {
            memset(buffer, 0, 1024);
            //讀取客戶端的文件名稱、權限配置
            char filename[1024], permission[10];
            read(clientSocket, buffer, 1024);
            sscanf(buffer, "%s %s", filename, permission);
            FILE *fp;
            fp = fopen(filename, "wb");
            // 創建一個新的 file_permission 結構體
            strcpy(file_permissions[file_count].filename, filename);
            strcpy(file_permissions[file_count].permission, permission);
            printf("client%d的文件建立成功,權限為:%s\n",ID,file_permissions[file_count].permission);
            file_count++;
            fclose(fp);
        }
        //客戶端改變權限
        if (strcmp(buffer, "change\n") == 0) {
            // 從客戶端讀取文件名和新的權限
            read(clientSocket, buffer, 1024);
            char filename[1024], permission[10];
            sscanf(buffer, "%s %s", filename, permission);
            int index = 0;
            // 查找對應的 file_permission 結構體並更新權限
            for (int i = 0; i < file_count; i++) {
                if (strcmp(file_permissions[i].filename, filename) == 0) {
                    printf("%s權限改變成功,權限從%s改為%s\n",filename,file_permissions[i].permission,permission);
                    strcpy(file_permissions[i].permission, permission);
                    break;
                }
            }
            
        }
    }

    close(clientSocket);
    free(client_socket);
    return NULL;
}

int main() {
    int serverSocket, clientSocket, bytesRead; // 定義服務器套接字、客戶端套接字和讀取的字節數
    struct sockaddr_in serverAddress;          // 定義服務器地址結構
    int optionValue = 1;                       // 用於設置套接字選項的值
    int addressLength = sizeof(serverAddress); // 計算地址結構的長度
    char buffer[1024] = {0};                   // 定義數據緩衝區
    char *message = "Hello from server";       // 要發送給客戶端的消息

    // 創建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 設置套接字選項
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optionValue, sizeof(optionValue))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;            // 設置地址族為IPv4
    serverAddress.sin_addr.s_addr = INADDR_ANY;    // 設置IP地址為任意地址
    serverAddress.sin_port = htons(PORT);          // 將端口號轉換為網絡字節順序

    // 綁定套接字到指定的IP地址和端口
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 啟動監聽請求
    if (listen(serverSocket, 3) < 0) { // 設定最大的等待連接數為3
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    //無限LOOP 等待多個client連入
    while(1) {
        int *client_sock = malloc(sizeof(int)); //分配記憶體來儲存客戶端套接字的文件描述符。
        *client_sock = accept(serverSocket, (struct sockaddr *)&serverAddress, (socklen_t *)&addressLength);
        if (*client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t thread_id;
        //handle_client線程開始執行時呼叫的函數指針。這個函數必須接受一個 void * 參數並返回一個 void * 值。
        if(pthread_create(&thread_id, NULL, handle_client, (void*)client_sock) < 0) {
            perror("Could not create thread");
            continue;
        }
        clientID ++; //為每個客戶付上ID
    }

    // 接受來自客戶端的連接請求


// 持續接收和處理客戶端信息
    

// 關閉套接字
close(clientSocket);
close(serverSocket);

return 0;
}
