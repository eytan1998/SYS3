
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <mqueue.h>


#define ERROR 1
#define FALSE 0
#define TRUE 1

#define TIME_OUT 100
#define DATA_BUFFER_SIZE 8192
#define UDS_FILE_NAME "UDS-FILE"
#define CHAT_BUFFER_SIZE 1024 //Maximum length of the information sent in chat for the 1st time

#define DATA_SIZE 104857600 // 100MB


unsigned int checksum(const char *data, size_t len) {
    unsigned int sum = 0;
    size_t i;
    for (i = 0; i < len; ++i)
        sum += data[i];
    return sum;
}

char *generateData(int isQuiet) {
    char *data = malloc(DATA_SIZE);
    for (int i = 0; i < DATA_SIZE; ++i) {
        data[i] = '$';
    }
    if (data == NULL) {
        perror("malloc data");
        return NULL;
    }
    if (!isQuiet)printf("checksum: %u\n", checksum(data, DATA_SIZE));
    return data;
}

int server_ipv4_TCP(int chat_socket, int port, int isQuiet) {
    if (!isQuiet)printf("------------server ipv4 tcp------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    int receiver_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (receiver_socket < 0) {
        perror("[-] server ipv4 tcp socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv4 tcp socket.\n");

    struct sockaddr_in receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);
    socklen_t addr_size = sizeof(client_addr);

    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
        perror("[-] server ipv4 tcp setsockopt(SO_REUSEADDR) failed");

    /*********************************/
    /*          bind()             */
    /*********************************/
    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("[-] server ipv4 tcp bind");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv4 tcp bind.\n");
    /*********************************/
    /*          listen()             */
    /*********************************/
    if (listen(receiver_socket, 1) < 0) {
        perror("[-] server ipv4 tcp listen");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv4 tcp listening.\n");

    //send to client to try to connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);

    /*********************************/
    /*          accept()             */
    /*********************************/
    int connection_socket = accept(receiver_socket, (struct sockaddr *) &client_addr, &addr_size);
    if (connection_socket < 0) {
        perror("[-] server ipv4 tcp accept");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv4 tcp connect.\n");

    /*********************************/
    /*          recv size             */
    /*********************************/
    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);


    /*********************************/
    /*          recv data            */
    /*********************************/
    //to measure time

    long time;
    struct timeval start, end;

    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    if (!isQuiet)printf("[+] server ipv4 tcp receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recv(connection_socket, data_buffer, DATA_BUFFER_SIZE, 0);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            perror("[-] receiving data.\n");
            break;
        }

    }
    gettimeofday(&end, NULL);
    /*********************************/
    /*     calc and print time       */
    /*********************************/
    time = (end.tv_sec - start.tv_sec) * 1000;
    time += (end.tv_usec - start.tv_usec) / 1000;
    printf("ipv4_tcp,%ld\n", time);
    return 0;
}

int client_ipv4_TCP(int chat_socket, char *ip, int port, int isQuiet) {
    if (!isQuiet)printf("------------client ipv4 tcp------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-] client ipv4 tcp socket");


    }
    if (!isQuiet)printf("[+] client ipv4 tcp socket.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] client ipv4 tcp setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;

    /*********************************/
    /*          connect()             */
    /*********************************/

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("[-] client ipv4 tcp connect");
        return ERROR;
    }
    if (!isQuiet)printf("[+] client ipv4 tcp connect.\n");


    /*********************************/
    /*          send size             */
    /*********************************/
    char *data = generateData(isQuiet);
    unsigned long un = strlen(data);
    printf("Sending file size : %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &un, sizeof(unsigned long), 0);

    /*********************************/
    /*          send data            */
    /*********************************/
    send(sock, data, strlen(data), 0);

    free(data);
    return 0;
}

int server_ipv6_TCP(int chat_socket, int port, int isQuiet) {
    if (!isQuiet)printf("------------server ipv6 tcp------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    int receiver_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (receiver_socket < 0) {
        perror("[-] server ipv6 tcp socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv6 tcp socket.\n");

    struct sockaddr_in6 receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin6_family = AF_INET6;
    receiver_addr.sin6_port = htons(port);

    socklen_t addr_size = sizeof(client_addr);

    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
        perror("[-] server ipv6 tcp setsockopt(SO_REUSEADDR) failed");

    /*********************************/
    /*          bind()             */
    /*********************************/
    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("[-] server ipv6 tcp bind");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv6 tcp bind.\n");

    /*********************************/
    /*          listen()             */
    /*********************************/
    if (listen(receiver_socket, 1) < 0) {
        perror("[-] server ipv6 tcp listen");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv6 tcp listening.\n");

    //send the ipv6 address to client
    if (send(chat_socket, &receiver_addr.sin6_addr, sizeof(receiver_addr.sin6_addr), 0) < 0) {
        perror("send ipv6 address");
        return ERROR;
    }
    //send to client to try connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);

    /*********************************/
    /*          accept()             */
    /*********************************/
    int connection_socket = accept(receiver_socket, (struct sockaddr *) &client_addr, &addr_size);
    if (connection_socket < 0) {
        perror("[-] client ipv6 accept.");
        return ERROR;
    }
    if (!isQuiet)printf("[+] client ipv6 accept.\n");

    /*********************************/
    /*          recv size             */
    /*********************************/
    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    /*********************************/
    /*          recv data            */
    /*********************************/
    //to measure time
    long time;
    struct timeval start, end;

    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    if (!isQuiet)printf("[+] receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recv(connection_socket, data_buffer, DATA_BUFFER_SIZE, 0);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            perror("[-] server ipv6 tcp receiving.\n");
            break;
        }

    }
    gettimeofday(&end, NULL);
    /*********************************/
    /*     calc and print time       */
    /*********************************/
    time = (end.tv_sec - start.tv_sec) * 1000;
    time += (end.tv_usec - start.tv_usec) / 1000;
    printf("ipv6_tcp,%ld\n", time);
    return 0;
}

int client_ipv6_TCP(int chat_socket, int port, int isQuiet) {
    if (!isQuiet)printf("------------client ipv6 tcp------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-] client ipv6 tcp socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] client ipv6 tcp socket.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] client ipv6 tcp setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in6 addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    //get address
    if (recv(chat_socket, &addr.sin6_addr, sizeof(addr.sin6_addr), 0) == -1) {
        perror("[-] recv ipv6 address");
        return ERROR;
    }

    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;

    /*********************************/
    /*          connect()             */
    /*********************************/
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("[-] client ipv6 tcp connect");
        return ERROR;
    }
    if (!isQuiet)printf("[+] client ipv6 tcp connect.\n");

    char *data = generateData(isQuiet);
    /*********************************/
    /*          send size             */
    /*********************************/
    //sending file size
    unsigned long un = strlen(data);
    if (!isQuiet)printf("Sending file size : %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &un, sizeof(unsigned long), 0);

    /*********************************/
    /*          send data            */
    /*********************************/
    send(sock, data, strlen(data), 0);
    free(data);

    return 0;
}

int server_ipv4_UDP(int chat_socket, int port, int isQuiet) {
    if (!isQuiet)printf("------------server ipv4 udp------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    int receiver_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiver_socket < 0) {
        perror("[-] server ipv4 udp socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv4 udp socket.\n");

    struct sockaddr_in receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);
    socklen_t addr_size = sizeof(client_addr);

    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] server ipv4 udp setsockopt(SO_REUSEADDR) failed");
    }
    //make time out, stop if don't recv more
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIME_OUT;
    if (setsockopt(receiver_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("[-] server ipv4 udp SO_RCVTIMEO Error");
    }

    /*********************************/
    /*          bind()             */
    /*********************************/
    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("[-] server ipv4 udp bind");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv4 udp bind.\n");

    //send to client to try to connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);

    /*********************************/
    /*          recv size             */
    /*********************************/
    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    /*********************************/
    /*          recv data            */
    /*********************************/
    //to measure time
    long time;
    struct timeval start, end;

    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    if (!isQuiet)printf("[+] server ipv4 udp receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recvfrom(receiver_socket, data_buffer, DATA_BUFFER_SIZE, 0,
                                           (struct sockaddr *) &client_addr, &addr_size);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("[-] server ipv4 udp receiving.\n");
                break;
            }

        }

    }
    gettimeofday(&end, NULL);
    /*********************************/
    /*     calc and print time       */
    /*********************************/

    time = (end.tv_sec - start.tv_sec) * 1000;
    time += (end.tv_usec - start.tv_usec) / 1000;
    printf("ipv4_udp,%ld\n", time);
    return 0;
}

int client_ipv4_UDP(int chat_socket, char *ip, int port, int isQuiet) {
    if (!isQuiet)printf("------------client ipv4 udp------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] client ipv4 udp socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] client ipv4 udp socket.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] client ipv4 socket setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);

    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;

    /*********************************/
    /*          send size             */
    /*********************************/
    char *data = generateData(isQuiet);
    //sending file size
    unsigned long data_size = strlen(data);
    if (!isQuiet)printf("Sending file size : %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &data_size, sizeof(unsigned long), 0);

    /*********************************/
    /*          send data             */
    /*********************************/
    //track how much send, for moving pointer;
    int data_index = 0;
    //send data in chunks
    while (data_size > 0) {
        //not send more
        int chunk_size = DATA_BUFFER_SIZE;
        if (data_size < chunk_size)chunk_size = data_size;

        ssize_t data_send = sendto(sock, data + data_index, chunk_size, 0, (struct sockaddr *) &addr, sizeof(addr));
        if (data_send < 0) {
            perror("[-] client ipv4 udp send");
            return ERROR;
        }
        data_index += data_send;
        data_size -= data_send;
    }
    free(data);

    return 0;
}

int server_ipv6_UDP(int chat_socket, int port, int isQuiet) {
    if (!isQuiet)printf("------------server ipv6 udp------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    int receiver_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (receiver_socket < 0) {
        perror("[-]server ipv6 UDP Socket error");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv6 UDP socket created.\n");

    struct sockaddr_in6 receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin6_family = AF_INET6;
    receiver_addr.sin6_port = htons(port);
    socklen_t addr_size = sizeof(client_addr);

    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] server ipv6 UDP set_sockopt(SO_REUSEADDR) failed");
    }
    //make time out, stop if you don't recv more
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIME_OUT;
    if (setsockopt(receiver_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("[-] server ipv6 UDP set_sockopt(SO_RCVTIMEO) failed");
    }
    /*********************************/
    /*          bind()             */
    /*********************************/
    if (
            bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("[-]server ipv6 udp bind");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server ipv6 udp bind.\n");

    //send the ipv6 address to client
    if (send(chat_socket, &receiver_addr.sin6_addr, sizeof(receiver_addr.sin6_addr), 0) == -1) {
        perror("[-] send ipv6 address");
        return ERROR;
    }
    //send to client to try to connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);

    /*********************************/
    /*          recv size             */
    /*********************************/
    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    /*********************************/
    /*          recv data            */
    /*********************************/
    //to measure time
    long time;
    struct timeval start, end;

    //recv data
    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    if (!isQuiet)printf("[+] server ipv6 UDP receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recvfrom(receiver_socket, data_buffer, DATA_BUFFER_SIZE, 0,
                                           (struct sockaddr *) &client_addr, &addr_size);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("[-] server ipv6 udp receiving.\n");
                break;
            }

        }

    }
    gettimeofday(&end, NULL);
    /*********************************/
    /*     calc and print time       */
    /*********************************/
    time = (end.tv_sec - start.tv_sec) * 1000;
    time += (end.tv_usec - start.tv_usec) / 1000;
    printf("ipv6_udp,%ld\n", time);
    return 0;

}

int client_ipv6_UDP(int chat_socket, int port, int isQuiet) {
    if (!isQuiet)printf("------------client ipv6 udp------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] client ipv6 udp socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] client ipv6 udp socket.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] client ipv6 udp setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in6 addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    //get address
    if (recv(chat_socket, &addr.sin6_addr, sizeof(addr.sin6_addr), 0) == -1) {
        perror("[-] recv ipv6 address");
        return ERROR;
    }

    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;

    /*********************************/
    /*          send size             */
    /*********************************/
    char *data = generateData(isQuiet);
    //sending file size
    unsigned long data_size = strlen(data);
    if (!isQuiet) printf("Sending file size : %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &data_size, sizeof(unsigned long), 0);

    /*********************************/
    /*          send data            */
    /*********************************/
    //track how much send, for moving pointer;
    int data_index = 0;
    //send data in chunks
    while (data_size > 0) {
        //not send more
        int chunk_size = DATA_BUFFER_SIZE;
        if (data_size < chunk_size)chunk_size = data_size;

        ssize_t data_sended = sendto(sock, data + data_index, chunk_size, 0, (struct sockaddr *) &addr, sizeof(addr));
        if (data_sended < 0) {
            perror("[-] client ipv6 udp send");
            return ERROR;
        }
        data_index += data_sended;
        data_size -= data_sended;
    }

    free(data);

    return 0;
}

int server_uds_dgram(int chat_socket, int isQuiet) {
    if (!isQuiet)printf("------------server uds dgram------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    struct sockaddr_un addr;
    struct sockaddr_un client_addr;
    socklen_t client_len;

    int receiver_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (receiver_socket < 0) {
        perror("[-] uds dgram socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server uds dgram socket.\n");

    // Delete any file that already exists at the address. Make sure the deletion
    // succeeds.  If the error is just that the file/directory doesn't exist, it's fine.
    if (remove(UDS_FILE_NAME) == -1 && errno != ENOENT) {
        perror("[-] server uds dgram remove file");
        return ERROR;
    }

    // Zero out the address, and set family and path.
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UDS_FILE_NAME, sizeof(addr.sun_path) - 1);

    //make time out, stop if don't recv more
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(receiver_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("[-] server uds dgram SO_RCVTIMEO Error");
    }


    /*********************************/
    /*          bind()             */
    /*********************************/
    if (bind(receiver_socket, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("uds dgram bind");
    }
    if (!isQuiet)printf("[+] server uds dgram bind.\n");



    //send to client to try to connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);


    /*********************************/
    /*          recv size             */
    /*********************************/
    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    /*********************************/
    /*          recv data            */
    /*********************************/

    //to measure time
    long time;
    struct timeval start, end;

    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    if (!isQuiet)printf("[+] server uds dgram receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recvfrom(receiver_socket, data_buffer, DATA_BUFFER_SIZE, 0,
                                           (struct sockaddr *) &client_addr, &client_len);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("[-] server uds dgram receiving.\n");
                break;
            }

        }

    }
    gettimeofday(&end, NULL);
    /*********************************/
    /*     calc and print time       */
    /*********************************/
    time = (end.tv_sec - start.tv_sec) * 1000;
    time += (end.tv_usec - start.tv_usec) / 1000;
    printf("uds_dgram,%ld\n", time);

    //remove the file that created
    if (remove(UDS_FILE_NAME) < 0) {
        if (!isQuiet) perror("[-] Failed delete file at the end.");
    }

    return 0;


}

int client_uds_dgram(int chat_socket, int isQuiet) {
    if (!isQuiet)printf("------------client uds dgram------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    struct sockaddr_un addr;
    ssize_t numRead;

    // Create a new client socket with domain: AF_UNIX, type: SOCK_STREAM, protocol: 0
    int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    // Make sure socket's file descriptor is legit.
    if (sock < 0) {
        perror("[-] client uds dgram socket");
    }
    if (!isQuiet)printf("[+] client uds dgram socket.\n");

    // Construct server address, and make the connection.
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UDS_FILE_NAME, sizeof(addr.sun_path) - 1);

    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;

    /*********************************/
    /*          connect()             */
    /*********************************/
    if (connect(sock, (struct sockaddr *) &addr,
                sizeof(struct sockaddr_un)) < 0) {
        perror("[-] client uds dgram connect");
    }
    if (!isQuiet)printf("[+] client uds dgram connect.\n");

    /*********************************/
    /*          send size             */
    /*********************************/
    char *data = generateData(isQuiet);
    //sending file size
    unsigned long data_size = strlen(data);
    if (!isQuiet)printf("Sending file size : %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &data_size, sizeof(unsigned long), 0);

    /*********************************/
    /*          send data             */
    /*********************************/
//track how much send, for moving pointer;
    int data_index = 0;
    //send data in chunks
    while (data_size > 0) {
        //not send more
        int chunk_size = DATA_BUFFER_SIZE;
        if (data_size < chunk_size)chunk_size = data_size;

        ssize_t data_send = sendto(sock, data + data_index, chunk_size, 0, (struct sockaddr *) &addr, sizeof(addr));
        if (data_send < 0) {
            perror("[-] client uds dgram send");
            return ERROR;
        }
        data_index += data_send;
        data_size -= data_send;
    }
    free(data);

    return 0;
}

int server_uds_stream(int chat_socket, int isQuiet) {
    if (!isQuiet)printf("------------server uds stream------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    struct sockaddr_un addr;

    int receiver_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (receiver_socket < 0) {
        perror("[-] server uds stream socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server uds stream socket.\n");

//    // Make sure the address we're planning to use isn't too long.
//    if (strlen(UDS_FILE_NAME) > sizeof(addr.sun_path) - 1) {
//        perror("[-] uds stream Server socket path too long");
//        return ERROR;
//    }

    // Delete any file that already exists at the address. Make sure the deletion
    // succeeds. If the error is just that the file/directory doesn't exist, it's fine.
    if (remove(UDS_FILE_NAME) == -1 && errno != ENOENT) {
        perror("[-] server uds stream remove file");
        return ERROR;
    }

    // Zero out the address, and set family and path.
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UDS_FILE_NAME, sizeof(addr.sun_path) - 1);

    /*********************************/
    /*          bind()             */
    /*********************************/
    if (bind(receiver_socket, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) < 0) {
        perror("[-] server uds stream bind");
    }
    if (!isQuiet)printf("[+] server uds stream bind.\n");

    /*********************************/
    /*          listen()             */
    /*********************************/
    if (listen(receiver_socket, 1) < 0) {
        perror("[-] server uds stream listen");
    }


    //send to client to try to connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);

    /*********************************/
    /*          accept()             */
    /*********************************/
    int connection_socket = accept(receiver_socket, NULL, NULL);


    /*********************************/
    /*          recv size             */
    /*********************************/
    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    /*********************************/
    /*          recv data            */
    /*********************************/
    //to measure time
    long time;
    struct timeval start, end;

    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    if (!isQuiet)printf("[+] server uds stream receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recv(connection_socket, data_buffer, DATA_BUFFER_SIZE, 0);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            perror("[-] server uds stream receiving");
            break;
        }

    }
    gettimeofday(&end, NULL);
    /*********************************/
    /*     calc and print time       */
    /*********************************/
    time = (end.tv_sec - start.tv_sec) * 1000;
    time += (end.tv_usec - start.tv_usec) / 1000;
    printf("uds_stream,%ld\n", time);


    //remove the file that created
    if (remove(UDS_FILE_NAME) < 0) {
        if (!isQuiet)perror("[-] Failed delete file at the end.");
    }
    return 0;


}

int client_uds_stream(int chat_socket, int isQuiet) {
    if (!isQuiet)printf("------------client uds stream------------\n");

    /*********************************/
    /*          socket()             */
    /*********************************/
    struct sockaddr_un addr;
    ssize_t numRead;

    // Create a new client socket with domain: AF_UNIX, type: SOCK_STREAM, protocol: 0
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    // Make sure socket's file descriptor is legit.
    if (sock < 0) {
        perror("[-] client uds stream socket");
    }
    if (!isQuiet)printf("[+] client uds stream socket.\n");

    // Construct server address, and make the connection.
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UDS_FILE_NAME, sizeof(addr.sun_path) - 1);

    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;

    /*********************************/
    /*          connect()             */
    /*********************************/
    if (connect(sock, (struct sockaddr *) &addr,
                sizeof(struct sockaddr_un)) < 0) {
        perror("[-] client uds stream connect");
    }
    if (!isQuiet)printf("[+] client uds stream connect.\n");

    /*********************************/
    /*          send size             */
    /*********************************/
    char *data = generateData(isQuiet);
    //sending file size
    unsigned long un = strlen(data);
    if (!isQuiet) printf("Sending file size : %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &un, sizeof(unsigned long), 0);

    /*********************************/
    /*          send data             */
    /*********************************/
    send(sock, data, strlen(data), 0);
    free(data);

    return 0;
}

int server_pipe(int chat_socket, int isQuiet, char *param) {
    if (!isQuiet)printf("------------server pipe------------\n");

    int fd;
    ssize_t num_read, num_written;
    off_t total_read = 0;


    /*********************************/
    /*         recv size             */
    /*********************************/
    //get file size from sender


    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);
    /*********************************/
    /*         open pipe             */
    /*********************************/
    // Open the named pipe for reading
    fd = open(param, O_CREAT | O_RDONLY);
    if (fd == -1) {
        perror("open");
        return ERROR;

    }


    char data_buffer[DATA_BUFFER_SIZE];
    long time;
    struct timeval start, end;

    gettimeofday(&start, NULL);
    if (!isQuiet)printf("receiving data...\n");
    gettimeofday(&end, NULL);

    /*********************************/
    /*         recv data             */
    /*********************************/
    while ((num_read = read(fd, data_buffer, DATA_BUFFER_SIZE)) > 0) {
        total_read += num_read;
    }
    gettimeofday(&end, NULL);
    time = (end.tv_sec - start.tv_sec) * 1000;
    time += (end.tv_usec - start.tv_usec) / 1000;
    printf("pipe,%ld\n", time);

    if (!isQuiet)printf("Received %ld bytes from pipe\n", total_read);
    close(fd);

    //remove the file that created
    if (remove(param) < 0) {
        if (!isQuiet)perror("[-] Failed delete file at the end.");
    }
    return 0;
}

int client_pipe(int chat_socket, int isQuiet, char *param) {
    if (!isQuiet)printf("------------client pipe------------\n");


    int fd;
    char mesg[CHAT_BUFFER_SIZE];
    char buffer[CHAT_BUFFER_SIZE];
    if (!isQuiet)printf("[+]client pipe.\n");


    //Checks if a file exists and if not creates such a file
    ssize_t num_written;
    unsigned long num_read;
    if (access(param, F_OK) == -1) {
        if (mkfifo(param, 0666) == -1) {
            perror("mkfifo");
            return ERROR;
        }
    }
    if (!isQuiet)printf("[+] pipe file created.\n");

    /*********************************/
    /*         send size             */
    /*********************************/

    char *data = generateData(isQuiet);
    //sending file size
    unsigned long data_size = strlen(data);
    if (isQuiet)printf("Sending file size : %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &data_size, sizeof(unsigned long), 0);



    /*********************************/
    /*         open pipe file        */
    /*********************************/
    // Open the file to be sent
    fd = open(param, O_CREAT | O_WRONLY);
    if (fd == -1) {
        perror("open");
        return ERROR;
    }
    if (!isQuiet)printf("[+]pipe file open.\n");

    /*********************************/
    /*         write to pipe         */
    /*********************************/
    num_written = write(fd, data, data_size);

    if (!isQuiet)printf("Sent %ld bytes to pipe\n", num_written);

    close(fd);
    free(data);

    return 0;
}

int server_mmap(int chat_socket, int isQuiet, char *param) {
    if (!isQuiet)printf("------------server mmap------------\n");

    int fd;
    char *addr;
    char buffer[CHAT_BUFFER_SIZE];

    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;

    // Open the shared memory object
    fd = shm_open(param, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        return ERROR;
    }


    // Map the shared memory object into the address space of this process
    addr = mmap(NULL, DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        return ERROR;
    }

    // Wait for a client to connect and read from the shared memory
    if (!isQuiet)printf("Waiting for a client to connect...\n");
    /*********************************/
    /*          recv size             */
    /*********************************/
    //get file size from sender
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    /*********************************/
    /*          recv data            */
    /*********************************/
    //to measure time
    long time;
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Read from the shared memory
//    printf("Message received from client: %s\n", addr);

    char *data = malloc(DATA_SIZE);
    bzero(data, DATA_SIZE);
    memcpy(data, addr, DATA_SIZE);

    gettimeofday(&end, NULL);
    /*********************************/
    /*     calc and print time       */
    /*********************************/
    time = (end.tv_sec - start.tv_sec) * 1000;
    time += (end.tv_usec - start.tv_usec) / 1000;
    printf("mmap,%ld\n", time);
    if (!isQuiet)printf("Received byte: %ld\n", strlen(data));

    // Unmap the shared memory and close the shared memory object
    if (munmap(addr, DATA_BUFFER_SIZE) == -1) {
        perror("munmap");
        return ERROR;
    }
    if (close(fd) == -1) {
        perror("close");
        return ERROR;
    }

    // Unlink the shared memory object
    if (shm_unlink(param) == -1) {
        perror("shm_unlink");
        return ERROR;
    }

    //free data malloc
    free(data);

    return 0;


}

int client_mmap(int chat_socket, int isQuiet, char *param) {
    if (!isQuiet)printf("------------client mmap------------\n");

    int fd;
    char *addr;

    // Open the shared memory object
    fd = shm_open(param, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        return ERROR;
    }
    // Resize the shared memory object to the desired size
    if (ftruncate(fd, DATA_SIZE) == -1) {
        perror("ftruncate");
        return ERROR;
    }

    // Map the shared memory object into the address space of this process
    addr = mmap(NULL, DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        return ERROR;
    }

    //send to client to try to connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);
    if (close(fd) == -1) {
        perror("close");
        return ERROR;
    }
    // Write data to the shared memory
    char *data = generateData(isQuiet);

    strncpy(addr, data, DATA_SIZE);

    // Wait for the server to read from the shared memory
    printf("Waiting for the server to receive the message...\n");

    /*********************************/
    /*          send size             */
    /*********************************/
    unsigned long un = strlen(data);
    printf("Sending file size : %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &un, sizeof(unsigned long), 0);


    return 0;
}

int server_chat(char *port, int isPerformance, int isQuiet) {
    //if error change this value
    int returned_value = !ERROR;
    /*********************************/
    /*          socket()             */
    /*********************************/
    int receiver_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (receiver_socket < 0) {
        perror("[-] server chat socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server chat socket.\n");

    struct sockaddr_in receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(atoi(port));
    socklen_t addr_size = sizeof(client_addr);

    //not get address in use
    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
        perror("[-] server chat setsockopt(SO_REUSEADDR) failed");

    /*********************************/
    /*          bind()             */
    /*********************************/
    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("[-] server chat bind");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server chat bind.\n");

    /*********************************/
    /*          listen()             */
    /*********************************/
    if (listen(receiver_socket, 1) < 0) {
        perror("[-] server chat listen");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server chat listening.\n");

    /*********************************/
    /*          accept()             */
    /*********************************/

    int connection_socket = accept(receiver_socket, (struct sockaddr *) &client_addr, &addr_size);
    if (receiver_socket < 0) {
        perror("[-] server chat accept");
        return ERROR;
    }
    if (!isQuiet)printf("[+] server chat accept.\n");

    char text[CHAT_BUFFER_SIZE];
    bzero(text, CHAT_BUFFER_SIZE);
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);

    if (isPerformance) {

        recv(connection_socket, buffer, CHAT_BUFFER_SIZE, 0);
        char *type = strtok(buffer, ",");
        char *param = strtok(NULL, ",");


        if (strcmp(type, "ipv4") == 0) {
            if (strcmp(param, "udp") == 0) {
                returned_value = server_ipv4_UDP(connection_socket, atoi(port) + 1, isQuiet);
            } else if (strcmp(param, "tcp") == 0) {
                returned_value = server_ipv4_TCP(connection_socket, atoi(port) + 1, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "ipv6") == 0) {
            if (strcmp(param, "udp") == 0) {
                returned_value = server_ipv6_UDP(connection_socket, atoi(port) + 1, isQuiet);
            } else if (strcmp(param, "tcp") == 0) {
                returned_value = server_ipv6_TCP(connection_socket, atoi(port) + 1, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "uds") == 0) {
            if (strcmp(param, "dgram") == 0) {
                returned_value = server_uds_dgram(connection_socket, isQuiet);

            } else if (strcmp(param, "stream") == 0) {
                returned_value = server_uds_stream(connection_socket, isQuiet);

            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "mmap") == 0) {
            returned_value = server_mmap(connection_socket, isQuiet, param);

        } else if (strcmp(type, "pipe") == 0) {

            returned_value = server_pipe(connection_socket, isQuiet, param);
        } else {
            printf("usage: stnc -c IP PORT -p <type> <param>\n");
            return ERROR;
        }

    } else {//simple chat
        if (!isQuiet)printf("------------server chat------------\n");

        struct pollfd pfsockin;
        pfsockin.fd = connection_socket;
        pfsockin.events = POLLIN;

        struct pollfd pfstdin;
        pfstdin.fd = STDIN_FILENO;
        pfstdin.events = POLLIN;

        while (TRUE) {
            if (poll(&pfsockin, 1, TIME_OUT) == 1) {
                recv(connection_socket, buffer, CHAT_BUFFER_SIZE, 0);
                printf("client: %s", buffer);
                if (strncmp(buffer, "exit", 4) == 0) {
                    break;
                }

                bzero(buffer, CHAT_BUFFER_SIZE);
            }
            if (poll(&pfstdin, 1, TIME_OUT) == 1) {
                fgets(text, CHAT_BUFFER_SIZE, stdin);
                send(connection_socket, text, strlen(text), 0);
                if (strncmp(text, "exit", 4) == 0) {
                    break;
                }
                bzero(text, CHAT_BUFFER_SIZE);

            }
        }
    }
    close(connection_socket);
    return returned_value;
}

int client_chat(char *ip, char *port, int isPerformance, int isQuiet, char *type, char *param) {
    //if error change this value

    int returned_value = !ERROR;

    /*********************************/
    /*          socket()             */
    /*********************************/
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-] client chat socket");
        return ERROR;
    }
    if (!isQuiet)printf("[+] client chat socket.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] client chat setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);

    /*********************************/
    /*          connect()             */
    /*********************************/
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("[-] client chat connect.");
        return ERROR;
    }
    if (!isQuiet)printf("[+] client chat connect.\n");


    if (isPerformance) {

        char task[20] = {0};
        strcat(task, type);
        strcat(task, ",");
        strcat(task, param);
        send(sock, task, strlen(task), 0);

        if (strcmp(type, "ipv4") == 0) {
            if (strcmp(param, "udp") == 0) {
                returned_value = client_ipv4_UDP(sock, ip, atoi(port) + 1, isQuiet);
            } else if (strcmp(param, "tcp") == 0) {
                returned_value = client_ipv4_TCP(sock, ip, atoi(port) + 1, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "ipv6") == 0) {
            if (strcmp(param, "udp") == 0) {
                returned_value = client_ipv6_UDP(sock, atoi(port) + 1, isQuiet);
            } else if (strcmp(param, "tcp") == 0) {
                returned_value = client_ipv6_TCP(sock, atoi(port) + 1, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "uds") == 0) {
            if (strcmp(param, "dgram") == 0) {
                returned_value = client_uds_dgram(sock, isQuiet);
            } else if (strcmp(param, "stream") == 0) {
                returned_value = client_uds_stream(sock, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "mmap") == 0) {
            returned_value = client_mmap(sock, isQuiet, param);
        } else if (strcmp(type, "pipe") == 0) {
            returned_value = client_pipe(sock, isQuiet, param);
        } else {
            printf("usage: stnc -c IP PORT -p <type> <param>\n");
            return ERROR;
        }
    } else {//simple chat
        if (!isQuiet)printf("------------client chat------------\n");

        char text[CHAT_BUFFER_SIZE];
        char buffer[CHAT_BUFFER_SIZE];

        struct pollfd pfsockin;
        pfsockin.fd = sock;
        pfsockin.events = POLLIN;

        struct pollfd pfstdin;
        pfstdin.fd = STDIN_FILENO;
        pfstdin.events = POLLIN;


        while (TRUE) {
            if (poll(&pfsockin, 1, TIME_OUT) == 1) {
                bzero(buffer, CHAT_BUFFER_SIZE);
                recv(sock, buffer, CHAT_BUFFER_SIZE, 0);
                printf("server: %s", buffer);
                if (strncmp(buffer, "exit", 4) == 0) {
                    break;
                }
                bzero(buffer, CHAT_BUFFER_SIZE);
            }
            if (poll(&pfstdin, 1, TIME_OUT) == 1) {
                bzero(text, CHAT_BUFFER_SIZE);
                fgets(text, CHAT_BUFFER_SIZE, stdin);
                send(sock, text, CHAT_BUFFER_SIZE, 0);
                if (strncmp(text, "exit", 4) == 0) {
                    break;
                }
                bzero(text, CHAT_BUFFER_SIZE);
            }
        }
    }
    close(sock);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: ./stnc -c\\-s\n");
        return ERROR;
    }
    int isPerformance = FALSE;
    int isQuiet = FALSE;
    int isClient = FALSE;
    char *port;
    char *ip;
    char *type = "null";
    char *param = "null";

    for (int i = 1; i < 10; ++i) {

        if (argv[i] == NULL) break;
        if (strcmp(argv[i], "-c") == 0) {

            isClient = TRUE;
            ip = argv[i + 1];
            port = argv[i + 2];

            i += 2;
        } else if (strcmp(argv[i], "-s") == 0) {
            isClient = FALSE;
            port = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-p") == 0) {
            isPerformance = TRUE;
            if (isClient) {
                type = argv[i + 1];
                param = argv[i + 2];
                i += 2;
            }
        }else if (strcmp(argv[i], "-q") == 0) isQuiet = TRUE;
        else if (strcmp(argv[i], "pipe") == 0) {
            type = argv[i];
            param = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "mmap") == 0) {
            type = argv[i];
            param = argv[i + 1];
            i++;
        } else {
            printf("usage: ./stnc -c\\-s\n");
            return ERROR;
        }
    }

    if (isClient) {

        if (!isQuiet) {
            printf("[!] Ip: %s, Port %s, Performance mode: %s, Type: %s, Param: %s.\n", ip, port,
                   (isPerformance == TRUE) ? "True" : "False", type, param);
        }

        return client_chat(ip, port, isPerformance, isQuiet, type, param);

    } else {
        if (!isQuiet)printf("[!] Port %s, Performance mode: %s.\n", port, (isPerformance) ? "True" : "False");
        return server_chat(port, isPerformance, isQuiet);
    }
    return 0;

}
