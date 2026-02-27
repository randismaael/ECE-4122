// tutorial referenced: https://www.youtube.com/watch?v=io2G2yW1Qk8
/*
Author: Rand Ismaael
Class: ECE4122
Last Date Modified: 11/14/2025
Description:
TCP client that connects to a server and continuously sends messages.
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
 * Purpose: Print connection failure message.
 * Input: ip - the IP address string.
 * Output: portStr - the port number string.
 */
void printConnectFail(const char *ip, const char *portStr)
{
    printf("Failed to connect to the server at %s on %s.\n", ip, portStr);
    printf("Please check your values and press any key to end program!\n");
    getchar();
}

/*
 * Purpose: Main function to run the TCP client.
 * Input: argc - argument count.
 *        argv - argument vector (IP address and port number).
 * Output: 0 on success, 1 on failure.
 */
int main(int argc, char *argv[])
{
    // command line argument check
    if (argc != 3)
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

    const char *ip = argv[1];
    const char *portStr = argv[2];
    int port;
    if (!isValidPort(portStr, &port))
    {
        printInvalid(portStr);
        return 1;
    }

    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;

    char buffer[1024]; // storage for data
    int n;

    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printConnectFail(ip, portStr);
        exit(1);
    }
    printf("[+]TCP client socket created successfully.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;   // Internet version 4 (IPv4)
    addr.sin_port = htons(port); // port number

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
    {
        printConnectFail(ip, portStr);
        close(sock);
        return 1;
    }

    // connect to the server
    n = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (n < 0)
    {
        printConnectFail(ip, portStr);
        close(sock);
        exit(1);
    }

    printf("[+]Connected to the server at %s:%d\n", ip, port);

    // continue to send and receive messages
    while (1)
    {
        printf("Please enter a message: ");
        if (!fgets(buffer, sizeof(buffer), stdin))
        {
            break;
        }

        if (strcmp(buffer, "quit\n") == 0 || strcmp(buffer, "quit\r\n") == 0)
        {
            break;
        }

        size_t len = strlen(buffer);
        if (len == 0)
        {
            continue; // ignore empty input
        }

        // send message to server
        ssize_t sent = send(sock, buffer, len, 0);
        if (sent < 0)
        {
            printf("Error sending message to server.\n");
            break;
        }
    }

    close(sock);
    printf("[+]Disconnected from the server.\n");

    return 0;
}