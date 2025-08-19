// init: The initial user-level program
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#ifndef USERNAME
#define USERNAME "user"
#endif

#ifndef PASSWORD
#define PASSWORD "pass"
#endif

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;
  char username[32];
  char password[32];
  int attempts = 0;
  int loginSuccess = 0;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  // --- Login system start ---
  while(attempts < 3 && !loginSuccess){
    printf(1, "Enter Username: ");
    gets(username, sizeof(username));
    // Remove trailing newline if any.
    if(username[strlen(username)-1] == '\n')
      username[strlen(username)-1] = '\0';
      
    if(strcmp(username, USERNAME) == 0){
      printf(1, "Enter Password: ");
      gets(password, sizeof(password));
      if(password[strlen(password)-1] == '\n')
        password[strlen(password)-1] = '\0';
        
      if(strcmp(password, PASSWORD) == 0){
        printf(1, "Login successful\n");
        loginSuccess = 1;
        break;
      } else {
        printf(1, "Incorrect password\n");
      }
    } else {
      printf(1, "Incorrect username\n");
    }
    attempts++;
  }
  
  if(!loginSuccess){
    printf(1, "Maximum login attempts reached. Disabling login system.\n");
    while(1)
      sleep(100);
  }
  // --- Login system end ---

  for(;;){
    printf(1, "init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}
