#include <string.h>
#include "donuts.h"
int shmid, semid[3];
int main(int argc, char *argv[])
{
/*** locals for the consumer ***/
	int	i,j,k,donut;
	struct donut_ring *shared_ring;
        struct timeval          randtime;
        unsigned short          xsub1[3];
	char *dtype[] = {"jelly" , "coconut" , "plain" , "glazed"};
        int doz[12][4];
        int d[4], m, n;
        time_t t;
        struct tm * tp;
        float ms;
        int ims;
/*** get shmid and map shared memory ***/
	if((shmid=shmget(MEMKEY, sizeof(struct donut_ring),
		0)) == -1){ // note 0 flag field
		perror("shared get failed: ");
		exit(1);
	}
	if((shared_ring=(struct donut_ring *)shmat(shmid, NULL, 0)) ==(void *) -1){
		perror("shared attach failed: ");
		exit(1);
	}
/*** get all 3 semids also with 0 flag field ***/
	for(i=0; i<NUMSEMIDS; i++){
		if((semid[i]=semget(SEMKEY+i, NUMFLAVORS, 
			0)) == -1){
		perror("semaphore allocation failed: ");
		exit(1);
		}
	}

/*** initialize nrand48() random generator ***/

	gettimeofday(&randtime, NULL);

	xsub1[0] = (ushort)randtime.tv_usec;
	xsub1[1] = (ushort)(randtime.tv_usec >> 16);
	xsub1[2] = (ushort)(getpid());
/*** get NUMDZ dozens and write out content ***/
	//to fill//
	for(i=0; i<NUMDZ; i++){ // each dozen setup

		for(m=0; m<12; ++m)
	  	for(n=0; n<4; ++n) doz[m][n]=0;
	 	for(n=0; n<4; ++n) d[n]=0;
	//to fill//


	t=time(NULL);
	tp = localtime(&t);
	gettimeofday(&randtime, NULL);

	ms = (float)randtime.tv_usec/1000000;

	ims = (int)(ms*1000);

	printf("consumer PID %d   time: %d:%d:%d.%d   dozen number: %d  \n\n",
	        getpid(), tp->tm_hour, tp->tm_min, tp->tm_sec, ims, i+1);

	for(k=0; k<12; k++){ // each individual donut processing
		j=nrand48(xsub1) & 3;
		
		if( p(semid[CONSUMER], j) == -1){
						perror("p operation failed: ");
						exit(3);
				}
		if( p(semid[OUTPTR], j) == -1){
                        perror("p operation failed: ");
                        exit(3);
                }
		donut=shared_ring->flavor[j][shared_ring->outptr[j]];
	  	shared_ring->outptr[j] = 
			(shared_ring->outptr[j]+1) % NUMSLOTS;

		if( v(semid[PROD], j) == -1){
                        perror("v operation failed: ");
                        exit(3);
                }


               if( v(semid[OUTPTR], j) == -1){
                        perror("v operation failed: ");
                        exit(3);
                }




		doz[d[j]][j]=donut;
		d[j] = d[j]+1;

	} // write this dozen out
	printf("Jelly\t\tCoconut\t\tPlain\t\tGlazed\n");

	for(m=0; m<12; ++m){
		(doz[m][0] == 0)?printf("\t\t"):printf("%d\t\t",doz[m][0]);
		(doz[m][1] == 0)?printf("\t\t"):printf("%d\t\t",doz[m][1]);
		(doz[m][2] == 0)?printf("\t\t"):printf("%d\t\t",doz[m][2]);
		(doz[m][3] == 0)?printf("\t\t\n"):printf("%d\n",doz[m][3]);
		if((doz[m][0] == 0) && (doz[m][1] == 0) && 
                   (doz[m][2] == 0) && (doz[m][3] == 0))break;
	  }
	usleep(10000); // sleep 10 ms to force context switch
	
	} // all dozens complete
	fprintf(stderr, "CONSUMER %s DONE\n", argv[1]);
	return 0;
} // consumer complete
