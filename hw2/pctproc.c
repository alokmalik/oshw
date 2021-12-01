#include "project_header.h"

DONUT_SHOP shared_ring;
pthread_mutex_t prod [NUMFLAVORS];
pthread_mutex_t cons [NUMFLAVORS];
pthread_cond_t prod_cond [NUMFLAVORS];
pthread_cond_t cons_cond [NUMFLAVORS];
pthread_t thread_id [NUMCONSUMERS+NUMPRODUCERS];
pthread_t sig_wait_id;
// pthread_cond_wait(pthread_cond_t *condx, pthread_mutex_t *mlk);
// pthread_cond_signal(pthread_cond_t *condx);

int main ( int argc, char *argv[] )

{
  int i, j, k, nsigs;
  struct timeval randtime, first_time, last_time;
  struct sigaction new_act;
  int arg_array[NUMCONSUMERS];
  sigset_t all_signals;
  int sigs[] = { SIGBUS, SIGSEGV, SIGFPE };
  pthread_attr_t thread_attr;
  struct sched_param sched_struct;
  unsigned int cpu;
  int proc_cnt=0;
  int proc_cntx, cn;
  float etime;
  ushort xsub1[3];
  cpu_set_t mask;
  char msg[300];
  //initial time to measure performance
  gettimeofday (&randtime,(struct timezone*)0);
  xsub1[0]=(ushort)randtime.tv_usec;
  xsub1[1]=(ushort)(randtime.tv_usec>>16);
  xsub1[0]=(ushort)(syscall(SYS_gettid));

  //number of available processors
  proc_cnt = get_nprocs();
  //puts affinity mask of a process in mask
  sched_getaffinity(0, sizeof(cpu_set_t), &mask);

  printf("\n This run will have\n Producers = %d\n Consumers=%d\n QueueDepth = %d\n Number Of Dozens = %d\n Flavor=%d\n Thread Scope =%s\n Number of CPUs=%d\n",NUMPRODUCERS,NUMCONSUMERS,NUMSLOTS,NUM_DOZ_TO_CONS,NUMFLAVORS,"Process",proc_cnt);

  printf("\n Process Affinity Mask Before Adjustment:\n");
  printf(" CPUs : 0");
  for(i=1; i<proc_cnt;++i)printf(" %d ",i);
  printf("\n       %d ",(CPU_ISSET(0,&mask))?1:0);
  for(i=1; i<proc_cnt;++i)printf(" %d ",(CPU_ISSET(i,&mask))?1:0);
  printf("\n");

  CPU_ZERO(&mask);
  CPU_SET((nrand48(xsub1))%proc_cnt, &mask);
  sched_setaffinity(0, sizeof(cpu_set_t), &mask);

  printf("\n Process Affinity Mask After Adjustment:\n");
  sched_getaffinity(syscall(SYS_gettid),sizeof(cpu_set_t),&mask);
  printf(" CPUs : 0");
  for(i=1; i<proc_cnt;++i)printf(" %d ",i);
  printf("\n       %d ",(CPU_ISSET(0,&mask))?1:0);
  for(i=1; i<proc_cnt;++i)printf(" %d ",(CPU_ISSET(i,&mask))?1:0);
  printf("\n\n");


  gettimeofday (&first_time, (struct timezone *) 0 );

  for ( i = 0; i < NUMCONSUMERS ; i++ )
  {
    arg_array [i] = i + 1;
  }

  for ( i = 0; i < NUMFLAVORS; i++ )
  {
    pthread_mutex_init ( &prod [i], NULL );
    pthread_mutex_init ( &cons [i], NULL );
    pthread_cond_init ( &prod_cond [i], NULL );
    pthread_cond_init ( &cons_cond [i], NULL );
    shared_ring.outptr [i] = 0;
    shared_ring.in_ptr [i] = 0;
    shared_ring.serial [i] = 1;
    shared_ring.spaces [i] = NUMSLOTS;
    shared_ring.donuts [i] = 0;
  }


  //fill a signal set to block all signals
  sigfillset (&all_signals );
  nsigs = sizeof ( sigs ) / sizeof ( int );
  for ( i = 0; i < nsigs; i++ )
    sigdelset ( &all_signals, sigs [i] );
  sigprocmask ( SIG_BLOCK, &all_signals, NULL );
  sigfillset (&all_signals );
  for( i = 0; i < nsigs; i++ )
  {
      new_act.sa_handler = sig_handler;
      new_act.sa_mask = all_signals;
      new_act.sa_flags = 0;
      if ( sigaction (sigs[i], &new_act, NULL) == -1 )
      {
        perror("can't set signals: ");
        exit(1);
      }
  }
  printf("All signals set to block \n");
  //signal handler
  if ( pthread_create (&sig_wait_id, NULL, sig_waiter, NULL) != 0 )
  {
    printf ( "pthread_create failed " );
    exit ( 3 );
  }

  pthread_attr_init ( &thread_attr );
  pthread_attr_setinheritsched ( &thread_attr, PTHREAD_INHERIT_SCHED );


  //printf("Creating Consumer threads \n");
  for ( i = 0; i < NUMCONSUMERS ; i++ )
  {
    if ( pthread_create ( &thread_id [i], &thread_attr, consumer, ( void * )&arg_array [i]) != 0 )
    {
      printf ( "pthread_create failed" );
      exit ( 3 );
    }
  }
  printf("Consumer threads created \n");
  printf("Creating Producer Threads \n");
  for ( ; i < NUMPRODUCERS + NUMCONSUMERS; i++ )
  {
    if ( pthread_create (&thread_id[i], &thread_attr, producer, NULL ) != 0 )
    {
      printf ( "pthread_create failed " );
      exit ( 3 );
    }
  }

  printf("Producer threads created \n");

  printf("starting all threads ");
  //usleep(10000000);
  for ( i = 0; i < NUMCONSUMERS; i++ ){
    pthread_join ( thread_id [i], NULL );
    printf(" %d",i);
  }
  printf("\n");
  gettimeofday(&last_time, (struct timezone *)0);
  if((i=last_time.tv_sec - first_time.tv_sec) == 0)
    j=last_time.tv_usec - first_time.tv_usec;
  else
  {
    if(last_time.tv_usec - first_time.tv_usec < 0)
    {
      i--;
      j = 1000000 + (last_time.tv_usec - first_time.tv_usec);
    }
    else
    {
      j=last_time.tv_usec - first_time.tv_usec;
    }
  }
  etime =i + (float)j/1000000;
  printf("\n\nElapsed consumer time is %d sec and %d usec,or %f sec\n", i, j,etime );
  if((cn = open("./process_times",O_WRONLY|O_CREAT|O_APPEND, 0644)) == -1)
  {
    perror("can not open sys time file ");
    exit(1);
  }
  sprintf(msg, "\n\nProcess_Scope: Elapsed consumer time is %f sec\n", etime);
  write(cn, msg, strlen(msg));
  printf("\nAll consumers are finished\n\n");
  exit(0);
}


void *producer ( void *arg )
{
  int i, j, k;
  unsigned short xsub1 [3];
  struct timeval randtime;
  gettimeofday ( &randtime, ( struct timezone * ) 0 );
  xsub1 [0] = ( ushort ) randtime.tv_usec;
  xsub1 [1] = ( ushort ) ( randtime.tv_usec >> 16 );
  xsub1 [2] = ( ushort ) ( pthread_self ());
  while ( 1 )
  {
    //select flavour
    j = nrand48 ( xsub1 ) & 3;
    pthread_mutex_lock ( &prod [j] );
    //wait if consumer or producer has the lock
    while ( shared_ring.spaces [j] == 0 )
     {
       pthread_cond_wait ( &prod_cond [j], &prod [j] );
     }
      // safe to manipulate in_ptr and serial
      shared_ring.flavor[j][shared_ring.in_ptr[j]]=shared_ring.serial[j];
      shared_ring.in_ptr[j] = (shared_ring.in_ptr[j]+1) % NUMSLOTS;
      shared_ring.serial[j]++;

      // space counter for flavor j
        shared_ring.spaces[j]--;
    //get lock for producer of flavour j
     pthread_mutex_unlock ( &prod [j] );

    //get lock for consumer of flavour j
    pthread_mutex_lock( &cons[j]);
    //produce the donut
    shared_ring.donuts[j]++;
    //release locks for consumer and producer
    pthread_mutex_unlock(&cons[j]);
    pthread_cond_signal(&cons_cond[j]);
  }
  usleep(100);
  return NULL;
}

void *consumer ( void *arg )
{
  int i, j, k, m,value, id,len;
  char msg[100],lbuf[5];
  unsigned short xsub [3];
  struct timeval randtime;
  FILE  *cn;
  unsigned cpu;
  char number[2] = {'\060','\n'};
  char file_name[10]="cons";
  char thread_number[5];
  unsigned int cpusetsize;
  cpu_set_t mask;
  int doz[12][4];
  int d[4], n;
  time_t t;
  struct tm * tp;
  float ms;
  int ims;

  gettimeofday ( &randtime, ( struct timezone * ) 0 );
  xsub [0] = ( ushort ) randtime.tv_usec;
  xsub [1] = ( ushort ) ( randtime.tv_usec >> 16 );
  xsub [2] = ( ushort ) (syscall(SYS_gettid) );

  #ifdef DEBUG
   itoa(pthread_self(),thread_number);
   strcat(file_name,thread_number);
   if((cn=fopen(file_name,"w+")) == -1)
   {
     perror("failed to open consumer log");
   }
   #endif
  //fileoutput
  id = *( int * ) arg;
  if(id%5==0)
  {
    sprintf(file_name,"cons%d",id);
    if((cn = fopen(file_name,"w+"))==NULL)
    {
      perror("failed to open consumer output file");
    }
  }
  sprintf(lbuf,"%d",id);




  for( i = 0; i < NUM_DOZ_TO_CONS; i++ )
  {

    t=time(NULL);
    tp = localtime(&t);
    gettimeofday(&randtime, NULL);
    ms = (float)randtime.tv_usec/1000000;
    ims = (int)(ms*1000);
    for( k = 0; k < 12; k++ )
    {
      j = nrand48( xsub ) & 3;
      
      pthread_mutex_lock ( &cons[j] );
      while ( shared_ring.donuts [j] == 0 )
       {
         pthread_cond_wait ( &cons_cond [j], &cons [j] );
       }
       value=shared_ring.flavor[j][shared_ring.outptr[j]];
       shared_ring.outptr[j] =(shared_ring.outptr[j]+1) % NUMSLOTS;
       shared_ring.donuts[j]--;

        pthread_mutex_unlock ( &cons [j] );
        pthread_mutex_lock( &prod[j]);
        shared_ring.spaces[j]++;
        pthread_mutex_unlock(&prod[j]);
        pthread_cond_signal(&prod_cond[j]);


    //    printf("donut type %s , \tserial number %d \n",dtype[j],value);

    } // end getting one dozen
    
      usleep(100);
  } // end getting 20000 dozen
  return NULL;
}

void *sig_waiter ( void *arg )
{
  sigset_t sigterm_signal;
  int signo;
  sigemptyset ( &sigterm_signal );
  sigaddset ( &sigterm_signal, SIGTERM );
  sigaddset ( &sigterm_signal, SIGINT );
  /* set for asynch signal management for SIGs 2 and 15 */
  if (sigwait ( &sigterm_signal, & signo) != 0 )
  {
    printf ( "\n sigwait ( ) failed, exiting \n");
    exit(2);
  }
  printf ( "Process exits on SIGNAL %d \n\n", signo );
  exit ( 1 );
  return NULL; /* not reachable */
}

void sig_handler ( int sig )
{
  pthread_t signaled_thread_id;
  int i, thread_index;
  signaled_thread_id = pthread_self ( );
  /******* check for own ID in array of thread Ids *******/
  for ( i = 0; i < ( NUMCONSUMERS ); i++)
  {
    if ( signaled_thread_id == thread_id [i] )
    {
      thread_index = i + 1;
      break;
    }
  }
  printf ( "\nThread %d took signal # %d, PROCESS HALT\n",
  thread_index, sig );
  exit ( 1 );
}
