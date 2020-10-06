/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

void error(const char *msg)
{
     perror(msg);
     exit(1);
}

int main(int argc, char *argv[])
{
     printf("process:\n");

     int sockfd, newsockfd, portno;
     socklen_t clilen;
     float buffer;
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     printf("1\n");
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
          error("ERROR opening socket");

     bzero((char *)&serv_addr, sizeof(serv_addr));
     portno = atoi("15728"); // port number in the first position of argv
     printf("2\n");
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *)&serv_addr,
              sizeof(serv_addr)) < 0)
          error("ERROR on binding");
     printf("3\n");
     listen(sockfd, 5);
     clilen = sizeof(cli_addr);
     printf("4\n");
     newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
     if (newsockfd < 0)
          error("ERROR on accept");
     bzero(&buffer, sizeof(buffer));
     printf("5\n");
     while (1)
     {
          printf("Ready to recive a message");

          n = read(newsockfd, &buffer, sizeof(buffer)); //leggo dato socket
          if (n < 0)
               error("ERROR reading from socket");

          printf("Here is the message: %.3f\n", buffer);
     }

     close(newsockfd);
     close(sockfd);
     return 0;
}
