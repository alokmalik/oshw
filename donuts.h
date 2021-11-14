#ifndef DONUTS_H_INCLUDED
#define DONUTS_H_INCLUDED

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <unistd.h>

#define		SEMKEY			(key_t)584335604
#define		MEMKEY			(key_t)587231454
#define		NUMFLAVORS	 	4
#define		NUMSLOTS       	100
#define		NUMSEMIDS	 	3
#define		NUMDZ		 	50
#define		PROD		 	0
#define		CONSUMER	 	1
#define		OUTPTR		 	2

struct	donut_ring{
	int	flavor  [NUMFLAVORS]  [NUMSLOTS];
	int	outptr  [NUMFLAVORS];
};

extern int	p(int, int);
extern int	v(int, int);
extern int	semsetall(int, int, int);

#endif // DONUTS_H_INCLUDED
