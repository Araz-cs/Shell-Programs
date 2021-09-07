
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h> 
#include <assert.h>
#include <semaphore.h>

#define MAXARGS 80
#define BUFFERSIZE 80


int MAXLINE = 600;
int LISTENQ = 600;

int count = 0;


struct data{
    char name[10];
    int thread;
    int location;
};
struct data table[4];

int openAppend = 0;

int fileRemoved(struct data * table, char * name, int p)
{ 
    int check = 0;
    int i = 0;
    while (i < count)
        {
            if(table[i].thread == p && strcmp(table[i].name, name) == 0)
            {
                // printf("\n\n\tFile name : %s \n \tThread Number : %i \n \tLocation : %i\n\n", table[i].name, table[i].thread, table[i].location);
                strcpy(table[i].name, "\0");
                table[i].thread = 0;
                table[i].location =0;

                int j = i;
                while(j < count)
                {
                    strcpy(table[j].name, table[j+1].name);
                    table[j].thread = table[j+1].thread;
                    table[j].location = table[j+1].location;
                    j++;
                }
                check = 1;
                count--;
                break;
            }
            else
            {
                check = 0;
            }
            i++;
        }
    
    return check;
}

int open_listenfd(char *port)
{
    printf("listened called\n");
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    getaddrinfo(NULL, port, &hints, &listp);

    for(p = listp; p; p = p->ai_next)
    {
        if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue;
        }
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }
        close(listenfd);
    }

    freeaddrinfo(listp);
    if(!p)
    {
        return -1;
    }
    if(listen(listenfd, LISTENQ) < 0)
    {
        close(listenfd);
        return -1;
    }
    return listenfd;
}
char **split(const char *str, const char *delim)
{
    char *aux;
    char *p;
    char **res;
    char *argv[MAXARGS]; 
    int n = 0, i;

    assert(aux = strdup(str));
	for (p = strtok(aux, delim); p; p = strtok(NULL, delim))
	{
		argv[n++] = p;
	}
    argv[n++] = NULL;
    
	//putting strdup() string one place past the NULL, so we can free(3), once finished  
    argv[n++] = aux;

	// Now we need to copy array
    assert(res = calloc(n, sizeof (char *)));
    for (i = 0; i < n; i++)
	{
        res[i] = argv[i];
	}
    return res;
}

static sem_t mutex;
static void init_echo(void)
{
    sem_init(&mutex, 0, 1);
}

void echo(int connfd)
{
    FILE *readinFile;
    
    char input[MAXLINE + 1];
    char **commands;
    char * output;
    size_t n;
    
    int i = 0;
    
    char appendFileName[MAXLINE];
    char readFileName[MAXLINE];

  
    
    
    int openRead = 0;
    

    char buffer[200];
    char text[200]= { 0 };
    char temp[200] ={ 0 };
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, init_echo);
    
	
    

    while((n = read(connfd, input, 200)) != 0)
    {
        sem_wait(&mutex);
		input[strlen(input)-1] = '\0';
		commands = split(input, " ");
        
        // printf("======================\n");
        // printf("Input : %s\n", input);
        // printf("command1 : %s\n", commands[0]);
        // printf("command2 : %s\n", commands[1]);
        // printf("command3 : %s\n", commands[2]);
        // printf("======================\n");
        int found = 0; 
        if(strcmp(commands[0], "openRead") == 0)
        {
            int j = 0;
            while ( j < 4)
            {
                if((strcmp(table[j].name, commands[1]) == 0) && (table[j].thread != (int)pthread_self()) && openAppend == 1)      
                {
                    found = 2;
                }
                else if(table[j].thread == (int)pthread_self() && openRead == 1)
                {
                    found = 1;
                }
                else if ((table[j].thread == (int)pthread_self() && openRead == 1) || table[j].thread == (int)pthread_self())
                {
                    found = 3;
                }
                j++;
            }
            if ( found == 0 )
            {
                readinFile = fopen(commands[1], "r");
            
                if(readinFile == NULL)
                {
                    perror("CAN'T OPEN FILE");
                    exit(1);
                }
                else
                {
                    puts(input);
                    strcpy(readFileName, commands[1]);
                    strcpy(table[count].name, commands[1]);
                    table[count].thread = (int)pthread_self();
                    table[count].location = 0;
                    openRead = 1;
                    output = "";
                    count++;

                    // fgets(buf, sizeof(buf), readinFile);

                    // strcat(line, buf);

                    while(fgets(buffer, sizeof(buffer), readinFile))
                    {
                        strcat(text, buffer);
                    }
                    // printf("%s \n", text);
                }
                fclose(readinFile);
            }
            else
            {
                if(found == 1)
                {
                    output = "A file is already open for reading";
                    found =0;
                    puts(output);
                }
                else if (found == 2)
                {
                    output = "The file is open by another client";
                    found =0;
                    puts(output);
                }
                else if (found == 3)
                {
                    output = "A file is already open for appending";
                    found =0;
                    puts(output);
                }
            }
            
            
        }
        else if(strcmp(commands[0], "openAppend") == 0)
        {
            int j = 0;
            while ( j < 4)
            {
                if(table[j].thread == (int)pthread_self() )
                {
                    found = 1;
                    // printf("IN FIRST THREAD CHECK\n");
                }
                else if (openRead == 1 )
                {
                    found = 3;
                    // printf("IN SECOND THREAD CHECK\n");
                }
                else if ((strcmp(table[j].name, commands[1]) == 0) && (table[j].thread != (int)pthread_self()))
                {
                    found = 2;
                }
                j++;
            }
            if(found == 0 && openAppend ==0 && (table[j].thread != (int)pthread_self()))
            {
                readinFile = fopen(commands[1], "a");
                if(readinFile == NULL)
                {
                    perror("can't open file");
                    exit(1);
                }
                else
                {
                    puts(input);
                    strcpy(appendFileName, commands[1]);
                    strcpy(table[count].name, commands[1]);
                    table[count].thread = (int)pthread_self();
                    table[count].location = 0;
                    openAppend = 1;
                    output= "";
                    count++;
                }
                fclose(readinFile);
            }
            else
            {
                if(found == 1)
                {
                    output = "A file is already open for appending";
                    found =0;
                    puts(output);
                }
                else if(found == 2)
                {
                    output = "The file is open by another client";
                    found =0;
                    puts(output);
                }
                else if (found ==3)
                {
                    output = "A file is already open for reading";
                    found = 0;
                    puts(output);
                }
            }

        }
        else if(strcmp(commands[0], "read") == 0)
        {

            
            // char * ps;
            int i = 0;
            int counter = 0;
            int length = 0;
            if (openRead == 1)
            {
                while (i < 4)
                {
                    if(table[i].thread == (int)pthread_self())
                    {
                        int loc =  table[i].location;           // loc = 0;
                        counter = atoi(commands[1]);            // read 2 getting the (2)
                        length = table[i].location + counter;   // 0 + 2
                        // output = " in read"; 
                        char symbol[20]= { 0 };      
                        int j = 0;             
                        while (loc < length)     // 0 < 0 + 2
                        {
                            // strcpy(symbol[j], text[loc]);
                            symbol[j] = text[loc];             
                            // *ps = symbol + 1;
                            // strncat(output, &symbol, 1);
                            loc++;
                            j++;
                        }
                        output = symbol;
                        table[i].location += counter;
                        puts(input);
                        break;
                    }
                    else
                    {
                        output = "File not open";
                        puts(output);
                    }
                    i++;
                }
            }
            else
            {
                output = "File not open";
                puts(output);
            }
   
        }
        else if(strcmp(commands[0], "append") == 0)
        {
            if(openAppend == 1)
            {
                puts(input);
                readinFile = fopen(appendFileName, "a");
                output = "";
                fputs(commands[1], readinFile);
                fclose(readinFile);
            }
            else
            {
                output = "file not open";
                puts(input);
            }
            
        }
        else if(strcmp(commands[0], "close") == 0)
        {
            puts(input);
            int check =0;
            check = fileRemoved(table ,commands[1], (int)pthread_self());
            // found = 0;
            if (check == 0)
            {
                output = "file can not close";
            }
            else
            {
                output = "";
                if (openAppend == 1)
                {
                    openAppend = 0;
                }
                else if( openRead == 1 )
                {
                    openRead = 0;
                }
            }
            //checking if file is open for appending or reading
           
            
        }
        else
        {
            output= "invalid syntax";
            puts(output);
        }

        sem_post(&mutex);
        int k;
        // for(k = 0; k < 4; k++)
        // {
        //     printf("           Tracker\n ");
        //     printf("\tFile name : %s \n \tThread Number : %i \n \tLocation : %i\n", table[k].name, table[k].thread, table[k].location);
        //     printf("\t======================\n");
        // }
        // printf("======================\n");




        int size = strlen(output) + 1;
        // write(connfd, &size, 1);
        write(connfd, output, size);
        free(commands);  
    
    }
   
}
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}
int main(int argc, char **argv)
{

    int i,listenfd, *connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; 
    pthread_t tid;
    listenfd = open_listenfd(argv[1]);

    // char input[MAXLINE];
    // char * commands[200] = {"placeholder"};
    if ( listenfd > 0){
        printf("server started\n");
        while (1)
        {
            clientlen = sizeof(struct sockaddr_storage); 
            connfd = malloc(sizeof(int));
            *connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
            pthread_create(&tid, NULL,thread,connfd);

            
        }

        
    }else{
        perror("Listenfd < 0");
    }
    exit(0);
    return 0;
}

