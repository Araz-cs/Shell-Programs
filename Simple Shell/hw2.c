#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <ctype.h> 
#include <signal.h> 
#include <fcntl.h> 
#include <assert.h>

#define BUFFERSIZE 80
#define MAXJOBS 5
#define MAXARGS 80

struct job
{
  int jobId;
  int pid;
  char* status;
  char cmd[BUFFERSIZE];
};

struct job jobs[MAXJOBS+1];
int jobCount = 0;
int i;

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

void ignoreHandler()
{
	return;
}

void childHandler(int sig)
{
	int pid;
	int status;
	pid = waitpid(WAIT_ANY, &status, WNOHANG | WNOWAIT);
	if (pid != -1)
	{
		int jobId = findJobId(pid);
		if(WIFSTOPPED(status))
		{
			jobs[jobId].status = "Stopped";
		}
		else
		{
			jobs[jobId].jobId = 0;
			jobCount--;
			waitpid(pid, 0, 0);
		}
	}
	return;
}

void waitJob(struct job* currentJob)
{
	int status;
	waitpid(currentJob->pid, &status, WUNTRACED);
	if(WIFSTOPPED(status))
	{
		currentJob->status = "Stopped";
	}
	else
	{
		currentJob->jobId = 0;
		jobCount--;
	}
	return;
}

int findJobId(int pid)
{
	for(i = 1; i <= MAXJOBS; i++)
	{
		if(jobs[i].pid == pid && jobs[i].jobId > 0)
		{
			return i;
		}
	}
	return 0;
}

void killAll()
{
	for(i = 1; i <= MAXJOBS; i++)
	{
		if(jobs[i].jobId > 0)
		{
			kill(jobs[i].pid, SIGKILL);
			jobs[i].jobId = 0;
			wait(0);
		}
	}
	jobCount = 0;
}
	
int main()
{
	char **args;
	char str[BUFFERSIZE + 1];
	char * token;
	
	signal(SIGCHLD, childHandler);
	signal(SIGINT, ignoreHandler);
	signal(SIGTSTP, ignoreHandler);
	while(1)
	{
		printf("prompt> ");

		fgets(str, BUFFERSIZE, stdin);
		str[strlen(str)-1] = '\0';
		args = split(str, " ");

		if(strcmp(args[0], "quit") == 0)
		{
			killAll();
			free(args);
			return 0;
		}
		
		else if(strcmp(args[0], "jobs") == 0)
		{
			for(i=1; i<=MAXJOBS; i++)
			{
				if(jobs[i].jobId > 0) // If entry is valid
				{
					printf("[%d] (%d) %s %s\n", jobs[i].jobId, jobs[i].pid, jobs[i].status, jobs[i].cmd);
				}
			}
		}
		
		else if(strcmp(args[0], "bg") == 0)
		{
			if(args[1][0] == '%')
			{
				i = atoi(args[1]+sizeof(char));
			}
			else
			{
				i = findJobId(atoi(args[1]));
			}
			
			jobs[i].status = "Running";
			kill(jobs[i].pid, SIGCONT);
		}
		
		else if(strcmp(args[0], "fg") == 0)
		{
			if(args[1][0] == '%')
			{
				i = atoi(args[1]+sizeof(char));
			}
			else
			{
				i = findJobId(atoi(args[1]));
			}
			
			jobs[i].status = "Foreground";
			kill(jobs[i].pid, SIGCONT);
			waitJob(&jobs[i]);
		}
		
		else if(strcmp(args[0], "kill") == 0)
		{
			if(args[1][0] == '%')
			{
				i = atoi(args[1]+sizeof(char));
			}
			else
			{
				i = findJobId(atoi(args[1]));
			}
			
			jobs[i].jobId = 0;
			kill(jobs[i].pid, SIGCONT);
			kill(jobs[i].pid, SIGINT);
			waitpid(jobs[i].pid, 0, 0);
		}
		
		else 
		{
			if(args[0] == NULL)
			{
				return 0;
			}
			
			if(jobCount >= MAXARGS)
			{
				printf("Max jobs reached. Couldn't create job.");
				return 0;
			}
			
			
			int pid = fork();
			jobCount++;
			i = 0;
			
			if(pid == 0)
			{
				while (args[i] != NULL)
				{
					if (strcmp(args[i],"&") == 0)
					{
						args[i] = NULL;
					}
					else if (strcmp(args[i],"<") == 0)
					{
						close(0);
						open(args[i+1], O_RDONLY);
						args[i] = NULL;
					}
					else if (strcmp(args[i],">") == 0)
					{
						close(1);
						open(args[i+1], O_CREAT|O_WRONLY, S_IRWXU);
						args[i] = NULL;
					}
					i++;
				}
				if(execvp(args[0], args) == -1 && execv(args[0], args) == -1)
				{
					printf("Command not executed");
					return 0;
				}
			}
			else
			{
				// find open job entry
				for(i = 1; i<=MAXJOBS; i++)
				{
					if(jobs[i].jobId == 0) // If entry is open
					{
						jobs[i].jobId = i;
						jobs[i].pid = pid;
						strcpy(jobs[i].cmd, str);
						if(strcmp(jobs[i].cmd + strlen(jobs[i].cmd) - 2, " &") == 0)
						{
							jobs[i].status = "Running";
						}
						else
						{
							jobs[i].status = "Foreground";
							waitJob(&jobs[i]);
						}
						break;
					}
				}
			}
		}
	}
	
	killAll();
	free(args);
	
	return 0;
}