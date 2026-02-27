// tutorial referenced: https://www.youtube.com/watch?v=io2G2yW1Qk8
/*
Author: Rand Ismaael
Class: ECE4122
Last Date Modified: 11/14/2025
Description:
TCP server that continuously accepts connections from clients and logs messages.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

/*
 * Purpose: Check command line args and print error message for invalid ones.
 * Input: arg - the invalid argument string.
 */
void printInvalid(const char *arg)
{
    printf("Invalid command line argument detected:  %s\n", arg);
    printf("Please check your values and press any key to end the program!\n");
    getchar();
}

/*
 * Purpose: check port is a number (btwn 61000-65535)
 * Input: str - the port number string.
 * Output: portOut - pointer to store valid port number
 */
int isValidPort(const char *str, int *portOut)
{
    if (str == NULL || *str == '\0')
    {
        return 0;
    }
    for (int i = 0; str[i] != '\0'; i++)
    {
        // check each character is a digit
        if (!isdigit((unsigned char)str[i]))
        {
            return 0;
        }
    }

    // convert string to long
    long input = strtol(str, NULL, 10);
    // make sure it is between 61000, 65535
    if (input < 61000 || input > 65535)
    {
        return 0;
    }
    *portOut = (int)input;
    return 1;
}

/*
 * Purpose: Main function to run the TCP server.
 * Input: argc - argument count.
 *        argv - argument vector (port number).
 * Output: 0 on success, 1 on failure.
 */
int main(int argc, char *argv[])
{
    // command line argument check
    if (argc != 2)
    {
        if (argc > 1)
        {
            const char *arg = argv[1];
            printInvalid(arg);
            return 1;
        }
        else
        {
            const char *arg = "<missing>";
            printInvalid(arg);
            return 1;
        }
    }

    int port;
    if (!isValidPort(argv[1], &port))
    {
        printInvalid(argv[1]);
        return 1;
    }

    int serverSock, clientSock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    char buffer[1024]; // storage for data
    int n;

    FILE *log = fopen("server.log", "a");
    if (log == NULL)
    {
        perror("[-]Failed to open server.log");
        return 1;
    }

    // create socket
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0)
    {
        perror("[-]Socket creation failed");
        fclose(log);
        exit(1);
    }
    printf("[+]TCP Server socket created successfully.\n");
    // Failed to connect to the server at <IP address entered> on <port num>.
    // Please check your values and press any key to end program!

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;         // Internet version 4 (IPv4)
    server_addr.sin_port = htons(port);       // port number
    server_addr.sin_addr.s_addr = INADDR_ANY; // IP address

    // bind socket to the address and port number
    n = bind(serverSock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (n < 0)
    {
        perror("[-]Bind error");
        fclose(log);
        close(serverSock);
        exit(1);
    }
    printf("[+]Bind to the port number: %d\n", port);

    // listen for connections
    if (listen(serverSock, 10) < 0)
    {
        perror("[-]Could not listen");
        fclose(log);
        close(serverSock);
        exit(1);
    }
    printf("Listening...\n"); // waiting for client

    fd_set masterSet;
    fd_set readSet;
    FD_ZERO(&masterSet);
    FD_SET(serverSock, &masterSet);
    int maxFd = serverSock;

    // continuously accept connections from clients
    while (1)
    {
        readSet = masterSet;
        int ready = select(maxFd + 1, &readSet, NULL, NULL, NULL);
        if (ready < 0)
        {
            perror("[-]Select error");
            break;
        }

        // check for new connection
        if (FD_ISSET(serverSock, &readSet))
        {
            addr_size = sizeof(client_addr);
            clientSock = accept(serverSock, (struct sockaddr *)&client_addr, &addr_size);
            if (clientSock >= 0)
            {
                FD_SET(clientSock, &masterSet);
                if (clientSock > maxFd)
                {
                    maxFd = clientSock;
                }
                printf("[+]Client connected. \n");
                fprintf(log, "Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                fflush(log);
            }
            ready--;
            if (ready <= 0)
            {
                continue;
            }
        }

        // check for data from clients
        for (int fd = 0; fd <= maxFd; fd++)
        {
            if (fd == serverSock)
            {
                continue;
            }
            if (!FD_ISSET(fd, &readSet))
            {
                continue;
            }

            // receive message from client
            bzero(buffer, sizeof(buffer));
            n = recv(fd, buffer, sizeof(buffer), 0);

            if (n > 0)
            {
                buffer[n] = '\0';
                printf("Message from client(%d): %s\n", fd, buffer);
                fprintf(log, "%s\n", buffer);
                fflush(log);
            }
            else
            {
                // client disconnected
                printf("[+]Client (%d) disconnected.\n\n", fd);
                fprintf(log, "Client (%d) disconnected.\n\n", fd);
                fflush(log);
                close(fd);
                FD_CLR(fd, &masterSet);
            }
        }

        // // receive message from client
        // bzero(buffer, sizeof(buffer));
        // recv(client_sock, buffer, sizeof(buffer), 0);
        // printf("Message from client: %s\n", buffer);

        // bzero(buffer, sizeof(buffer));
        // strcpy(buffer, "Hello, world! This is server. CYRENE LIVES!");
        // printf("server: %s\n", buffer);
        // send(client_sock, buffer, strlen(buffer), 0); // send message to client

        // // close the socket
        // close(client_sock);
        // printf("[+]Client disconnected.\n\n");
    }
    fclose(log);
    close(serverSock);
    return 0;
}