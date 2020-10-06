#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define max(a, b) \
	({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

float newToken;
char *timeString;
pid_t pid_S, pid_G, pid_L, pid_P;

FILE *fp; //Configuration file

char *signame[] = {"INVALID", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGPOLL", "SIGPWR", "SIGSYS", NULL};

const char *myfifo1 = "myfifo1"; //path
const char *myfifo2 = "myfifo2"; //path
const char *myfifo3 = "myfifo3"; //path

void error(const char *msg)
{
	perror(msg);
	exit(-1);
}

void readConfFile(char *ip, char *port_n, char *port_p, int *rf)
{
	fp = fopen("ConfigurationFile.txt", "r");

	if (fp == NULL)
		error("Failed to open ConfigurationFile");

	fscanf(fp, "%s", ip);
	fscanf(fp, "%s", port_n);
	fscanf(fp, "%s", port_p);
	fscanf(fp, "%d", rf);

	printf("IP : %s\n", ip);
	printf("Port_N : %s\n", port_n);
	printf("Port_P : %s\n", port_p);
	printf("RF : %d\n", *rf);

	fclose(fp);
}

void logFile(pid_t process_id, float message, float token)
{
	FILE *f;
	f = fopen("logFile.log", "a");
	time_t currentTime;
	currentTime = time(NULL);
	timeString = ctime(&currentTime);
	fprintf(f, "-%sPID: %d value:%.3f.\n", timeString, process_id, message);
	fprintf(f, "-%s%.3f.\n\n", timeString, token);

	fclose(f);
}

void sig_handler(int signo)
{
	if (signo == SIGUSR1) //Stop Process
	{
		printf("Received SIGUSR1\n");
		kill(pid_P, SIGSTOP);
		kill(pid_G, SIGSTOP);
		kill(pid_L, SIGSTOP);
	}
	else if (signo == SIGUSR2) //Resume Process
	{
		printf("Received SIGUSR2\n");
		kill(pid_P, signo);
		kill(pid_G, signo);
		kill(pid_L, signo);
	}
	else if (signo == SIGCONT) //Dump Log
	{
		printf("Received SIGCONT\n");
		printf("%d\n", pid_S);
		logFile(pid_S, (float)signo, newToken);
		printf("-%sPID: %d value:%s.\n", timeString, pid_S, signame[(int)signo]);
		printf("-%s%.3f.\n\n", timeString, newToken);
	}
}

int main(int argc, char *argv[])
{
	char port_N[128]; //Socket port of the next student
	char port_P[128]; //Socket port of the previous student
	char ip[32];	  //IP address of the next student
	int rf;			  //Frequency of the sine wave

	readConfFile(ip, port_N, port_P, &rf);

	int n;			   //Return value
	struct timeval tv; //Select delay

	char *argdata[4];  //Process G execution argument
	char *cmd = "./G"; //Process G executable path

	float msg1, msg2, t; //Message from P to L

	argdata[0] = cmd;
	argdata[1] = port_P;
	argdata[2] = (char *)myfifo2;
	argdata[3] = NULL;

	/*-----------------------------------------Pipes Creation---------------------------------------*/

	if (mkfifo(myfifo1, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|S
		perror("Cannot create fifo 1. Already existing?");

	if (mkfifo(myfifo2, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|G
		perror("Cannot create fifo 2. Already existing?");

	if (mkfifo(myfifo3, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|L
		perror("Cannot create fifo 3. Already existing?");

	int fd1 = open(myfifo1, O_RDWR); //apro la pipe1

	if (fd1 == 0)
	{	
		unlink(myfifo1);
		error("Cannot open fifo 1");
	}

	int fd2 = open(myfifo2, O_RDWR); //apro la pipe2

	if (fd2 == 0)
	{	
		unlink(myfifo2);
		error("Cannot open fifo 2");
	}

	int fd3 = open(myfifo3, O_RDWR); //apro la pipe3

	if (fd3 == 0)
	{	
		unlink(myfifo3);
		error("Cannot open fifo 3");
	}

	printf("\n");

	/*-------------------------------------------Process P--------------------------------------*/

	pid_P = fork();
	if (pid_P < 0)
	{
		perror("Fork P");
		return -1;
	}

	if (pid_P == 0)
	{
		float line_G = 0; //Recived message from G
		float line_S;	  //Recived message from S
		int retval, fd;
		fd_set rfds;

		sleep(2);

		/*-------------------------------------Socket (client) initialization---------------------------*/
		int sockfd, portno;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		portno = atoi(port_N);

		printf("Hey I'm P and my PID is : %d.\n", getpid());

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
			error("ERROR opening socket");

		server = gethostbyname(ip);
		if (server == NULL)
			error("ERROR, no such host\n");

		bzero((char *)&serv_addr, sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;

		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

		serv_addr.sin_port = htons(portno);
		if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
			error("ERROR connecting");

		n = write(sockfd, &line_G, sizeof(line_G));
		if (n < 0)
			error("ERROR writing to socket");

		while (1) //Select body
		{
			FD_ZERO(&rfds);
			FD_SET(fd1, &rfds);
			FD_SET(fd2, &rfds);

			fd = max(fd1, fd2);

			tv.tv_sec = 5;
			tv.tv_usec = 0;

			retval = select(fd + 1, &rfds, NULL, NULL, &tv);

			switch (retval)
			{

			case 0:
				printf("No data avaiable.\n");
				break;

			case 1:
				if (FD_ISSET(fd1, &rfds))
				{
					n = read(fd1, &line_S, sizeof(line_S));
					if (n < 0)
						error("ERROR reading from G");
					printf("From S recivedMsg = %.3f \n", line_S);
					sleep((int)line_S);
				}

				else if (FD_ISSET(fd2, &rfds))
				{
					n = read(fd2, &line_G, sizeof(line_G));
					if (n < 0)
						error("ERROR reading from G");
					/* if (line_G < -1 || line_G > 1)
					{
						printf("Value should be between -1 and 1!.\n");
						break;
					} */
					printf("From G recivedMsg = %.3f \n", line_G);

					n = write(fd3, &line_G, sizeof(line_G));
					if (n < 0)
						error("ERROR writing to L");

					line_G += 1;

					newToken = line_G;

					n = write(fd3, &newToken, sizeof(newToken));
					if (n < 0)
						error("ERROR writing to L");

					n = write(sockfd, &line_G, sizeof(line_G));
					if (n < 0)
						error("ERROR writing to socket");
				}

				sleep(1);
				break;

			case 2:
				n = read(fd1, &line_S, sizeof(line_S));
				if (n < 0)
					error("ERROR reading from S");
				printf("From S recivedMsg = %.3f \n", line_S);
				sleep((int)line_S);
				break;

			default:
				perror("You should not be here!");
				break;
			}
		}

		close(fd1);
		unlink(myfifo1);

		close(fd2);
		unlink(myfifo2);

		close(fd3);
		unlink(myfifo3);

		close(sockfd);
	}

	else
	{
		pid_L = fork();

		if (pid_L < 0)
		{
			perror("Fork L");
			return -1;
		}

		if (pid_L == 0)
		{
			printf("Hey I'm L and my PID is : %d.\n", getpid());

			while (1)
			{
				n = read(fd3, &msg1, sizeof(msg1));
				if (n < 0)
					error("ERROR reciving file from P");

				n = read(fd3, &msg2, sizeof(msg2));
				if (n < 0)
					error("ERROR reciving file from P");

				logFile(getpid(), msg1, msg2);
			}

			close(fd3);
			unlink(myfifo3);
		}

		else
		{

			/*----------------------------------------------Process G---------------------------------------*/

			pid_G = fork(); // FORK 2
			if (pid_G < 0)
			{
				perror("Fork G");
				return -1;
			}

			if (pid_G == 0)
			{
				printf("Hey I'm G and my PID is : %d.\n", getpid());
				execvp(argdata[0], argdata);
				error("Exec fallita");
				return 0;
			}

			/*----------------------------------------------Process S---------------------------------------*/

			pid_S = getpid();

			printf("Hey I'm S and my PID is : %d.\n", getpid());

			if (signal(SIGUSR1, sig_handler) == SIG_ERR)
				printf("Can't catch SIGUSR1\n");

			if (signal(SIGCONT, sig_handler) == SIG_ERR)
				printf("Can't catch SIGCONT\n");

			if (signal(SIGUSR2, sig_handler) == SIG_ERR)
				printf("Can't catch SIGUSER2\n");

			srand(time(0)); //current time as seed of random number generator
			sleep(5);

			while (1)
			{
				t = (rand() % (10 + 1));
				n = write(fd1, &t, sizeof(t));
				if (n < 0)
					error("ERROR writing to P");
				sleep(rand() % (10 + 1));
			}

			close(fd1);
			unlink(myfifo1);

			close(fd2);
			unlink(myfifo2);

			close(fd3);
			unlink(myfifo3);
		}

		return 0;
	}
}