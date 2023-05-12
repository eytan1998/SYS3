
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/poll.h>
#include <time.h>
#include <sys/time.h>

#define ERROR 1
#define FALSE 0
#define TRUE 1

#define TIME_OUT 100
#define DATA_BUFFER_SIZE 60000

#define CHAT_BUFFER_SIZE 1024
#define PIPE_NAME "/tmp/my_pipe"
#define UDS_FILE_NAME "UDS-FILE"
#define UNIX_PATH_MAX 108
struct sockaddr_un {
    unsigned short int sun_family; /* AF_UNIX */
    char sun_path[UNIX_PATH_MAX]; /* pathname */
};
#define DATA_SIZE 104857600

unsigned int checksum(const char *data, size_t len)
{
    unsigned int sum = 0;
    size_t i;
    for (i = 0; i < len; ++i)
        sum += data[i];
    return sum;
}

char *generateData() {
    char *data = malloc(DATA_SIZE);
    for (int i = 0; i < DATA_SIZE; ++i) {
        data[i] = '$';
    }
    if (data == NULL) {
        perror("malloc data");
        return NULL;
    }
    printf("checksum: %u\n", checksum(data,DATA_SIZE));
    return data;
}

int server_ipv4_TCP(int chat_socket, int port, int isQuiet) {
    int receiver_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (receiver_socket == ERROR) {
        perror("[-] ipv4 tcp Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]server ipv4 socket created.\n");

    struct sockaddr_in receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);
    socklen_t addr_size = sizeof(client_addr);

    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
        perror("[-] setsockopt(SO_REUSEADDR) failed");

    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) == ERROR) {
        perror("[-]Bind error");
        exit(1);
    }
    if (!isQuiet)printf("[+]Bind to: %d\n", port);

    if (listen(receiver_socket, 1) == ERROR) {
        perror("[-]Listen error");
        exit(1);
    }
    if (!isQuiet)printf("Listening...\n");

    //send to client to try connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);

    int connection_socket = accept(receiver_socket, (struct sockaddr *) &client_addr, &addr_size);
    if (!isQuiet)printf("[+]client ipv4 connected.\n");

    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    //to measure time
    long time;
    struct timeval start, end;

    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    printf("receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recv(connection_socket, data_buffer, DATA_BUFFER_SIZE, 0);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            printf("[-]Error while receiving.\n");
            break;
        }

    }
    gettimeofday(&end, NULL);
    time = (end.tv_usec - start.tv_usec) / 1000;
    printf("ipv4_tcp,%ld\n", time);
    return 0;
}

int client_ipv4_TCP(int chat_socket, char *ip, int port, int isQuiet) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]client ipv4 socket created.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] setsockopt(SO_REUSEADDR) failed");
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


    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == ERROR) {
        perror("[-]Connection error ");
        exit(1);
    }
    if (!isQuiet)printf("[+]server ipv4 connected.\n");

    char *data = generateData();
    //sending file size
    unsigned long un = strlen(data);
    printf("Sending file size is: %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &un, sizeof(unsigned long), 0);

    //send data
    send(sock, data, strlen(data), 0);


    return 0;
}

int server_ipv6_TCP(int chat_socket, int port, int isQuiet) {
    int receiver_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (receiver_socket == ERROR) {
        perror("[-]Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]server ipv6 socket created.\n");

    struct sockaddr_in6 receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin6_family = AF_INET6;
    receiver_addr.sin6_port = htons(port);

    socklen_t addr_size = sizeof(client_addr);

    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
        perror("[-] setsockopt(SO_REUSEADDR) failed");

    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) == ERROR) {
        perror("[-]Bind error");
        exit(1);
    }
    if (!isQuiet)printf("[+]Bind to: %d\n", port);

    if (listen(receiver_socket, 1) == ERROR) {
        perror("[-]Listen error");
        exit(1);
    }
    if (!isQuiet)printf("Listening...\n");

    //send the ipv6 address to client
    if (send(chat_socket, &receiver_addr.sin6_addr, sizeof(receiver_addr.sin6_addr), 0) == -1) {
        perror("send");
        return 1;
    }
    //send to client to try connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);

    int connection_socket = accept(receiver_socket, (struct sockaddr *) &client_addr, &addr_size);
    if (!isQuiet)printf("[+]client ipv6 connected.\n");

    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    //to measure time
    long time;
    struct timeval start, end;

    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    printf("receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recv(connection_socket, data_buffer, DATA_BUFFER_SIZE, 0);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            printf("[-]Error while receiving.\n");
            break;
        }

    }
    gettimeofday(&end, NULL);
    time = (end.tv_usec - start.tv_usec) / 1000;
    printf("ipv6_tcp,%ld\n", time);
    return 0;
}

int client_ipv6_TCP(int chat_socket, int port, int isQuiet) {

    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]client ipv6 socket created.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in6 addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    //get address
    if (recv(chat_socket, &addr.sin6_addr, sizeof(addr.sin6_addr), 0) == -1) {
        perror("recv");
        return 1;
    }

    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;


    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == ERROR) {
        perror("[-]Connection error ");
        exit(1);
    }
    if (!isQuiet)printf("[+]server ipv6 connected.\n");

    char *data = generateData();
    //sending file size
    unsigned long un = strlen(data);
    printf("Sending file size is: %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &un, sizeof(unsigned long), 0);

    //send data
    send(sock, data, strlen(data), 0);
    return 0;
}

int server_ipv4_UDP(int chat_socket, int port, int isQuiet) {
    int receiver_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiver_socket == ERROR) {
        perror("[-]server ipv4 UDP Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]server ipv4 UDP socket created.\n");

    struct sockaddr_in receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);
    socklen_t addr_size = sizeof(client_addr);

    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] server ipv4 UDP setsockopt(SO_REUSEADDR) failed");
    }
    //make time out, stop if don't recv more
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(receiver_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("[-] server ipv4 UDP SO_RCVTIMEO Error");
    }
    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) == ERROR) {
        perror("[-]server ipv4 UDP Bind error");
        exit(1);
    }
    if (!isQuiet)printf("[+]server ipv4 UDP Bind to: %d\n", port);


    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    //to measure time
    long time;
    struct timeval start, end;

    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    printf("server ipv4 UDP receiving data...\n");
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
            perror("[-]server ipv4 UDP Error while receiving.\n");
            break;
        }

    }
    gettimeofday(&end, NULL);
    time = (end.tv_usec - start.tv_usec) / 1000;
    printf("ipv4_udp,%ld\n", time);
    return 0;
}

int client_ipv4_UDP(int chat_socket, char *ip, int port, int isQuiet) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] client ipv4 socket Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]client ipv4 socket created.\n");

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
    //TODO
//    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
//    if (strcmp(buffer, "ready") != 0) return ERROR;


    char *data = generateData();
    //sending file size
    unsigned long data_size = strlen(data);
    printf("Sending file size is: %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &data_size, sizeof(unsigned long), 0);

    //track how much send, for moving pointer;
    int data_index = 0;
    //send data in chunks
    while (data_size > 0) {
        //not send more
        int chunk_size = DATA_BUFFER_SIZE;
        if (data_size < chunk_size)chunk_size = data_size;

        ssize_t data_sended = sendto(sock, data + data_index, chunk_size, 0, (struct sockaddr *) &addr, sizeof(addr));
        if (data_sended < 0) {
            perror("[-]client ipv4 socket cant send");
            return ERROR;
        }
        data_index += data_sended;
        data_size -= data_sended;
    }


    return 0;
}

int server_ipv6_UDP(int chat_socket, int port, int isQuiet) {
    int receiver_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (receiver_socket == ERROR) {
        perror("[-]server ipv6 UDP Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]server ipv6 UDP socket created.\n");

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
    tv.tv_usec = 1;
    if (setsockopt(receiver_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("[-] server ipv6 UDP set_sockopt(SO_RCVTIMEO) failed");
    }
    //bind
    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) == ERROR) {
        perror("[-]server ipv6 UDP Bind error");
        exit(1);
    }
    if (!isQuiet)printf("[+]server ipv6 UDP Bind to: %d\n", port);

    //send the ipv6 address to client
    if (send(chat_socket, &receiver_addr.sin6_addr, sizeof(receiver_addr.sin6_addr), 0) == -1) {
        perror("[-] send ipv6 address");
        return 1;
    }
    //send to client to try to connect after create socket
    send(chat_socket, "ready", strlen("ready"), 0);

    //get file size from sender
    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    //to measure time
    long time;
    struct timeval start, end;

    //TODO pull, to avoid fill resource
    //recv data
    char data_buffer[DATA_BUFFER_SIZE];
    gettimeofday(&start, NULL);
    printf("server ipv6 UDP receiving data...\n");
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
            perror("[-]server ipv6 UDP Error while receiving.");
            break;
        }

    }
    gettimeofday(&end, NULL);
    time = (end.tv_usec - start.tv_usec) / 1000;
    printf("ipv4_udp,%ld\n", time);
    return 0;

}

int client_ipv6_UDP(int chat_socket, int port, int isQuiet) {
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[-] client ipv6 socket Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]client ipv6 socket created.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] client ipv6 socket setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in6 addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    //get address
    if (recv(chat_socket, &addr.sin6_addr, sizeof(addr.sin6_addr), 0) == -1) {
        perror("recv");
        return 1;
    }

    char buffer[CHAT_BUFFER_SIZE];
    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    if (strcmp(buffer, "ready") != 0) return ERROR;


    char *data = generateData();
    //sending file size
    unsigned long data_size = strlen(data);
    printf("Sending file size is: %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &data_size, sizeof(unsigned long), 0);

    //track how much send, for moving pointer;
    int data_index = 0;
    //send data in chunks
    while (data_size > 0) {
        //not send more
        int chunk_size = DATA_BUFFER_SIZE;
        if (data_size < chunk_size)chunk_size = data_size;

        ssize_t data_sended = sendto(sock, data + data_index, chunk_size, 0, (struct sockaddr *) &addr, sizeof(addr));
        if (data_sended < 0) {
            perror("[-]client ipv6 socket cant send");
            return ERROR;
        }
        data_index += data_sended;
        data_size -= data_sended;
    }


    return 0;
}

int server_uds_dgram(int chat_socket, int isQuiet) {
    /*********************************/
    /*          socket()             */
    /*********************************/
    struct sockaddr_un addr;

    int receiver_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (receiver_socket == -1) {
        perror("[-] uds dgram socket");
        return ERROR;
    }

//    // Make sure the address we're planning to use isn't too long.
//    if (strlen(UDS_FILE_NAME) > sizeof(addr.sun_path) - 1) {
//        perror("[-] uds dgram Server socket path too long");
//        return ERROR;
//    }

    // Delete any file that already exists at the address. Make sure the deletion
    // succeeds. If the error is just that the file/directory doesn't exist, it's fine.
    if (remove(UDS_FILE_NAME) == -1 && errno != ENOENT) {
        perror("[-] uds dgram remove file");
        return ERROR;
    }

    // Zero out the address, and set family and path.
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UDS_FILE_NAME, sizeof(addr.sun_path) - 1);

    /*********************************/
    /*          bind()             */
    /*********************************/
    if (bind(receiver_socket, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("uds dgram bind");
    }

    /*********************************/
    /*          listen()             */
    /*********************************/
    if (listen(receiver_socket, 1) == -1) {
        perror("uds dgram listen");
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
    if (!isQuiet)printf("receiving data...\n");
    while (1) {
        //zero the buffer
        bzero(data_buffer, DATA_BUFFER_SIZE);
        int bytesReceived = (int) recv(connection_socket, data_buffer, DATA_BUFFER_SIZE, 0);
        //to track how much byte to receive
        receiveSize -= bytesReceived;
        if (bytesReceived == 0 || receiveSize <= 0) break;
        //check error
        if (bytesReceived < 0) {
            perror("[-]Error while receiving");
            break;
        }

    }
    /*********************************/
    /*     calc and print time       */
    /*********************************/
    gettimeofday(&end, NULL);
    time = (end.tv_usec - start.tv_usec) / 1000;
    printf("uds_dgram,%ld\n", time);
    return 0;


}

int client_uds_dgram(int chat_socket, int isQuiet) {
    /*********************************/
    /*          socket()             */
    /*********************************/
    struct sockaddr_un addr;
    ssize_t numRead;

    // Create a new client socket with domain: AF_UNIX, type: SOCK_STREAM, protocol: 0
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    // Make sure socket's file descriptor is legit.
    if (sock == -1) {
        perror("socket");
    }

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
                sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
    }

    /*********************************/
    /*          send size             */
    /*********************************/
    char *data = generateData();
    //sending file size
    unsigned long un = strlen(data);
    printf("Sending file size is: %lu bytes, (~%lu MB)\n", strlen(data), strlen(data) / 1000000);
    send(chat_socket, &un, sizeof(unsigned long), 0);

    /*********************************/
    /*          send data             */
    /*********************************/
    send(sock, data, strlen(data), 0);

    return 0;
}

int server_uds_stream(int chat_socket, int isQuiet) {
    return 0;
}

int client_uds_stream(int chat_socket, int isQuiet) {
    return 0;
}


/*
int server_pipe(int chat_socket, int isPerformance, int isQuiet) {
    int fd;
    char buffer[CHAT_BUFFER_SIZE];
    char mesg[CHAT_BUFFER_SIZE];
    ssize_t num_read, num_written;
    off_t total_read = 0;


    // Open the named pipe for reading
    fd = open(PIPE_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    //send ready
    send(chat_socket, "ready", strlen("ready"), 0);



    //get file size from sender

    bzero(buffer, CHAT_BUFFER_SIZE);
    recv(chat_socket, buffer, CHAT_BUFFER_SIZE, 0);
    int receiveSize = (int) *(int *) buffer;
    bzero(buffer, CHAT_BUFFER_SIZE);
    if (!isQuiet)printf("Received file size is: %d bytes, (~%d MB)\n", receiveSize, receiveSize / 1000000);

    // Read data from the named pipe
    while ((num_read = read(fd, buffer, CHAT_BUFFER_SIZE)) > 0) {
        total_read += num_read;
    }

    printf("Received %ld bytes from pipe\n", total_read);
    close(fd);
    return 0;
}


int client_pipe(int chat_socket, int isQuiet, char *param) {
    int fd;
    char mesg[CHAT_BUFFER_SIZE];
    char *buffer = generateData();

    ssize_t num_read, num_written;
    off_t total_written = 0;
    // Create the named pipe
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    if (!isQuiet)printf("[+]pipe file created.\n");
    // Open the file to be sent
    FILE *fp = fopen("file.txt", "rb");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
// Open the named pipe for writing
    fd = open(PIPE_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    //recv ready!
    char ready[CHAT_BUFFER_SIZE];
    if (!isQuiet)printf("[+]pipe file open.\n");
    bzero(ready, CHAT_BUFFER_SIZE);
    recv(chat_socket, ready, CHAT_BUFFER_SIZE, 0);
    if (strcmp(ready, "ready") != 0) return ERROR;

// Read the buffer data and write it to the named pipe
    while ((num_read = fread(buffer, 1, CHAT_BUFFER_SIZE, buffer)) > 0) {
        num_written = write(fd, buffer, num_read);
        if (num_written == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        total_written += num_written;
    }

    printf("Sent %ld bytes to pipe\n", total_written);

    close(fd);

    return 0;
}
*/
int server_mmap() {}

int client_mmap() {}


int server_chat(char *port, int isPerformance, int isQuiet) {

    //1 establish connection
    int receiver_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (receiver_socket == ERROR) {
        perror("[-]Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+] Server chat socket created.\n");

    struct sockaddr_in receiver_addr, client_addr;
    memset(&receiver_addr, '\0', sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(atoi(port));
    socklen_t addr_size = sizeof(client_addr);

    if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
        perror("[-] setsockopt(SO_REUSEADDR) failed");

    if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) == ERROR) {
        perror("[-]Bind error");
        exit(1);
    }
    if (!isQuiet)printf("[+]Server chat Bind to: %s\n", port);

    if (listen(receiver_socket, 1) == ERROR) {
        perror("[-]Listen error");
        exit(1);
    }
    if (!isQuiet)printf("Server chat Listening...\n");

    int connection_socket = accept(receiver_socket, (struct sockaddr *) &client_addr, &addr_size);
    if (!isQuiet)printf("[+] Client chat connected.\n");

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
                server_ipv4_UDP(connection_socket, atoi(port) + 1, isQuiet);
            } else if (strcmp(param, "tcp") == 0) {
                server_ipv4_TCP(connection_socket, atoi(port) + 1, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "ipv6") == 0) {
            if (strcmp(param, "udp") == 0) {
                server_ipv6_UDP(connection_socket, atoi(port) + 1, isQuiet);
            } else if (strcmp(param, "tcp") == 0) {
                server_ipv6_TCP(connection_socket, atoi(port) + 1, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "uds") == 0) {
            if (strcmp(param, "dgram") == 0) {
                server_uds_dgram(connection_socket, isQuiet);

            } else if (strcmp(param, "stream") == 0) {
                server_uds_stream(connection_socket, isQuiet);

            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "mmap") == 0) {
        } else if (strcmp(type, "pipe") == 0) {

        } else {
            printf("usage: stnc -c IP PORT -p <type> <param>\n");
            return ERROR;
        }

    } else {//simple chat

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
    return 0;
}

int client_chat(char *ip, char *port, int isPerformance, int isQuiet, char *type, char *param) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    if (!isQuiet)printf("[+]Client chat socket created.\n");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("[-] setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == ERROR) {
        perror("[-]Connection error ");
        exit(1);
    }
    if (!isQuiet)printf("[+]Client chat socket connected.\n");


    if (isPerformance) {

        char task[20] = {0};
        strcat(task, type);
        strcat(task, ",");
        strcat(task, param);
        send(sock, task, strlen(task), 0);

        if (strcmp(type, "ipv4") == 0) {
            if (strcmp(param, "udp") == 0) {
                client_ipv4_UDP(sock, ip, atoi(port) + 1, isQuiet);
            } else if (strcmp(param, "tcp") == 0) {
                client_ipv4_TCP(sock, ip, atoi(port) + 1, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "ipv6") == 0) {
            if (strcmp(param, "udp") == 0) {
                client_ipv6_UDP(sock, atoi(port) + 1, isQuiet);
            } else if (strcmp(param, "tcp") == 0) {

                client_ipv6_TCP(sock, atoi(port) + 1, isQuiet);
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "uds") == 0) {
            if (strcmp(param, "dgram") == 0) {
                client_uds_dgram(sock, isQuiet);
            } else if (strcmp(param, "stream") == 0) {
                client_uds_stream(sock, isQuiet);

            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return ERROR;
            }
        } else if (strcmp(type, "mmap") == 0) {
        } else if (strcmp(type, "pipe") == 0) {
//            client_pipe(sock, isQuiet, param);
        } else {
            printf("usage: stnc -c IP PORT -p <type> <param>\n");
            return ERROR;
        }
    } else {//simple chat
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
        return 1;
    }
    int isPerformance = FALSE;
    int isQuiet = FALSE;
    int isClient = FALSE;
    char *port;
    char *ip;
    char *type;
    char *param;

    for (int i = 1; i < 7; ++i) {
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
        } else if (strcmp(argv[i], "-q") == 0) isQuiet = TRUE;
        else {
            printf("usage: ./stnc -c\\-s\n");
            return 1;
        }
    }
    if (isClient) client_chat(ip, port, isPerformance, isQuiet, type, param);
    else server_chat(port, isPerformance, isQuiet);

    return 0;

}
