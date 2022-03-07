
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>

#define N 10
#define FREE 0
#define INIT 1
#define STACKSIZE (10000)

typedef struct state_block
{
    int state;
    void (*function)(int);
    int function_arg;
    ucontext_t run_env;
    ucontext_t rtn_env;
    stack_t mystk;
}STATE_BLOCK;

STATE_BLOCK t_state[N];
int enter_flag = 0;
char interrupt_string2[] = "\tinterrupt on context \n";
char interrupt_string1[] = "\tcall in from context \n";
int current = 0;
sigset_t maskval;
int bx[10];
void fun1(int), fun2(int), fun3(int), clock_isr(int); // functions are started as threads
int t_init(void (*)(), int);

int main(int argc, char *argv[])
{
 int i,j,k;
 struct itimerval quantum;
 struct sigaction sigdat;
 for(i=0; i<N; i++){
 t_state[i].state = FREE;
 }
 sigemptyset(&maskval);
 sigdat.sa_handler = clock_isr;
 sigemptyset(&sigdat.sa_mask);
 sigdat.sa_flags = SA_RESTART;
 if(sigaction(SIGVTALRM, &sigdat, (struct sigaction *)0) == -1){
 perror("signal set error");
 exit(2);
 }
 if((i = t_init(fun1, 1)) == -1){
 printf("thread failed to initialize\n");
 exit(7);
 }
 if((j = t_init(fun2, 2)) == -1){
 printf("thread failed to initialize\n");
 exit(7);
 }

 if((k = t_init(fun3, 3)) == -1){
 printf("thread failed to initialize\n");
 exit(7);
 }
 printf("thread IDs are %d %d %d\n", i,j,k);
 quantum.it_interval.tv_sec = 0;
 quantum.it_interval.tv_usec = 500000;
 quantum.it_value.tv_sec = 0;
 quantum.it_value.tv_usec = 500000;
 current = 1;
 if(setitimer(ITIMER_VIRTUAL, &quantum,
 (struct itimerval *)0) == -1){
 perror("setitimer");
 exit(3);
 }
 if(setcontext(&(t_state[current].run_env)) == -1){
 perror("setcontext");
 exit(5);
 }
 while(1);
 return 0;
} /****** End main() ******/
int t_init (void (*fun_addr)(), int fun_arg)
{
 int i,j,k;
 for (i=1; i<N; i++) {
 if (t_state[i].state == FREE) break;
 }
 if (i == N) return(-1);
 if(getcontext(&t_state[i].run_env) == -1){
 perror("getcontext in t_init");
 exit(5);
 }
 t_state[i].state = INIT;
 t_state[i].function = fun_addr;
 t_state[i].mystk.ss_sp = (void *)(malloc(STACKSIZE));
 if(t_state[i].mystk.ss_sp == NULL){
 printf("thread failed to get stack space\n");
 exit(8);
 }
 t_state[i].mystk.ss_size = STACKSIZE;
 t_state[i].mystk.ss_flags = 0;
 t_state[i].run_env.uc_stack = t_state[i].mystk;
 makecontext(&t_state[i].run_env, fun_addr, 1,fun_arg);
 return(i);
} /****** End t_init() ******/
void clock_isr(int sig)
{
 int i,j,k, old;
 interrupt_string1[23] = (char)(current + 48);
 interrupt_string2[23] = (char)(current + 48);
 if(sig < 0)write(1, interrupt_string1, 27);
 else write(1, interrupt_string2, 27);
 if (t_state[current].state == INIT) {
 printf ("\t\t\t\t\tthread %d has b = %d\n", current, bx[current]);
 for(i=1; i<(N+1); i++){
 if (t_state[j=(current+i)%N].state == INIT) {
 old = current;
 current = j;
 sigprocmask (SIG_SETMASK, &maskval, NULL);
 swapcontext (&(t_state[old].run_env), &(t_state[current].run_env));
 return;
 }
 }
 return;
 }else{
 printf ("***** Thread %d has FINISHED\n", current);
 for(i=1; i<(N+1); i++){
 if (t_state[j=(current+i)%N].state == INIT) {
 current = j;
 sigprocmask (SIG_SETMASK, &maskval, NULL);
 setcontext (&(t_state[current].run_env));
 }
 }
 printf("no more threads left, EXITING FROM clock_isr NOW\n\n");
 exit(0);
 }
 return;
} /****** End clock_isr ******/


void fun1 (int global_index)
{
 volatile int a=0, b=0;
 for (a=0; a<10; ++a) {
 printf ("\t from fun1 a = %d\tb = %d\n", a, b);
 write (1,"1",1);
 for (b=0, bx[global_index]=0; b<25000000; ++b,++bx[global_index]);
 }
 t_state[current].state = FREE;   //change these three lines of code 
 clock_isr(-current);
 return;
}


void fun2 (int global_index)
{
 volatile int a=0, b=0;
 for (a=0; a<10; ++a) {
 printf ("\t from fun2 a = %d\tb = %d\n", a, b);
 write (1,"2",1);
 for (b=0, bx[global_index]=0; b<18000000; ++b,++bx[global_index]);
 }
 t_state[current].state = FREE;
 clock_isr(-current);
 return;
}


void fun3 (int global_index)
{
 volatile int a=0, b=0;
 for (a=0; a<10; ++a) {
 printf ("\t from fun3 a = %d\tb = %d\n", a, b);
 write (1,"3",1);
 for (b=0, bx[global_index]=0; b<40000000; ++b,++bx[global_index]);
 }
 t_state[current].state = FREE;
 clock_isr(-current);
 return;
}
