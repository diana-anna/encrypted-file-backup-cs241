#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define CAPACITY 256

// limit to 3 clients at once
static int clients[3];

// void pre_client(char* tid) {
//
// }

int check_for_file(char* file_name){
    printf("Checking for file %s\n", file_name);
    FILE *fp = fopen(file_name, "r");
    if (fp) {
        return 1;
    } else {
        return 0;
    }
}

int send_file(int Socket, char* file_name){
    char msgBuff[CAPACITY];
    memset(msgBuff, 0, CAPACITY);

    if (file_name == NULL){
        printf("No filename found!\n");
        exit(1);
    }
    int file_exists = check_for_file(file_name);

    if (!file_exists){
        msgBuff[0] = '0';
        msgBuff[1] = 0;
        send(Socket, msgBuff, CAPACITY, 0);
        printf("File doesn't exist!\n");
        exit(1);
    } else{
        msgBuff[0] = '1';
        msgBuff[1] = 0;
        send(Socket, msgBuff, CAPACITY, 0);
    }

    FILE *fp = fopen(file_name,"rb");
    if(fp==NULL)
    {
        perror("File open error");
        exit(1);
    }

    // Read data from file and send it
    while(1)
    {
        // First read file in chunks of 256 bytes
        unsigned char buff[CAPACITY]={0};
        int nread = fread(buff,1,CAPACITY,fp);
        printf("Bytes read %d \n", nread);

        // If read was success, send data.
        if(nread > 0){
            printf("Sending \n");
            write(Socket, buff, nread);
        }

        // There is something tricky going on with read, either there was error, or we reached end of file.
        if (nread < 256){
            if (feof(fp))
                printf("End of file\n");
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }


    }
    fclose(fp);
    return 0;
}

int receive_file(int Socket, char* file_name){
    int bytesReceived = 0;
    char recvBuff[CAPACITY];
    char msgBuff[CAPACITY];
    memset(recvBuff, 0, CAPACITY);
    memset(msgBuff, 0, CAPACITY);

    //wait for response -- does file exist?
    recv(Socket, msgBuff, CAPACITY, 0);
    int exists_file = atoi(msgBuff);

    if (exists_file){
        // Create file where data will be stored
        FILE *fp;
        fp = fopen(file_name, "ab");
        if(fp == NULL){
            perror("Error opening file");
            exit(1);
        }

        // Receive data in chunks of CAPACITY bytes
        while((bytesReceived = read(Socket, recvBuff, CAPACITY)) > 0){
            printf("Bytes received %d\n",bytesReceived);
            fwrite(recvBuff, 1, bytesReceived, fp);
            if (bytesReceived < CAPACITY){
                if (feof(fp)){
                    printf("End of file\n");
                    fclose(fp);
                }
                if (ferror(fp)){
                    printf("Error reading\n");
                }
                break;
            }
        }

        if(bytesReceived < 0){
            perror("read error");
            exit(1);
        }
    }
    else{
        printf("File does not exist!\n");
        exit(1);
    }



    return 0;
}

int client(char* ip_addr, char* drive_dir) {
//int client(char* info) {

    int bytesReceived = 0;
    int clientSocket;
    char recvBuff[CAPACITY];
    memset(recvBuff, '0', sizeof(recvBuff));
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    char* input = NULL;
    size_t line_size;
    char* mode;
    char* file_name;

    if (drive_dir != NULL){
        chdir(drive_dir);
    }

    clientSocket = socket(PF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7891);
    serverAddr.sin_addr.s_addr = inet_addr(ip_addr);
    // Set all bits of the padding field to 0
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    addr_size = sizeof serverAddr;
    if((connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size)) == -1){
        perror("Connection Failed!");
        exit(1);
    }

    while(1){

        printf("Please enter a command.\n");
        if (getline(&input, &line_size, stdin) == -1){
            perror("getline failed");
            exit(1);
        }
        input[strlen(input)-1] = 0; //get rid of new line char
        send(clientSocket, input, CAPACITY, 0);
        mode = strtok(input, " ");
        file_name = strtok(NULL, " ");
        if (strcmp(mode, "receive") == 0){
            receive_file(clientSocket, file_name);
        }
        else if (strcmp(mode, "send") == 0){
            send_file(clientSocket, file_name);
        }
        else if (strcmp(mode, "close") == 0){
            break;
        }
    }

    close(clientSocket);

    return 0;
}

void* pre_client(void* info) {
    char* info_c = (char*) info;

    char* ip_addr;
    char* drive_dir;
    // split info to get individual vars
    char* token;
    token = strtok(info_c, " "); // get index
    clients[atoi(token)] = 1; // fill in client box

    token = strtok(NULL, " "); // get ip


    if (!token) { // ip_addr shouldn't be null
        perror("something with client info passed (ip_addr)");
        exit(1);
    }
    ip_addr = token;

    token = strtok(NULL, " "); // get dd if possible
    if (!token) { // get drive_dir
        drive_dir = NULL;
    } else {
        drive_dir = token;
    }

    client(ip_addr, drive_dir);
}

int server(char* drive_dir){
    int welcomeSocket, newSocket;
    char* serverIP = NULL;
    struct sockaddr_in serverAddr;
    struct ifaddrs *addrs = malloc(sizeof(struct ifaddrs));
    char action[1024];

    if (drive_dir != NULL){
        chdir(drive_dir);
    }

    // Create the socket. The three arguments are:
    // 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case)
    welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);

    //get this machine's IP address
    getifaddrs(&addrs);
    struct ifaddrs *tmp = addrs;

    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            printf("%s: %s\n", tmp->ifa_name, inet_ntoa(pAddr->sin_addr));
        }

        tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);

    //Let the user decide which IP address to use as the server's IP address
    size_t line_size;
    printf("Please input the IP address you would like to use for this server: \n");
    if (getline(&serverIP, &line_size, stdin) == -1){
        perror("getline failed");
        exit(1);
    }

    // Configure settings of the server address struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7891);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    // Set all bits of the padding field to 0
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    // Listen on the socket, with 5 max connection requests queued
    if(listen(welcomeSocket,5)==0)
    printf("Listening\n");
    else
    printf("Error\n");



    newSocket = accept(welcomeSocket, (struct sockaddr*)NULL ,NULL);

    while(1){

        if (recv(newSocket, action, 1024, 0) == -1){
            perror("recv failed");
            exit(1);
        }
        char* mode = strtok(action, " ");
        char* file_name = strtok(NULL, " ");
        printf("Command entered: %s %s\n", mode, file_name);
        if (strcmp(mode, "receive") == 0){
            //we need to send file to client
            send_file(newSocket, file_name);
        }
        else if (strcmp(mode, "send") == 0){
            //we need to receive the client's file
            receive_file(newSocket, file_name);
        }
        else if (strcmp(mode, "close") == 0){
            break;
        }

    }

    close(newSocket);

  return 0;
}

int main(int argc, char **argv){
    char* usage_msg = "Usage: must specify server or client\n";


    if (argc == 1){
        printf("%s", usage_msg);
        exit(1);
    }

    char* mode = argv[1];
    if (strcmp(mode, "server") == 0){
        if (argc == 2){
            server(NULL);
        } else if (argc == 3){
            server(argv[2]);
        }
        else{
            printf("Usage: program server optional_path_to_drive\n");
        }
    }
    else if (strcmp(mode, "client") == 0){

        if (clients[0] && clients[1] && clients[2]){
            // client limit reached
            printf("Client limit reached.\n");
            exit(1);
        }
        // otherwise, generate new thread for client and add it to global array
        if (argc < 3){
            printf("Usage: program client IP_addrs optional_path_to_drive\n");
            exit(1);
        } else if (argc == 3 || argc == 4) {


            // find spot in pthread array for this client
            int idx = -1;
            if (!clients[0]) {
                idx = 0;


            } else if (!clients[1]) {
                idx = 1;
            } else if (!clients[2]) {
                idx = 2;
            }

            if (idx < 0) {
                perror("reporting client limit reached when it shouldn't");
                exit(1);
            }
            

            char client_info[1024]; // will hold info for thread parameter
            memset(client_info, '\0', sizeof(client_info));

            char index_to_str[3];
            memset(index_to_str, '\0', sizeof(index_to_str));
            sprintf(index_to_str, "%d", idx); // index_to_str holds the index in clients[]

            char ip[50]; // ip
            memset(ip, '\0', sizeof(ip));
            strcpy(ip, argv[2]);


            if (argc == 4) { // if drive path exist
                char dd[970]; // drive path
                memset(dd, '\0', sizeof(dd));
                strcpy(dd, argv[3]);

                strcat(index_to_str, " ");
                strcpy(client_info, index_to_str); // client info holds idx + " "

                strcat(ip, " "); // ip with space at end
                strcat(client_info, ip); // client info holds idx + " " + ip + " "
                strcat(client_info, dd); // client info holds idx + " " + ip + " " + dd
            } else {
                strcat(index_to_str, " ");
                strcpy(client_info, index_to_str); // client info holds idx + " "

                strcat(client_info, ip); // client info holds idx + " " + ip


            }

            pthread_t client_id;
            int check = pthread_create(&client_id, NULL, pre_client, (void*) client_info);
            pthread_join(client_id, 0);

        // } else if (argc == 3){
        //     client(argv[2], NULL);
        // } else if (argc == 4){
        //     client(argv[2], argv[3]);
        } else {
            printf("Usage: program client IP_addrs ");
        }
    } else{
        printf("%s", usage_msg);
        exit(1);
    }

    return 0;
}

