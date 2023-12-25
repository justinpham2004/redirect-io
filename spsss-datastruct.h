/*
  Name: Justin Pham
  UID: 118649726
  TerpConnect: jpham04@umd.edu
  Section: 0307

  I promise that I have not given or received unauthorized aid on this
  assignment.
  
 */



#ifndef SPSSS_DATASTRUCT_H

#define SPSSS_DATASTRUCT_H

typedef struct Spsss_commands {
  char **compile_cmds;
  char **test_cmds;

  int num_compile_cmds;
  int num_test_cmds;
} Spsss_commands;

#endif
