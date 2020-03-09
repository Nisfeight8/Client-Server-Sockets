#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#define READ 0
#define WRITE 1
#define STDIN 0
#define STDOUT 1
#define STDERR 2
void chopN(char *str, size_t n)
{
    assert(n != 0 && str != 0);
    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str+n, len - n + 1);
}
void pipeCommand(int sockfd,char* buffer)
{
    char comd1[128];
    char comd2[128];
    char* cmd1[10];
    char* cmd2[10];
    char* token;
    int fd[2];
    int pd = open("fileName.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(pipe(fd)<0)
        perror("pipe");
    int i=0;
    while(buffer[i]!='|')
    {
      comd1[i]=buffer[i];
        i++;
    }
    i--;
    comd1[i]='\0';
    //Στο string cοmd1 έχω την πρώτη εντολή
    i+=3;
    strcpy(comd2,buffer+i);// Εγγραφή της δεύτερης εντολής στο comd2
    token=strtok(comd1, " ");
    i=0;
    while(token!=NULL)
    {
        cmd1[i]=token;
        token=strtok(NULL," ");
        i++;
    }
    cmd1[i]=NULL;
    //tokenization για την δεύτερη εντολή
    token=strtok(comd2, " ");
    i=0;
    while(token!=NULL)
    {
        cmd2[i]=token;
        token=strtok(NULL," ");
        i++;
    }
    cmd2[i]=NULL;
    pid_t pid;
    switch (pid = fork())
  	{
  		case 0: //child
      close(fd[READ]);
      dup2(fd[WRITE],STDOUT);
      dup2(fd[WRITE],STDERR);
      execvp(cmd1[0],cmd1);
      perror(cmd1[0]);
    default: // parent
        //  το stdout και stderr δείχνουν πλέον στο άκρο εγγραφής και γίνεται exec
        // H Εxec υπο φυσιολογικές συνθήκες , εκτυπώνει το αποτέλεσμα της στο stdout
        //λόγω της dup το αποτέλεσμα θα γραφτεί στο άκρο εγγραφής του pipe
        close(fd[WRITE]); // Το παιδί κλείνει το άκρο εγγραφής
        dup2(fd[READ],STDIN);
        dup2(pd,STDOUT);
        dup2(pd,STDERR);
        close(pd);//Kλείσιμο αρχείου
        execvp(cmd2[0],cmd2); //εκτελεί την exec και λόγω της αλλαγής του stdin -> pd τα αποτελέσματα πάνε στο FILE
        perror(cmd2[0]);
    case -1:
  			perror("fork");
  			exit(1);
		}
}
void redirect(char* command,int newsockfd)
{
    char comd[32];
    char file[32];
    char* splittedcommand[10];
    char* token;
    int i=0;
    while( command[i]!='>' && i<32 && i <strlen(command) )
    {
        comd[i]=command[i];
        i++;
    }
    comd[i-1]='\0';
    command+=i+2;
    strcpy(file,command);
    unlink(file);
    i=0;
    int cntr=0;
    while(comd[i]!='\0')
    {
        if(comd[i]==' ')
        {
            comd[i]='\0';
            cntr++;
        }
        i++;
    }
    int j=0;
    char* position=comd;
    for(i=0; i<=cntr; i++)
    {
        splittedcommand[j]=position;
        j++;
        position+=strlen(position)+1;
    }
    splittedcommand[j]=NULL;
    int filen=open(file,O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,0666);
    if(filen==-1)
      {
          perror("open function redirection");
      }
    else{ 
      int n;
      n = write(newsockfd, "FILE CREATED", 12);
    }
    dup2(filen,STDOUT);
    execvp(splittedcommand[0],splittedcommand);
    exit(1);
    perror("exec redirection function");
}
void check_child_exit(int status){//Συνάρτηση για τον ελεγχο της εξόδου του παιδιου
    if (WIFEXITED(status))
        printf("Child ended normally. Exit code is %d\n",WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("Child ended because of an uncaught signal, signal = %d\n",WTERMSIG(status));
    else if (WIFSTOPPED(status))
        printf("Child process has stopped, signal code = %d\n",WSTOPSIG(status));
}
void signalhandler(int signum)//Συνάρτηση για την προβολή μηνυμάτων μετα την χρήση ctrl c
{
	printf("Caught signal %d\n",signum);
	exit(signum);
}
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
  signal(SIGINT, signalhandler);//Kάλεσμα συνάρτησης για προβολη μηνυματος μετα το ctrl c
  signal(SIGTERM, signalhandler);//Καλεσμα συνάρτησης για προβολή μηνύματος μετά το ctrl c
	int sockfd,portno,tmp,n;//Αρχικοποίηση μεταβλητών
  struct sockaddr_in serv_addr;//struct για το server address
	socklen_t clilen;
	int newsockfd;
  FILE*fileName;//Δημιουργία αρχείου
  FILE*fp;
	struct sockaddr_in cli_addr;//struct για client address
  char buffer[256];
	char str[INET_ADDRSTRLEN];
	pid_t childpid;//pid παιδιου
  char temp_buffer[19]="Directory changed";

	sockfd = socket(AF_INET, SOCK_STREAM, 0);//sockfd socket γονέα
	if (argc < 2)
	{
			fprintf(stderr, "No port provided\n");//Error αν δεν έχει δώσει ο χρήστης port
			exit(1);
	}
	if(sockfd < 0){
		printf("Error in connection.\n");//Error στην σύνδεση
		exit(1);
	}
	printf("Server Socket is created.\n");
	portno = atoi(argv[1]);//port είναι το δεύτερο όρισμα που δίνει ο χρήστης κατά την εκτέλεση του Server
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	tmp = bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	if(tmp < 0){
		printf("Error in binding.\n");
		exit(1);
	}
	printf("Bind to port %d\n", portno);//Εμφάνιση port

	if(listen(sockfd, 10) == 0){
		printf("Listening for connections\n");//ψάχνει για συνδέσεις
	}else{
		printf("Error in binding.\n");
	}
	while(1 && strncmp("EXIT", buffer, 4) != 0){
    clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);//accept client δλδ newsockfd, το παιδι δλδ
		if(newsockfd < 0){
			perror("ERROR on accept");//Error στην αποδοχή
		}
		printf("Accepted connection\n");//Αποδοχή σύνδεσης
		if (inet_ntop(AF_INET, &cli_addr.sin_addr, str, INET_ADDRSTRLEN) == NULL) {
        fprintf(stderr, "Could not convert byte to address\n");
        exit(1);
    }
		fprintf(stdout, "The client address is :%s\n", str);//Εμφάνιση address
		if((childpid = fork()) == 0){//fork
			close(sockfd);
			while(1){
				bzero(buffer, 256);//Καθάρισμα buffer
		    n = read(newsockfd, buffer, 256);//read από newsockfd
        buffer[strlen(buffer)-1]='\0';//Αφαίρεση newline από buffer
        int pid,status;//Εκχώριση μεταβλητών
        char *argv[2];//Δημιουργία πίνακα από χαρακτήρες
        if ((childpid=fork())==-1)    //Έλεγχος για errors
        {
            perror("fork");
            exit(1);
        }
        if (childpid!=0)  		//The parent process
        {
            printf("I am parent process %d\n",getpid());
            if (wait(&status)==-1) //Wait for child
            {
                perror("wait");
            }
            check_child_exit(status);
        }
        else  			//The child process
        {
          if(strstr(buffer,"|"))
          {    
            printf("found pipe");
            pipeCommand(newsockfd,buffer);
          }
          else if(strstr(buffer," > "))
          {   
            printf("found redirection");
            redirect(buffer,newsockfd);
          }
          else if(strstr(buffer,"cd"))
               //Αν η εντολή ξεκινάει με "cd", τότε καλείται διαδικασία για αλλαγή καταλόγου
            {
              pid_t pid;
              switch (pid = fork())
              {
                case 0: //child
                  chopN(buffer,3);
                  char *temp;
                  temp=buffer;
                  memmove(temp+1, temp, 256);
                  temp[0] = '/';
                  printf("%s\n",temp);
                  char s[100]; 
                  // printing current working directory 
                  printf("%s\n", getcwd(s, 100)); 
                  // using the command 
                  chdir(temp); 
                  // printing current working directory 
                  printf("%s\n", getcwd(s, 100)); 
                  // after chdir is executed  
                default:
                  
                 fp = fopen("fileName.txt", "w");
                 fprintf(fp, "\n%s",temp_buffer);
                  fclose(fp);
                case -1:
                  perror("fork");
                  exit(1);
                
              }
            }
          else if(strstr(buffer,"history")){
            printf("The client asked for History\n");
            char temp_buffer[19]="here is the history";
            fp = fopen("fileName.txt", "w");
            fprintf(fp, "\n%s",temp_buffer);
            fclose(fp);
          }
          else{
           //tokenization
            char seperator[]= " ";
            int num=-1;
            char *newargv[256];
            char *token, *comm;
            int pd = open("fileName.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            token =strtok(buffer,seperator);
            comm=token;
            while(token!=NULL){
              newargv[++num]=token;
              token=strtok(NULL , seperator);
            }
            newargv[++num]='\0';
            printf("I am child Process %d \n",getpid());
          //ανακατεύθυνση του output της exec σε αρχείο για εμφάνιση στον Client
            dup2(pd, STDOUT);
            dup2(pd, STDERR);
            close(pd);//Kλείσιμο αρχείου
            execvp(comm,newargv);//χρήση exec
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        }

        if (strncmp("EXIT", buffer, 4) == 0) {//Μετά το δώσιμο της λέξης EXIT κάνει disconnect ο server.
					printf("Disconnected from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					break;
				}else{//Αλλιώς λαμβάνει το μύνημα απο τον client
  			    printf("Executing command: %s\n", buffer);
            char *source = NULL;
            int bufsize=0;
            FILE *fp = fopen("fileName.txt", "r");
            if (fp != NULL) { // Πηγαινε στο τέλος του αρχείου.
              if (fseek(fp, 0L, SEEK_END) == 0) {//Πάρε το μέγεθος του αρχείου.
                bufsize = ftell(fp);
                if (bufsize == -1) { // Error
                  printf("Error reading file");
                }
                // Κάνω τον buffer να έχει μέγεθος ίσα με το μέγεθος του αρχείου.
                source = malloc(sizeof(char) * (bufsize + 1));

              // Πήγαινε πάλι στην αρχή του αρχείου.
                if (fseek(fp, 0L, SEEK_SET) != 0) { //ERROR
                }
                //Διάβασε όλο το αρχείο στην μνήμη.
                size_t newLen = fread(source, sizeof(char), bufsize, fp);
                if ( ferror( fp ) != 0 ) {
                  fputs("Error reading file", stderr);
                } else {
                  source[newLen++] = '\0';
                }
              }
              fclose(fp);
              n = write(newsockfd, source, bufsize);//Στέιλε στον client τοαρχείο ως buffer.
              //printf("%s\n", source);
              fclose(fopen("fileName.txt", "w"));//Καθάρισμα αρχείου.

            }
            free(source);
  			    if (n < 0) error("ERROR writing to socket");
  					bzero(source, bufsize);
				
          
        }

			}
		}
	}
  close(newsockfd);
	return 0;
}
