

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

#define ERROR -1
#define FALSE 0
#define TRUE 1
#define BACKLOG 10   // how many pending connections queue will hold

#define TIME_OUT 100
#define BUFFER_SIZE 1024



int main(int argc, char *argv[]) {


    if (argc < 2) {
        printf("usage: ./stnc -c\\-s");
        return 1;
    }
    if (strcmp("-c", argv[1]) == 0) {

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("[-]Socket error");
            exit(1);
        }
        printf("[+]TCP socket created.\n");

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
            perror("[-] setsockopt(SO_REUSEADDR) failed");
        }

        struct sockaddr_in addr;
        memset(&addr, '\0', sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(atoi(argv[3]));
        addr.sin_addr.s_addr = inet_addr(argv[2]);

        if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == ERROR) {
            perror("[-]Connection error ");
            exit(1);
        }
        printf("[+]TCP socket connected.\n");


        struct pollfd pfsockin;
        pfsockin.fd = sock;
        pfsockin.events = POLLIN;

        struct pollfd pfstdin;
        pfstdin.fd = STDIN_FILENO;
        pfstdin.events = POLLIN;



        char *text[BUFFER_SIZE];
        int exit = 1;
        char buffer[BUFFER_SIZE];
        while (exit) {
            if (poll(&pfsockin, 1, TIME_OUT) == 1) {
                bzero(buffer, BUFFER_SIZE);
                recv(sock, buffer, BUFFER_SIZE, 0);
                printf("server: %s", buffer);
                if (strncmp(buffer, "exit", 4) == 0) {
                    break;
                }
                bzero(buffer, BUFFER_SIZE);
            }
            if (poll(&pfstdin, 1, TIME_OUT) == 1) {
                bzero(text, BUFFER_SIZE);
                fgets(text, BUFFER_SIZE, stdin);
                send(sock, text, BUFFER_SIZE, 0);
                if (strncmp(text, "exit", 4) == 0) {
                    break;
                }
                bzero(text, BUFFER_SIZE);
            }
        }
        close(sock);
    } else if (strcmp("-s", argv[1]) == 0) {
        int isPerform = (strcmp(argv[3], "-p") == 0 || strcmp(argv[4], "-p") == 0);
        int isQuiet = (strcmp(argv[3], "-q") == 0 || strcmp(argv[4], "-q") == 0);
        //1 establish connection
        int receiver_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (receiver_socket == ERROR) {
            perror("[-]Socket error");
            exit(1);
        }
        printf("[+]TCP Receiver socket created.\n");

        struct sockaddr_in receiver_addr, client_addr;
        memset(&receiver_addr, '\0', sizeof(receiver_addr));
        receiver_addr.sin_family = AF_INET;
        receiver_addr.sin_port = htons(atoi(argv[2]));
        socklen_t addr_size = sizeof(client_addr);

        if (setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
            perror("[-] setsockopt(SO_REUSEADDR) failed");

        if (bind(receiver_socket, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) == ERROR) {
            perror("[-]Bind error");
            exit(1);
        }
        printf("[+]Bind to: %s\n", argv[2]);

        if (listen(receiver_socket, 1) == ERROR) {
            perror("[-]Listen error");
            exit(1);
        }
        printf("Listening...\n");

        int connection_socket = accept(receiver_socket, (struct sockaddr *) &client_addr, &addr_size);
        printf("[+]Sender connected.\n");

        //
        struct pollfd pfsockin;
        pfsockin.fd = connection_socket;
        pfsockin.events = POLLIN;

        struct pollfd pfstdin;
        pfstdin.fd = STDIN_FILENO;
        pfstdin.events = POLLIN;
        char text[BUFFER_SIZE];
        bzero(text, BUFFER_SIZE);

        int exit = 1;

        char buffer[BUFFER_SIZE];
        while (exit) {
            if (poll(&pfsockin, 1, TIME_OUT) == 1) {
                recv(connection_socket, buffer, BUFFER_SIZE, 0);
                printf("client: %s", buffer);
                if (strncmp(buffer, "exit", 4) == 0) {
                    break;
                }

                bzero(buffer, BUFFER_SIZE);
            }
            if (poll(&pfstdin, 1, TIME_OUT) == 1) {
                fgets(text, BUFFER_SIZE, stdin);
                send(connection_socket, text, strlen(text), 0);
                if (strncmp(text, "exit", 4) == 0) {
                    break;
                }
                bzero(text, BUFFER_SIZE);

            }
        }
        close(connection_socket);
    } else {
        printf("usage: ./stnc -c\\-s");
        return 1;
    }
    return 0;

}


int func(int argc, char *argv[]) {
    {
        if (argc < 3 || argc > 6) {
            printf("usage: stnc -c IP PORT -p <type> <param>\n");
            return 1;
        }
        int isperform = FALSE;
    }

        int i = 0;
        if (argc >= 5) {
            if (strcmp(argv[4], "-P") == 0) {
                isperform=TRUE;
                i = 1;
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return 1;
            }
            if (is_ip(argv[2])) {
                if (is_port(argv[3])) {
                    if (strcmp(argv[4 + i], "ipv4") == 0) {
                        if (strcmp(argv[5 + i], "udp") == 0) {}
                        if (strcmp(argv[5 + i], "tcp") == 0) {}
                    }
                    if (strcmp(argv[4 + i], "ipv6") == 0) {
                        if (strcmp(argv[5 + i], "udp") == 0) {}
                        if (strcmp(argv[5 + i], "tcp") == 0) {}
                    }

                    if (strcmp(argv[4 + i], "uds") == 0) {
                        if (strcmp(argv[5 + i], "dgram") == 0) {}
                        if (strcmp(argv[5 + i], "stream") == 0) {}
                    }
                }
            }
        }
            FILE *fp1 = fopen(argv[4+i], "r");
            if (fp1 == NULL) {
            printf("target file not exist\n");
            return 1;
            }
            if (strcmp(argv[5+i], "mmap") == 0) {}
            if (strcmp(argv[5+i], "pipe") == 0) {}




}
        if (argc <= 4) {
            if (strcmp(argv[2], "-P") == 0) {
                isperform = TRUE;
                i = 1;
            } else {
                printf("usage: stnc -c IP PORT -p <type> <param>\n");
                return 1;
            }
        }