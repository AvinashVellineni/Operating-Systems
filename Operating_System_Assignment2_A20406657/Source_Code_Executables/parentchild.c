#include "types.h"
#include "user.h"

int 
main(void)
{
  int childprocess = fork();
  if(childprocess<0){
    printf(1,"Fork Failed  with value from fork call %d\n",childprocess);
  }
  else if(childprocess>0){
    printf(1,"I am the parent with parent pid %d and child pid %d\n",getpid(),childprocess);
    wait();
  }
  else{
    printf(1,"I am the child with child pid %d\n",getpid());
  }
  exit();
}

