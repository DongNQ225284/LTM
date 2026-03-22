#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
    struct addrinfo *res, *p;
    int ret = getaddrinfo(argv[1], "http", NULL, &res);
    if (ret != 0) {
        printf("getaddrinfo() Failed)");
        exit(1);
    }

    p = res;
    while (p != NULL) {
        if (p ->ai_family == AF_INET) {
            struct sockaddr_in addr;
            memcpy(&addr, p->ai_addr, p->ai_addrlen);
            printf("IPv4:\t %s\n", inet_ntoa(addr.sin_addr));
        } else if (p ->ai_family == AF_INET6) {
            struct sockaddr_in6 addr;
            char buf[256];
            memcpy(&addr, p->ai_addr, p->ai_addrlen);
            inet_ntop(AF_INET6, &addr.sin6_addr, buf, sizeof(buf));
            printf("IPv6:\t %s\n", buf);
        }
        p = p ->ai_next;
    }
    
    freeaddrinfo(res);
    return 0;
}