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
#include <sys/time.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <netdb.h>

#define max(a, b) \
	({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct{
	timeval time;
	double token = 0;
}msg;

msg message;

char *timeString;
pid_t pid_S, pid_G, pid_L, pid_P;

char name[30] = "token_";

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

// Read the configuration file info
void readConfFile(char *ip, char *port, int *wt, int *rf)
{
	fp = fopen("ConfigurationFile.txt", "r");

	if (fp == NULL)
		error("Failed to open ConfigurationFile");

	fscanf(fp, "%s", ip);
	fscanf(fp, "%s", port);
	fscanf(fp, "%d", wt);
	fscanf(fp, "%d", rf);

	printf("IP : %s\n", ip);
	printf("Port : %s\n", port);
	printf("Waiting time : %d\n", wt);
	printf("RF : %d\n", *rf);

	fclose(fp);
}

void logFile(pid_t process_id, float msg, float token)
{
	FILE *f;
	f = fopen("logFile.log", "a");
	time_t currentTime;
	currentTime = time(NULL);
	timeString = ctime(&currentTime);
	fprintf(f, "-%sPID: %d value:%.3f.\n", timeString, process_id, msg);
	fprintf(f, "-%s%.3f.\n\n", timeString, token);

	fclose(f);
}

void tokenFileInit(int rf)
{
 	FILE *tokenFile;

	char rfStr[10];
	sprintf(rfStr, "%d", rf);
	strcat(name, rfStr);
	strcat(name,".txt");
	tokenFile = fopen(name, "w+");
}
void tokenFile(double token)
{
	FILE *tokenFile;
	tokenFile = fopen(name, "a");
	fprintf(tokenFile, "%f\n",token);
	fclose(tokenFile);
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
		kill(pid_P, SIGCONT);
		kill(pid_G, SIGCONT);
		kill(pid_L, SIGCONT);
	}
	else if (signo == SIGCONT) //Dump Log
	{
		printf("Received SIGCONT\n");
		printf("%d\n", pid_S);
		logFile(pid_S, (float)signo, message.token);
		printf("-%sPID: %d value:%s.\n", timeString, pid_S, signame[(int)signo]);
		printf("-%s%.3f.\n\n", timeString, message.token);
	}
}

int main(int argc, char *argv[])
{
	char port[128];   //Socket port
	char ip[32];	  //IP address of the next student
	int wt;			  // Waiting time
	int rf;			  //Frequency of the sine wave

	readConfFile(ip, port, &wt, &rf);

	int n;			   //Return value
	struct timeval tv; //Select delay

	char *argdata[4];  //Process G execution argument
	char *cmd = "./G"; //Process G executable path

	float msg1, msg2, t; //Message from P to L

	argdata[0] = cmd;
	argdata[1] = port;
	argdata[2] = (char *)myfifo2;
	argdata[3] = NULL;

	int flag = 1; // crescente

	tokenFileInit(rf);
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
		//float line_G = 0; //Recived message from G
		msg line_G;
		float line_S;	  //Recived message from S
		int retval, fd;
		fd_set rfds;

		struct timeval current_time;
		//struct timespec time; //current time for DT computation
		//struct timespec prev_time; //previous time for DT computation
		double delay_time;

		sleep(2);

		/*-------------------------------------Socket (client) initialization---------------------------*/
		int sockfd, portno;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		portno = atoi(port);
		float old_tok, new_tok;

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
			//Either no active pipes or the timeout has expired
				printf("No data avaiable.\n");
				break;

			case 1:
				// If only one pipe is active, check which is between S and G
				if (FD_ISSET(fd1, &rfds))
				{
					n = read(fd1, &line_S, sizeof(line_S));
					if (n < 0)
						error("ERROR reading from S");
					printf("From S recivedMsg = %.3f \n", line_S);
					sleep((int)line_S);
				}
				else if (FD_ISSET(fd2, &rfds))
				{
					// If G, make the computation and log the results through L
					n = read(fd2, &line_G, sizeof(line_G));
					if (n < 0)
						error("ERROR reading from G");

					/* if (line_G < -1 || line_G > 1)
					{
						printf("Value should be between -1 and 1!.\n");
						break;
					} */

					printf("From G recived token = %.3f \n", line_G.token);
					// send to L 
					n = write(fd3, &line_G, sizeof(line_G));
					if (n < 0)
						error("ERROR writing to L");

					// Get the current time 
					//clock_gettime(CLOCK_REALTIME,&time);
					gettimeofday(&current_time, NULL);
					printf("current time: %f \n", (double)(current_time.tv_sec + current_time.tv_usec/(double)1000000));

					// Compute DT

					delay_time = (double)(current_time.tv_sec - message.time.tv_sec) + (double)(current_time.tv_usec - message.time.tv_usec)/(double)1000000;
					printf("differenza: %f\n",delay_time);
					printf("line_G token %f\n", line_G.token);
					old_tok = line_G.token;
					//message.token = old_tok + delay_time * (1 - pow(old_tok,2)/2 ) * 2 * 3.14 * rf;

					printf("old tock: %f\n", fabs(old_tok));

					if (old_tok >= 1)
					{
						flag = 0; 
					}else if(old_tok <= -1)
					{
						flag = 1;
					}
					printf("flag: %d\n", flag);
					switch(flag)
						{
							case 0:
								message.token = old_tok * cos(2 * 3.14 * rf * delay_time) - sqrt(1 - pow(old_tok,2)/2 ) * sin(2 * 3.14 * rf * delay_time);
								break;
							case 1:
								message.token = old_tok * cos(2 * 3.14 * rf * delay_time) + sqrt(1 - pow(old_tok,2)/2 ) * sin(2 * 3.14 * rf * delay_time);
								break;
						}
						// save token values on a txt file
					tokenFile(message.token);
					
					message.time = current_time;
					//line_G += 1; 			////////////////////////////////////////////FORMULA////////////////////////////////////////////////
					//message.token = line_G;


					// Send new value to L
					n = write(fd3, &message, sizeof(message));
					if (n < 0)
						error("ERROR writing to L");

					// Write new value to the socket
					//n = write(sockfd, &line_G, sizeof(line_G));
					n = write(sockfd, &message, sizeof(message));
					if (n < 0)
						error("ERROR writing to socket");
					
					//clock_gettime(CLOCK_REALTIME,&prev_time);

					usleep(wt); 	//Simulate communication delay
				}

				sleep(1);
				break;

			case 2:
				// If two active pipes, give priority to S
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
				int a = execvp(argdata[0], argdata);
				printf("%d",a);
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

			//srand(time(0)); //current time as seed of random number generator
			sleep(5);

			while (1)
			{
				/* t = (rand() % (10 + 1));
				n = write(fd1, &t, sizeof(t));
				if (n < 0)
					error("ERROR writing to P");
				sleep(rand() % (10 + 1)); */
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