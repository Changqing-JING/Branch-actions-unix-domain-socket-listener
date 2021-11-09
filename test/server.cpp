

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

struct sockaddr_un *servaddr = NULL, *clientaddr = NULL;
int sock_fd;
const char *SOCK_PATH="/tmp/unix_sock.sock";


int main () {
        char buffer[50] = {0};
        const char *msg = "Hello from server";
        socklen_t length = sizeof(struct sockaddr_un);
        int error;

        /* Part 1 – create the socket */
        if (0 > (sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0))) {
                printf("unable to create socket\n");
                exit(0);
        }
        char buffer_send[50] = {0};
        memset(buffer_send, 0, sizeof(buffer));
        strcpy(buffer_send, msg);

        /* Part 2 bind to the socket path */
        remove(SOCK_PATH);
        struct sockaddr_un servaddr;

        servaddr.sun_family = AF_UNIX;
        strcpy(servaddr.sun_path, SOCK_PATH);

        if (0 != (bind(sock_fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_un)))) {
                printf("bind failed\n");
                goto end;
        }

        /* Part 3 – send and receive data to the client */
        struct sockaddr_un clientaddr;

        int num_of_bytes;
        while (1) {
                memset(buffer, 0, sizeof(buffer));
                num_of_bytes = recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr, &length);
                if (num_of_bytes == -1) {
                        printf("read from client failed\n");
                        goto end;
        }
        printf("%s\n", buffer);

        
        if((sendto(sock_fd, buffer_send, sizeof(buffer_send), 0, (struct sockaddr *)&clientaddr, length)) != sizeof(buffer)) {
                printf("errno is %d", errno);
                printf("send failed\n");
                goto end;
                }
        }

end:
        close(sock_fd);
        return 0;

}
