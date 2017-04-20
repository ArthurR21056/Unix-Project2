#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int mysh_execute(char** args) {
  pid_t pid, wpid;
  int status;
  pid = fork();
  if (pid == 0) {
      char *progPath = getenv(args[0]);
      if (!progPath) {progPath = args[0];}
      if (execvp(progPath, args) == -1) {
         printf("fail\n");
          exit(EXIT_FAILURE);
       }
   }
   else if (pid < 0) {
      printf("Error, fork\n");
   }    
   else {
       do {
          wpid = waitpid(pid, &status, WUNTRACED);
       } while (!WIFEXITED(status) && !WIFSIGNALED(status));
   }
   return 1;
}


int numDelims(char* str, char* delim) {
   int num = 0;
   char* c;
   for (c = delim; *delim; ++delim) {
      while (*str) {
         if (*str == *c)
	    num++;
	 str++;
       }
   }
   return num;
}

void splitLine(char* line, char*** cmdStr, char* delims) {
    *cmdStr = malloc(sizeof(char*) * (numDelims(line, delims) + 2));
    if (*cmdStr) {
        size_t i = 0;
        int length, j;
        char* token = strtok(line, delims);
        
        while (token != NULL) {
	    (*cmdStr)[i++] = strdup(token);
            token = strtok(0, delims);
        }
	(*cmdStr)[i] = '\0';
    }
}

int mysh_execPipe(char** pipes){
    char** args1 = NULL;
    char** args2 = NULL;
    splitLine(pipes[0],&args1, " ");
    splitLine(pipes[1],&args2, " ");

    pid_t pid1, pid2;
    int fd[2];
    pipe(fd);

    pid1=fork();
    if (pid1 == 0) {    
       dup2(fd[STDOUT_FILENO], STDOUT_FILENO);
       close(fd[STDIN_FILENO]);
       close(fd[STDOUT_FILENO]);
       char *progPath = getenv(args1[0]);
       if (!progPath) {progPath = args1[0];}
       if (execvp(progPath, args1) == -1){
          printf("fail\n");
          exit(EXIT_FAILURE);
       }
       exit(1);
   }
   pid2=fork();
   if (pid2 == 0) {
        dup2(fd[STDIN_FILENO], STDIN_FILENO);
        close(fd[STDOUT_FILENO]);
        close(fd[STDIN_FILENO]);        
        char *progPath = getenv(args2[0]);
        if (!progPath) {progPath = args2[0];}
        if(execvp(progPath, args2) == -1){
           printf("fail\n");
           exit(EXIT_FAILURE);
        }
        exit(1);
  }
  close(fd[STDIN_FILENO]);
  close(fd[STDOUT_FILENO]);
  wait(0);
  wait(0);
  
  free(args1);
  free(args2);   
  return 1; 
}


int mycd(char **args){
if (args[1] == NULL) //args[1] is target directory 
       fprintf(stderr, "mycd expects an argument \n");
else 
{
	if(chdir(args[1]) != 0){
		perror("Error");
	}
	return 1;
}
}

void set_env_variables(){
   char buffer[100];
   realpath("myls",buffer);
   setenv("myls",buffer, 0);
   realpath("mycat", buffer);
   setenv("mycat", buffer, 0);

}

int main(int argc, char const *argv[]) {
  const int SIZE = 512;
  int status;
  char line[SIZE];
  char temp_buf[SIZE];
  char **args = NULL;
  char **pipes = NULL;
  do {
    set_env_variables();
    printf("mysh-$ ");

    fgets(line, SIZE, stdin);
    line[strcspn(line, "\n")] = 0; //remove newline character
    
    /*check for pipes */
    if(strstr(line, "|")){
       splitLine(strcpy(temp_buf, line), &pipes, "|"); //split pipe cmds
      status = mysh_execPipe(pipes); 
    }
    else{ 
      splitLine(strcpy(temp_buf, line), &args, " "); //split into tokens
      
      /* check if cmd is a built-in or exit*/
      if (strcmp(args[0], "mypwd") == 0) {
        printf("%s\n", getenv("PWD"));
        status =1;
      }
      else if (strcmp(args[0], "mycd") == 0) {
        status = mycd(args);        
      }

      else if(strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0)
        status=0;
      else
        status = mysh_execute(args);
    }
  } while (status);

  free(args);
  free(pipes);
  return EXIT_SUCCESS;
}

