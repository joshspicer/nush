#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h> //Bitwise stuf
#include <sys/wait.h>
#include <stdlib.h>
#include <assert.h>

#include "tokenize.h"
#include "svec.h"

#include <fcntl.h>  // for open sysCall

// Written By Josh Spicer
// Used Nat Tuck's "sort-pipe.c" example from class


// HELPERS
void routeInHelper(svec* left, svec* right);
void routeOutHelper(svec* left, svec* right);
void pipeHelper(svec* left, svec* right);
void andHelper(svec* left, svec* right);
void orHelper(svec* left, svec* right);
void backgroundHelper(svec* vector);
// STUBS
void splitInTwo(svec* original, svec* left, svec* right, int index);
// SUB-MAIN
void processTokens(svec* tokens);
void execute(char* cmd, char* arg[]);


// ------------------------------------------------------------------------- //

// Handles the route in redirection
void
routeInHelper(svec* left, svec* right) {

   int cpid;

   // FORK
   if ((cpid = fork())) {
      //** PARENT ** //

      int status;
      waitpid(cpid, &status, 0);

   } else {
      // ** CHILD ** //

      // OPEN TARGET
      int input = open(svec_get(right,0), O_RDONLY);

      // DUP
      close(0); //Close std:in
      dup(input);
      close(input);

      // EXEC
      // Process additional tokens recursively
      processTokens(left);
      // char* args[] = {svec_get(left,0), 0};
      // execute(svec_get(left,0), args);
   }
}

// ------------------------------------------------------------------------- //

// Handles the route out redirection
void
routeOutHelper(svec* left, svec* right) {

   int cpid;

   // FORK
   if ((cpid = fork())) {
      //** PARENT ** //

      int status;
      waitpid(cpid, &status, 0);

   } else {
      // ** CHILD ** //

      // Open Target
      int output =
      open(svec_get(right,0),
      O_CREAT | O_TRUNC | O_WRONLY,
      S_IWUSR | S_IRUSR);

      // DUP
      close(1); //Close std:out
      dup(output);
      close(output);

      // EXEC
      // Process additional tokens recursively
      processTokens(left);
      close(1); //Close std:out again
   }
}

// ------------------------------------------------------------------------- //

// Helper for the pipe command
// void
// pipeHelper(svec* left, svec* right) {
//
//    int cpid, rv;
//
//    int pipes[2];
//    rv = pipe(pipes);
//    assert(rv != -1);
//
//    // pipes[0] is for reading
//    // pipes[1] is for writing
//
//    if ((cpid = fork())) {
//       // ** PARENT ** //
//
//       close(pipes[1]);
//
//       int status;
//       waitpid(cpid, &status, 0);
//
//
//    }
//    else {
//       // ** CHILD ** //
//
//       close(pipes[0]);
//       close(1); // stdout
//
//       rv = dup(pipes[1]);
//       assert(rv != -1);
//
//       rv = pipeHelperLeft(left, pipe[1]);
//       assert(rv != -1);
//       rv = pipeHelperRight(right, **);
//       assert(rv != 1);
//
//       printf("%s\n", "DANGER: ran past execvp in PIPE-HELPER function.");
//    }
//
// }

// ------------------------------------------------------------------------- //

// Helper for the AND command
void
andHelper(svec* left, svec* right) {

   int cpid;

   // FORK
   if ((cpid = fork())) {
      //** PARENT ** //

      int status;
      waitpid(cpid, &status, 0);

      // Print the status returned
      //printf("%d\n", status); //TEST CODE

      // If and only if the left side is TRUE, evaluate the right side
      if (status == 0) {
         processTokens(right);
      }

   } else {
      // ** CHILD ** //

      // EXEC
      char* command = svec_get(left, 0);
      char* argument[50];
      argument[0] = command;
      for (int i = 1; i < left->size;i++) {
         argument[i] = svec_get(left,i);
      }
      argument[left->size] = 0;
      execvp(command, argument);
      printf("%s\n", "DANGER: ran past execvp in AND function.");

   }
}
// ------------------------------------------------------------------------- //

// Helper for the OR command
void
orHelper(svec* left, svec* right) {

   int cpid;

   // FORK
   if ((cpid = fork())) {
      //** PARENT ** //

      int status;
      waitpid(cpid, &status, 0);

      // Print the status returned
      //printf("%d\n", status); //TEST CODE

      // If status is not TRUE from left side, we must evaluate right side
      if (status != 0) {
         processTokens(right);
      }

   } else {
      // ** CHILD ** //

      // EXEC
      char* command = svec_get(left, 0);
      char* argument[50];
      argument[0] = command;
      for (int i = 1; i < left->size;i++) {
         argument[i] = svec_get(left,i);
      }
      argument[left->size] = 0;
      execvp(command, argument);
      printf("%s\n", "DANGER: ran past execvp in OR function.");
   }
}

// ------------------------------------------------------------------------- //

// Helper for the & command
// Puts the given cmd into background process
void
backgroundHelper(svec* vector) {

   int cpid;

   // FORK
   if ((cpid = fork())) {
      //** PARENT ** //

      // Don't put anything here!

   } else {
      // ** CHILD ** //

      // EXEC
      char* command = svec_get(vector, 0);
      char* argument[50];
      argument[0] = command;
      // Capture the 2nd through second to last tokens as arguments
      for (int i = 1; i < vector->size - 1;i++) {
         argument[i] = svec_get(vector,i);
      }
      argument[vector->size] = 0;
      execvp(command, argument);
      printf("%s\n", "DANGER: ran past execvp in backgrounding function.");
   }

}


// ------------------------------------------------------------------------- //

// Divides up an svec into two svecs split at the index
// store left and right sides into arguments as appropriate
void
splitInTwo(svec* original, svec* left, svec* right, int index) {
   for (int i = 0; i < original->size;i++) {

      if (i < index) {
         svec_push_back(left, svec_get(original,i));
      }
      if (i > index) {
         svec_push_back(right, svec_get(original,i));
      }
   }
}

// ------------------------------------------------------------------------- //

// This function will check for pipes/redirection/other operators/etc
// And do the appropriate branching when found.
// RECURSIVELY call this function to determine how to handle special ops
void
processTokens(svec* tokens) {

   // HUNT DOWN SEMICOLONS
   // Solve each side recursively instead of in THIS interactions
   for (int index = 0; index < tokens->size;index++) {
      char* semiColonStr = ";";
      if (strcmp(svec_get(tokens, index), semiColonStr) == 0) {
         svec* left = make_svec();
         svec* right = make_svec();
         splitInTwo(tokens, left, right, index);
         processTokens(left);
         processTokens(right);
         return;
      }
   }

   // HUNT DOWN PIPES
   for (int index = 0; index < tokens->size;index++) {

      // ** Check for Pipe ** //
      char* pipeStr = "|";
      if (strcmp(svec_get(tokens, index), pipeStr) == 0) {
         // Split
         svec* left = make_svec();
         svec* right = make_svec();
         splitInTwo(tokens, left, right, index);
         //pipeHelper(left, right);
         return;
      }
   }

   // Iterate through entire tokens looking for special operators
   for (int index = 0; index < tokens->size;index++) {

      // ** Check for "exit" token ** //
      char* exitStr = "exit";
      if (strcmp(svec_get(tokens, index), exitStr) == 0) {
         // printf("%s\n", "Triggered exit"); //TEST CODE
         exit(0);
      }

      // ** Check for "cd" token ** //
      char* cd = "cd";
      if (strcmp(svec_get(tokens, index), cd) == 0) {

         char* dirToChangeTo = svec_get(tokens, index + 1);
         //printf("cd to %s\n", dirToChangeTo); //TEST CODE
         int statusCode = chdir(dirToChangeTo);
         assert(statusCode == 0);
         // assert(0);
         // printf("Status of CD: %d\n", statusCode); //TEST CODE
         return;
      }

      // ** Check for route-in ** //
      char* routInStr = "<";
      if (strcmp(svec_get(tokens, index), routInStr) == 0) {
         svec* left = make_svec();
         svec* right = make_svec();
         splitInTwo(tokens, left, right, index);
         // Defer to a helper function
         routeInHelper(left, right);
         return;
      }

      // ** Check for route-out ** //
      char* routOutStr = ">";
      if (strcmp(svec_get(tokens, index), routOutStr) == 0) {
         svec* left = make_svec();
         svec* right = make_svec();
         splitInTwo(tokens, left, right, index);
         // Defer to a helper function
         routeOutHelper(left, right);
         return;
      }

      // ** Check for AND ** //
      char* andStr = "&&";
      if (strcmp(svec_get(tokens, index), andStr) == 0) {
         svec* left = make_svec();
         svec* right = make_svec();
         splitInTwo(tokens, left, right, index);
         // Defer to a helper function
         andHelper(left, right);
         return;
      }

      // ** Check for OR ** //
      char* orStr = "||";
      if (strcmp(svec_get(tokens, index), orStr) == 0) {
         svec* left = make_svec();
         svec* right = make_svec();
         splitInTwo(tokens, left, right, index);
         // Defer to a helper function
         orHelper(left, right);
         return;
      }

      //** Check for background token (&) **//
      char* bgStr = "&";
      if (strcmp(svec_get(tokens, index), bgStr) == 0) {
         // Defer to a helper function
         backgroundHelper(tokens);
         return;
      }



   } // end of big for-loop



   // Got to the end without forking somewhere else
   // This is simple command execution with unlimited simple args
   char* command = svec_get(tokens, 0);
   char* argument[20];
   argument[0] = command;
   for (int i = 1; i < tokens->size;i++) {
      argument[i] = svec_get(tokens,i);
   }
   argument[tokens->size] = 0;
   // Send command and args to execute method
   execute(command, argument);


   // *********** TEST CODE *********
   // char* command = "echo";
   // char* args[] = {command, "JOSHUASPICER", 0};
   // execvp(command, args);


}

// ------------------------------------------------------------------------- //

// Execute the passed in {cmd} with the specified {arg}
// In a new process
// {arg}'s first element must be {cmd}, and must be null-termianted.
void
execute(char* cmd, char* arg[])
{
   int cpid;

   if ((cpid = fork())) {
      // parent process

      // *********** TEST CODE *********
      // printf("Parent pid: %d\n", getpid());
      // printf("Parent knows child pid: %d\n", cpid);

      // Child may still be running until we wait.

      int status;
      waitpid(cpid, &status, 0);


      // *********** TEST CODE *********

      // printf("== executed program complete ==\n");

      // printf("child returned with wait code %d\n", status);
      // if (WIFEXITED(status)) {
      //    printf("child exited with exit code (or main returned) %d\n", WEXITSTATUS(status));
      // }

   }
   else {
      // child process

      // *********** TEST CODE *********
      // printf("Child pid: %d\n", getpid());
      // printf("Child knows parent pid: %d\n", getppid());

      // printf("== executed program's output: ==\n");

      execvp(cmd, arg);
      printf("==== EXEC ERROR ===");
   }
}

// ------------------------------------------------------------------------- //

int
main(int argc, char* argv[])
{
   char cmd[256];
   // Place where an input is temporarily sent to before tokenized
   char temp[400];
   // Represents the fd of inputted .sh file (if there is one)
   FILE* input;

   if (argc == 1) { // NO argument upon first starting nush

      // Initial prompt print
      printf("nush$ ");

      while (fgets(cmd, 256, stdin) != NULL) {

         fflush(stdout);
         svec* tokens = tokenize(cmd);
         // // *********** TEST CODE *********
         // printf("\n\n\n\n\n\n\n\n");
         // dyArray_print_with_interrupt(tokens);
         // printf("\n\n\n\n\n\n\n\n");
         // // **********************************
         //TODO execute...
         processTokens(tokens);
         printf("nush$ ");
         free_svec(tokens);
      }

   } else { // .sh file as argument upon first starting nush

      // Open up the given file!
      input = fopen(argv[1], "r");

      while (fgets(temp, 400, input) != NULL) {
         // Tokenize this line of input
         svec* tokens = tokenize(temp);
         // // *********** TEST CODE *********
         // printf("\n\n\n\n");
         // dyArray_print_with_interrupt(tokens);
         // printf("\n\n\n\n");
         // // **********************************
         // Execute
         processTokens(tokens);
         // Free up tokens svec for next line (if there is one)
         free_svec(tokens);
      }
   }

   // fclose(input);

   return 0;
}
