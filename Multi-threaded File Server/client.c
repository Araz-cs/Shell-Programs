
//Araz Sultanian, 88077118

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 100

#define PORT 30000 // Can use between 30000 - 60000

int open_clientfd(char *hostname, char *port)
{
    int clientfd;
    struct addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    getaddrinfo(hostname, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next)
    {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue;
        }
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
        {
            break;
        }
        close(clientfd);
    }

    freeaddrinfo(listp);
    if (!p)
    {
        return -1;
    }
    else
    {
        return clientfd;
    }
}
int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];

    host = argv[1];
    port = argv[2];

    clientfd = open_clientfd(host, port);

    do
    {
        printf("> ");
        fgets(buf, MAXLINE, stdin);
        if (strcmp(buf, "quit\n") == 0)
        {
            break;
        }
        else
        {
            write(clientfd, buf, sizeof(buf));
            bzero(buf, sizeof(buf));
            read(clientfd, buf, MAXLINE);
            if (strcmp(buf, "") != 0)
            {
                printf("%s\n", buf);
            }
                
            
        }
    } while (strcmp(buf, "quit\n") != 0);

    close(clientfd);
    exit(0);
    return 0;
}