/*********  message queue client  ***********/

#include <sys/types.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>

#define MSGKEY  (1234567)

typedef struct{
  long mtype;
  char mtext[252];
} MSG;


int main(int argc, char * argv[]){
  struct timeval randtime;
  ushort  xsub1[3];
  MSG amessage;
  long msg_type;
  pid_t *pid_ptr, pid;
  int  msg_id;

  gettimeofday(&randtime, (struct timezone *)0);
  xsub1[0] = (ushort)randtime.tv_usec;
  xsub1[1] = (ushort)(randtime.tv_usec >> 16);
  xsub1[2] = (ushort)(getpid());


  msg_type = (long)((nrand48(xsub1) % 10) + 1);
  printf("CLIENT: client sending message type: %lld\n", msg_type);

  if((msg_id = msgget((key_t)MSGKEY, 0)) == -1){
     perror("msgget failed :");
     exit(1);
  }
  pid = getpid();
  pid_ptr = (pid_t *)amessage.mtext;
  *pid_ptr = pid;
  sprintf(( amessage.mtext + sizeof(pid_t)), "This is message from child with PID: %d\n\0", pid);

  amessage.mtype = msg_type;
  if(msgsnd(msg_id, &amessage,
            sizeof(amessage.mtext), 0) == -1){
     perror("msgsnd failed :");
     exit(1);
  }
  msg_type = (long)pid;
  if(msgrcv(msg_id, &amessage,
            sizeof(amessage.mtext), msg_type, 0) == -1){

     perror("msgrcv failed :");
     exit(1);
  }
  printf("CLIENT: Server message is %s\n", amessage.mtext);
  printf("CLIENT: Client with pid  %d  is finished\n", pid);
}
