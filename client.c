#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>



void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{	
	
    FILE*fileName;
    char *source;
	int sockfd, portno,tmp,n;//Εκχώριση μεταβλητών
  	int q=0;
	struct sockaddr_in serv_addr;//struct για server address
	struct hostent *server;//host address
	char buffer[256];
  if (argc < 3)
	{
			fprintf(stderr, "usage %s hostname port\n", argv[0]);//error αν το host address ειναι λαθος
			exit(0);
	}
	portno = atoi(argv[2]);//όρισμα ως port το δεύτεροόρισμα που δίνει ο χρήστης
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("ERROR opening socket\n");
		exit(1);
	}
	printf("Client Socket is created.\n");
	server = gethostbyname(argv[1]);//Host address το πρώτο όρισμα που δίνει ο χρήστης
	if (server == NULL)
	{
			fprintf(stderr, "ERROR, no such host\n");//Error αν το host ειναι λάθος
			exit(0);
	}
  //lab11_5_client
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	bcopy((char *)server->h_addr,
					(char *)&serv_addr.sin_addr.s_addr,
					server->h_length);

	tmp = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(tmp < 0){
		printf("Error in connection.\n");//Error στη σύνδεση
		exit(1);
	}
	printf("Connected to Server.\n");//Σύνδεση στον Server
		
	while(1){
		printf("%s_>",argv[1]);//Εμφάνιση hostaddres για την είσοδο μηνυμάτων
		bzero(buffer, 256);//Καθαρισμός buffer
		fgets(buffer, 256, stdin);//Eίσοδος από τον χρήστη
		n = write(sockfd, buffer,
      256);//read από sockfd
		
		if(strstr(buffer,"history")){
			FILE *file = fopen("history.txt", "r");
			char s;
			while((s=fgetc(file))!=EOF) {
		      printf("%c",s);
		   }
		   fclose(file);
		}
		FILE *file = fopen("history.txt", "a");
		fprintf(file, "\n%s",buffer);
		fclose(file);
		if (n < 0)
				error("ERROR writing to socket");
    if (strncmp("EXIT", buffer, 4) == 0) {//Αν ο χρήστης δώσει την λέξη EXIT ο client αποσυνδέεται από τον server
    		fclose(fopen("history.txt", "w"));
			close(sockfd);
			printf("Disconnected from server.\n");
			exit(1);
		}
    bzero(buffer, 256);
    n=read(sockfd, buffer,256);
    if (n>0){
      printf("%s\n", buffer);
			bzero(buffer, 256);
      if(n==256){
        while(n == 256){	//Αν γεμίσει ο buffer συνεχίχει μέχρι να εκτυπώσει όλα τα αποτελέσματα.
          n=read(sockfd, buffer, 256);
          printf("%s", buffer);
          bzero(buffer, 256);
        }
      }
    }else{
      if (n < 0) error("ERROR reading from socket");
      if (n==0) error("Server is closed");
    }
  }
	  return 0;
}
