#include <string.h>
#include "donuts.h"
int shmid, semid[3];
int main(int argc, char *argv[])
{
/*** locals for the consumer ***/
/*** get shmid and map shared memory ***/
 if((shmid=shmget(MEMKEY, sizeof(struct donut_ring),
 0)) == -1){ // note 0 flag field
 perror("shared get failed: ");
 exit(1);
 }
/*** get all 3 semids also with 0 flag field ***/
/*** initialize nrand48() random generator ***/
/*** get NUMDZ dozens and write out content ***/
 for(i=0; i<NUMDZ; i++){ // each dozen setup
……
for(k=0; k<12; k++){ // each individual donut processing
 ……
 } // write this dozen out
usleep(10000); // sleep 10 ms to force context switch
 …….
 } // all dozens complete
 …….
} // consumer complete