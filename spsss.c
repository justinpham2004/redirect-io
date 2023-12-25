/*
  Name: Justin Pham
  UID: 118649726
  TerpConnect: jpham04@umd.edu
  Section: 0307

  I promise that I have not given or received unauthorized aid on this
  assignment.
  
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <err.h>

#include "safe-fork.h"
#include "spsss.h"
#include "spsss-datastruct.h"
#include "split.h"

#ifdef SPSSS_H

/*
#define SUCCESSFUL_COMPILATION 1
#define FAILED_COMPILATION 0
#define LINE_MAX 1024
*/
#define IN_CAP 10



static void redirect(char **cmd);

Spsss_commands read_spsss_commands(const char compile_cmds[],
                                   const char test_cmds[]) {

  FILE *compile_file;
  FILE *test_file;
  char curr_cmd[LINE_MAX];
  Spsss_commands commands;

  int comp_cap = IN_CAP;
  int test_cap = IN_CAP;


  /*error case 1: if either parameter is NULL*/
  if(compile_cmds == NULL || test_cmds == NULL) {
    perror("COMPILE or TEST COMMANDS NULL");
    exit(1);
  }

  compile_file = fopen(compile_cmds, "r");
  if(compile_file == NULL ) {
    perror("COMPILE FILE OPEN FAILURE");
    exit(1);
  }
  
  test_file = fopen(test_cmds, "r");
  if(test_file == NULL) {
    fclose(compile_file);
    perror("TEST FILE OPEN FAILURE");
    exit(1);
  }
  /*error case 3: if there is an error reading either file*/
  /*read from compilation commands*/

  /*allocate memory for Spsss_command structure */ 
  commands.compile_cmds = malloc(sizeof(char *) * IN_CAP);
  commands.test_cmds = malloc(sizeof(char *) * IN_CAP);
  commands.num_compile_cmds = 0;
  commands.num_test_cmds = 0;

  
  while(fgets(curr_cmd, LINE_MAX, compile_file) != NULL) {

    if(commands.num_compile_cmds >= comp_cap) {
      /*reszie array*/
      
      comp_cap *= 2;
      
      
      commands.compile_cmds = realloc(commands.compile_cmds,
				      sizeof(char *) * comp_cap);
      
    }

    commands.compile_cmds[commands.num_compile_cmds] = malloc(strlen(curr_cmd) + 1);
    
    strcpy(commands.compile_cmds[commands.num_compile_cmds], curr_cmd);
    commands.num_compile_cmds++;
  }

  while(fgets(curr_cmd, LINE_MAX, test_file) != NULL) {

    if(commands.num_compile_cmds >= test_cap) {
      /*resize array*/
      test_cap *= 2;
      commands.test_cmds = realloc(commands.test_cmds,
				   sizeof(char *) * test_cap);            
    }

    commands.test_cmds[commands.num_test_cmds] =
      malloc(strlen(curr_cmd) + 1);
    strcpy(commands.test_cmds[commands.num_test_cmds], curr_cmd);
    commands.num_test_cmds++;
  }

  fclose(compile_file);
  fclose(test_file);

  return commands;
}


void clear_spsss_commands(Spsss_commands *const commands) {
  /*iterate through array of commands, and clear them*/
  int i;

  if(commands == NULL) {
    return;
  }
  for(i = 0; i < commands->num_compile_cmds; i++) {
    /*
    printf("freeing: %s", commands->compile_cmds[i]);
    */
    free(commands->compile_cmds[i]);
  }

  free(commands->compile_cmds);
  
  for(i = 0; i < commands->num_test_cmds; i++) {
    free(commands->test_cmds[i]);
  }

  free(commands->test_cmds);
  
  commands->num_compile_cmds = 0;
  commands->num_test_cmds = 0;
}


int compile_program(Spsss_commands commands) {
  int i;
  int j;
  int status;
  
  pid_t pid;
  /*
  char **comp_cmd;
  */

  if( commands.compile_cmds == NULL || commands.compile_cmds[0] == NULL) {
    return SUCCESSFUL_COMPILATION;
  }

  /*iterate through compilation commands*/
  for(i = 0; i < commands.num_compile_cmds; i++) {
    char **comp_cmd = split(commands.compile_cmds[i]);
    /*run command*/
    pid = safe_fork();

    if(pid > 0) {
      /*parent*/
      waitpid(pid, &status, 0);
      
      for(j = 0; comp_cmd[j] != NULL; j++) {
	free(comp_cmd[j]);
      }
      free(comp_cmd);

      if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {

	return FAILED_COMPILATION;
      }

            
    } else if(pid == 0) {
      /*child process*/
      redirect(comp_cmd);
      /*after redirection check*/
      if(execvp(comp_cmd[0], comp_cmd) == -1) {
	perror("FAILED EXECVP CALL: COMPILATION COMMAND FAILED");
	return FAILED_COMPILATION;
      }
      
    } else {
      return FAILED_COMPILATION;
    }  
  }  
  return SUCCESSFUL_COMPILATION;
 
}

int test_program(Spsss_commands commands) {
  int i;
  int j;
  int status;
  int tests_passed = 0;

  pid_t pid;
  char **test_cmd;

  if (commands.test_cmds == NULL || commands.compile_cmds[0] == NULL) {
    return SUCCESSFUL_COMPILATION;
  }

  for(i = 0; i < commands.num_test_cmds; i++) {
    /*split*/
    test_cmd = split(commands.test_cmds[i]);
    /*fork*/
    pid = safe_fork();

    if(pid == 0) {
      /*child*/
      redirect(test_cmd);
      if(execvp(test_cmd[0], test_cmd) == -1) {
	return FAILED_COMPILATION;
      }
      
    } else if( pid > 0) {
      /*parent*/
      waitpid(pid, &status, 0);
      if( WIFEXITED(status) &&  WEXITSTATUS(status) == 0) {
	tests_passed++;
      }
      

    } else {
      /*failed fork*/
      perror("FAILED FORK: CHILD WAS NOT ABLE TO BE SUCCESSFULLY CREATED");
      exit(1);
    }

    for(j = 0; test_cmd[j] != NULL; j++) {
      free(test_cmd[j]);
    }
    free(test_cmd);
  }
  return tests_passed;
    
}

static void redirect(char **cmd) {
  int i;
  int j;
  int in_fd;
  int out_fd;
  /*check for input/output redirection*/
  for(i = 0; cmd[i] != NULL; i++) {
    if(strcmp(cmd[i], "<") == 0) {
      /*input redirection*/
      in_fd = open(cmd[i+1], O_RDONLY);
      if(in_fd == -1) {
	perror("FILE REDIRECT/IN  UNSUCCESSFULLY OPENED");
	exit(1); /*what exit status am i using?*/
      }

      dup2(in_fd, STDIN_FILENO);
      close(in_fd); /*done using*/
    
      /*free the command and the file name from mem*/
      free(cmd[i]);
      free(cmd[i+1]);

      /*now shift all characters down if the input is not the last char*/
      for(j = i; cmd[j] != NULL; j++) {
	cmd[j] = cmd[j + 2];
      }

    } else if(strcmp(cmd[i], ">") == 0) {
      out_fd = open(cmd[i+1], FILE_FLAGS, FILE_MODE);
      if(out_fd == -1) {
	perror("FILE REDIRECT/OUT UNSUCCESSFULLY OPENED");
	exit(1);
      }

      dup2(out_fd, STDOUT_FILENO);
      close(out_fd);

      free(cmd[i]);
      free(cmd[i+1]);

      for(j = i; cmd[j] != NULL; j++ ) {
	cmd[j] = cmd[j + 2];
      }
    }
  } 
}

#endif
  

  
