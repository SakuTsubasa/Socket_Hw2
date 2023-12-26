#include <stdio.h>          // 標準輸入輸出函式庫
#include <stdlib.h>         // 標準庫，用於常用函式如 exit()
#include <string.h>         // 字串處理函式庫
#include <unistd.h>         // 提供訪問POSIX操作系統API
#include <sys/socket.h>     // 套接字接口函式
#include <netinet/in.h>     // 定義網際網路協議地址結構
#include <pthread.h> // 引入pthread庫

#define PORT 8080           // 定義服務器要監聽的端口
int clientID = 0;
int flag=0;

typedef struct {
    char username[50];
    char password[50];
} Credential;

Credential credentials[] = {
    {"AOS1", "1"},
    {"AOS2", "2"},
    {"AOS3", "3"},
    {"CSE1", "1"},
    {"CSE2", "2"},
    {"CSE3", "3"}
};
int credential_count = 6;

int check_credentials(const char* username, const char* password) {
    for (int i = 0; i < credential_count; ++i) {
        if (strcmp(credentials[i].username, username) == 0 &&
            strcmp(credentials[i].password, password) == 0) {
            return 1; // 認證成功
        }
    }
    return 0; // 認證失敗
}

struct file_permission {
    char filename[1024];
    int filelen;
    char permission[10];
    int ownerID;
    int group;
    int flag;
    int year;
    int month;
    int day;
    pthread_mutex_t lock;
};

struct file_permission file_permissions[100];  // 假設我們最多只處理100個文件
int file_count = 0;  // 當前已經處理的文件數量

void *handle_client(void *client_socket) {
    int clientSocket = *(int*)client_socket;
    char buffer[1024];
    int bytesRead;
    int ID = clientID;
    int memberID;
    char filename[1024];
    char permission[1024];
    char status[1024];
    FILE *fp = NULL;
    printf("client%d connect!\n",ID);
    
    // 處理客戶端通訊的代碼
    // ...
    char username[50];
    char password[50];
    printf("等待client輸入帳號:");
    read(clientSocket, username, sizeof(username)); // 接收用戶名
    printf("%s\n等待client輸入密碼:",username);
    read(clientSocket, password, sizeof(password)); // 接收密碼

    if (check_credentials(username, password)) {
        // 認證成功
        send(clientSocket, "Login successful", 17, 0);
        printf("%s\n請賦予身分組(1為AOS-members,2為CSE-members):\n",password);
        scanf("%d",&memberID);
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

        if (strcmp(buffer, "reset\n") == 0){
            char filename[1024];
            char permission[1024];
            char status[1024];
            memset(buffer, 0, 1024);
            memset(filename, 0, 1024);
            memset(permission, 0, 1024);
            memset(status, 0, 1024);
        }
        if (strcmp(buffer, "ls\n") == 0){
            for (int i = 0; i < file_count; i++){
                if (file_permissions[i].group ==1){
                    printf("%s client%d AOS %d %d %d %d %s\n",file_permissions[i].permission,file_permissions[i].ownerID,file_permissions[i].filelen,file_permissions[i].month,file_permissions[i].day,file_permissions[i].year,file_permissions[i].filename);
                }else{
                    printf("%s client%d CSE %d %d %d %d %s\n",file_permissions[i].permission,file_permissions[i].ownerID,file_permissions[i].filelen,file_permissions[i].month,file_permissions[i].day,file_permissions[i].year,file_permissions[i].filename);
                }
                
            }
        }
        // 檢查是否收到特定退出命令，例如 "exit"
        if (strcmp(buffer, "exit\n") == 0) {
            printf("client%d disconnect!\n",ID);
            break;
        }
        //客戶端下載文件
        if (strcmp(buffer, "read\n") == 0) {
            char filename[1024];
            char permission[1024];
            char status[1024];
            memset(buffer, 0, 1024);
            printf("等待client%d輸入檔案名稱\n",ID);
            //A client想要的文件名稱
            read(clientSocket, buffer, 1024);
            //抓取檔案名稱         
            sscanf(buffer, "%s", filename);
            for (int i = 0; i < file_count; i++) {
                if (strcmp(file_permissions[i].filename, filename) == 0) {
                    if(file_permissions[i].flag==1){
                        printf("%s正在被使用中,終止client%d操作\n",file_permissions[i].filename,ID);
                        fflush(stdout);
                        //B傳送檔案的讀寫狀態
                        send(clientSocket, "lock\n", 5, 0);
                        break;
                    }else{
                        //B傳送檔案的讀寫狀態
                        send(clientSocket, "safe\n", 5, 0);
                        printf("---------------------------\n");
                        printf("%s\n擁有者ID:%d\n群組ID:%d\n權限:",filename,file_permissions[i].ownerID,file_permissions[i].group);
                        for (int j=0;j<=5;j++ ){
                            printf("%c",file_permissions[i].permission[j]);
                        }
                        printf("\n---------------------------\n");
                        printf("client\n身分ID:%d\n群組ID:%d",ID,memberID);
                        printf("\n---------------------------\n");
                        
                        //如果 不是檔案擁有者 且 其他人權限不是r 且 群組成員權限不是r 則deny
                        if (file_permissions[i].permission[4] == 'r' || (file_permissions[i].permission[0] == 'r' && file_permissions[i].ownerID == ID) || (file_permissions[i].permission[2] == 'r' && file_permissions[i].group == memberID) ) {
                            file_permissions[i].flag=2;
                            //通過權限後回傳檔案名稱
                            printf("Permission accept\n");
                            //C傳送檔案的權限狀態
                            send(clientSocket, "accept", 6,0);
                            printf("正在傳送檔案...\n");
                            sleep(10);
                            FILE *fp = fopen(buffer,"r");
                            if (fp ==NULL){
                                printf("File %s not found!\n",buffer);
                            }
                            while(1){
                                unsigned char buff[1024]={0};
                                memset(buff, 0, 1024);
                                int nread = fread(buff,1,1024,fp);
                                if(nread > 0){
                                    //B 傳送文件給client         
                                    write(clientSocket, buff, nread);
                                }
                                if(nread < 1024){
                                    if (feof(fp))
                                        printf("End of file\n");
                                        char* endSignal = " ";
                                        write(clientSocket, endSignal, strlen(endSignal));
                                        fclose(fp);
                                    if (ferror(fp))
                                        printf("Error reading\n");
                                    break;
                                }
                            }
                            file_permissions[i].flag=0;
                            printf("傳送完成...\n");
                            break;
                        }else{
                            //C傳送檔案的權限狀態
                            send(clientSocket, "reject", 6,0);
                            printf("Permission denied\n");
                        }
                    }
                    break;         
                }
            }   
        }
        //客戶端上傳文件
        if (strcmp(buffer, "write\n") == 0) {
            time_t now = time(NULL);

            // 將 time_t 轉換為 tm 結構
            struct tm *local = localtime(&now);

            // 從 tm 結構中獲取年、月、日
            int year = local->tm_year + 1900;  // tm_year 是從 1900 年開始的年份
            int month = local->tm_mon + 1;     // tm_mon 是從 0 開始的月份
            int day = local->tm_mday;          // tm_mday 是一個月中的日期
            char filename[1024];
            char mode[2]; // 存儲一個字符和 '\0'
            memset(buffer, 0, sizeof(buffer));
            //A讀取客戶端的文件名稱
            read(clientSocket, buffer, 1024);
            memset(filename, 0, sizeof(filename));
            memset(mode, 0, sizeof(mode));
            sscanf(buffer, "%s %c", filename, mode);
            //printf("%s %s %s\n",buffer,filename,mode);
            //遍歷所有檔案
            for (int i = 0; i < file_count; i++){
                //找到客戶端要的檔案
                if (strcmp(file_permissions[i].filename, filename) == 0){
                    if(file_permissions[i].flag==1 || file_permissions[i].flag==2){
                        printf("%s正在被使用中,終止client%d操作\n",file_permissions[i].filename,ID);
                        fflush(stdout);
                        send(clientSocket, "lock\n", 5, 0);
                        break;
                    }else{                   
                        send(clientSocket, "safe\n", 5, 0);
                        printf("---------------------------\n%s\n擁有者ID:%d\n群組ID:%d\n權限:",filename,file_permissions[i].ownerID,file_permissions[i].group);
                        for (int j=0;j<=5;j++ ){
                            printf("%c",file_permissions[i].permission[j]);
                        }
                        printf("\n---------------------------\nclient\n身分ID:%d\n群組ID:%d\n---------------------------\n",ID,memberID);
                        if (file_permissions[i].permission[5] == 'w' || 
                        (file_permissions[i].permission[1] == 'w' && file_permissions[i].ownerID == ID) || 
                        (file_permissions[i].permission[3] == 'w' && file_permissions[i].group == memberID) ){
                            file_permissions[i].flag=1;
                            fflush(stdout);
                            printf("%s上鎖,被client%d使用\n",file_permissions[i].filename,ID);
                            //B傳送檔案的權限狀態
                            int temp;
                            temp=20000;
                            send(clientSocket, "accept", 6,0);
                            if (strcmp(mode,"o") == 0){
                                FILE *fp;
                                fp = fopen(filename, "wb");
                                memset(buffer, 0, 1024);
                                if(NULL == fp){
                                    printf("Error opening file");
                                    return NULL;
                                }
                                while(1){
                                    unsigned char recvBuff[1024];
                                    memset(recvBuff, 0, 1024);
                                    printf("等待client%d輸入內容:\n",ID);
                                    //C接收文件內容                                   
                                    int nread = read(clientSocket, recvBuff, 1024);                                  
                                    if (temp==20000)
                                        temp=nread;
                                    //開始寫檔案
                                    printf("覆蓋,內容為:%s\n",recvBuff);
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
                                file_permissions[i].filelen=temp;
                                printf("client%d的文件上傳成功\n",ID);
                                fclose(fp);
                                file_permissions[i].flag=0;
                            }else if(strcmp(mode,"a") == 0){
                                FILE *fp;
                                fp = fopen(filename, "ab");
                                memset(buffer, 0, 1024);
                                if(NULL == fp){
                                    printf("Error opening file");
                                    return NULL;
                                }
                                while(1){
                                    unsigned char recvBuff[1024];
                                    memset(recvBuff, 0, 1024);
                                    printf("等待client%d輸入內容:\n",ID);
                                    //C接收文件內容                                   
                                    int nread = read(clientSocket, recvBuff, 1024);
                                    if (temp==-1)
                                        temp==nread;
                                    //開始寫檔案
                                    printf("添增,內容為:%s\n",recvBuff);
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
                                file_permissions[i].filelen=temp;
                                fclose(fp);
                                file_permissions[i].flag=0;
                            }else{
                                printf("Error C, mode:%s\n",mode);
                            }
                            
                        }else{
                            //B傳送檔案的權限狀態
                            send(clientSocket, "reject", 6,0);
                        }
                        break;
                    }
                    
                }

            }
            
            
        }
        //客戶端建立文件
        if (strcmp(buffer, "create\n") == 0) {
            time_t now = time(NULL);

            // 將 time_t 轉換為 tm 結構
            struct tm *local = localtime(&now);

            // 從 tm 結構中獲取年、月、日
            int year = local->tm_year + 1900;  // tm_year 是從 1900 年開始的年份
            int month = local->tm_mon + 1;     // tm_mon 是從 0 開始的月份
            int day = local->tm_mday;          // tm_mday 是一個月中的日期
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
            file_permissions[file_count].ownerID = ID;
            file_permissions[file_count].group = memberID;
            file_permissions[file_count].filelen = 0;
            file_permissions[file_count].year = year;
            file_permissions[file_count].month = month;
            file_permissions[file_count].day = day;
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
                    if (file_permissions[i].ownerID==ID){
                        printf("%s權限改變成功,權限從%s改為%s\n",filename,file_permissions[i].permission,permission);
                        char *message = "權限改變成功";
                        send(clientSocket, message, strlen(message), 0);
                        strcpy(file_permissions[i].permission, permission);
                        break;
                    }else if(file_permissions[i].ownerID!=ID){
                        printf("權限改變失敗，client並非檔案擁有者\n");
                        char *message = "權限改變失敗，你並非檔案擁有者";
                        send(clientSocket, message, strlen(message), 0);
                        break;
                    }
                }
            }
            
        }
    }
    } else {
        // 認證失敗
        printf("%s\n客戶登入失敗\n",password);
        send(clientSocket, "Login failed", 12, 0);
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
