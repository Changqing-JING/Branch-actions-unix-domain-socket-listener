#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

struct sockaddr_un *servaddr = NULL, *clientaddr = NULL;
int sockfd;
const char *SOCKPATH = "/tmp/unix_client_sock.sock";
const char *SERVERPATH = "/tmp/unix_sock.sock";



int main() {
        char buffer[50] = {0};
        const char *msg = "Hello from client";
        int length, num_of_bytes;
        int error;

        char buffer_send[50] = {0};

        strcpy(buffer_send, msg);

        /* Part 1 – create the socket */
        if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
                printf("socket creation failed\n");
                exit(0);
        }

        /*part 2 – fill the client socket details and bind*/
        struct sockaddr_un clientaddr;

        remove(SOCKPATH);
        clientaddr.sun_family = AF_UNIX;
        strcpy(clientaddr.sun_path, SOCKPATH);

        if( 0 != (bind(sockfd, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr_un)))) {
                printf("Unable to bind at client end\n");
                goto end;
        }

        /* read and write data */
        struct sockaddr_un servaddr;
        

        servaddr.sun_family = AF_UNIX;
        strcpy(servaddr.sun_path, SERVERPATH);

        while (1) {
                
            num_of_bytes = sendto(sockfd, buffer_send, sizeof(buffer), 0, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_un));

            if(num_of_bytes < sizeof(buffer_send)) {
                    printf("unable to send data to server\n");
                    goto end;
            }

            memset(buffer, 0, sizeof(buffer));
            num_of_bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
            if (num_of_bytes == -1) {
                    printf("unable to receive data from the server\n");
                    goto end;
            }
            printf("%s\n", buffer);
            sleep(1);
        }
end:
        close(sockfd);
        return 0;
}