/*    node controller.c - a Lamport algorithm node controller      */
/*    USAGE:  node controller my_role<1-n> node_cnt<n>             */
/*            where role n does all connect and role 1 all accepts */


#include "buf_mgr.h"
#include "ddonuts.h"

#include <unistd.h>
#include <pthread.h>

#define  MAXNODE	16




void send_req(MSG * msg,LAMPORT * lamport,int connected_ch[]);
void send_reply(MSG * msg,LAMPORT * lamport,int sock);
void send_release(MSG * msg,LAMPORT * lamport,int connected_ch[]);
void add_request(LAMPORT * lamport,int node_id,int clock);
void add_reply(LAMPORT * lamport,int node_id,int clock);
void add_release(LAMPORT * lamport,int node_id,int clock);
void lamport_print(LAMPORT * lamport);
int lamport_check_cs(LAMPORT * lamport);





char	*host_list[] = {"host list", "cs91515-1", "cs91515-2", "cs91515-3", "cs91515-4", "cs91515-5", "cs91515-6"}; 

typedef struct thread_arg{
  int my_chan;
  int my_node_id;
}TH_ARG;

int connected_ch[MAXNODE];

void *chan_monitor(void *);
void *msg_maker(void *);

int main(int argc, char *argv[])
{
  MSG     msg;
	MBUF		raw;
	DONUT		donut;
  int    	inet_sock, new_sock, out_index, my_role, node_cnt;
	int			my_chan, type_val, id_val, read_val, trigger;
  int     i, j, k, fromlen, nsigs, donut_num, node_id;
	char    *buffer_ptr;
  struct sockaddr_in 	inet_telnum;
	struct hostent 		*heptr, *gethostbyname();
	int			wild_card = INADDR_ANY;
	struct sigaction 	sigstrc;
	sigset_t	mask;
  struct donut_ring       *shared_ring;
  struct timeval          randtime;
  unsigned short          xsub1[3];
	TH_ARG 			*th_arg;
	pthread_t		thread_ids[MAXNODE];
	int			connect_cnt = 0, th_index = 0;
	

	my_role = atoi(argv[1]);
	node_cnt = atoi(argv[2]);

  /*8socket() creates an endpoint for communication and returns a file
  descriptor that refers to that endpoint.  The file descriptor
  returned by a successful call will be the lowest-numbered file
  descriptor not currently open for the process.*/

// Unless I'm node n of n nodes, I need to do at least 1 accept

	if(node_cnt - my_role > 0){
    if((inet_sock=socket(AF_INET, SOCK_STREAM, 0)) == -1){
      perror("inet_sock allocation failed: ");
      exit(2);
  }

/***** byte copy the wild_card IP address INADDR_ANY into   *****/
/***** IP address structure, along with port and family and *****/
/***** use the structure to give yourself a connect address *****/

  bcopy(&wild_card, &inet_telnum.sin_addr, sizeof(int));
  inet_telnum.sin_family = AF_INET;
  inet_telnum.sin_port = htons( (u_short)PORT );

  if(bind(inet_sock, (struct sockaddr *)&inet_telnum, 
    sizeof(struct sockaddr_in)) == -1){
    perror("inet_sock bind failed: ");
    exit(2);
  }

/***** allow client connect requests to arrive: call-wait 5 *****/

          listen(inet_sock, 5);

/***** set sizeof(struct sockaddr) into fromlen to specify  *****/
/***** original buffer size for returned address (the       *****/
/***** actual size of the returned address then goes here)  *****/

	fromlen = sizeof(struct sockaddr);

// Accept a connection based on role ... if role is 3 and node_cnt is 5
// then perform 2 accepts

	for(i=0; i<(node_cnt - my_role); ++i){

// Accept connection and spawn listener thread

         while((connected_ch[i] = accept(inet_sock, (struct sockaddr *)&inet_telnum, 
                                          &fromlen)) == -1 && errno == EINTR);
         if(connected_ch[i] == -1){
		perror("accept failed: ");
		exit(2);
	 }
	 ++connect_cnt;

	 th_arg = malloc(sizeof(TH_ARG));
	 th_arg->my_chan = connected_ch[i];
	 th_arg->my_node_id = my_role;

	 if((errno = pthread_create(&thread_ids[th_index++], NULL, 
                          chan_monitor, (void *)th_arg)) != 0){
		perror("pthread_create channel monitor failed ");
		exit(3);
         }
	}
	} // some accepts required

  //4 cases
	switch(my_role){
		case 1:
	  break; //all connected
		
    case 2:
      // need 1 connect, access hostname array for connect target
	    if((heptr = gethostbyname( host_list[1] )) == NULL){
        perror("gethostbyname failed: ");
        exit(1);
      }
	    if((connected_ch[connect_cnt] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("connect channel allocation failed: ");
        exit(2);
      }

      bcopy(heptr->h_addr, &inet_telnum.sin_addr, heptr->h_length);
      inet_telnum.sin_family = AF_INET;
      inet_telnum.sin_port = htons( (u_short)PORT );

      // connect to target and spawn listener thread

	    if(connect(connected_ch[connect_cnt], (struct sockaddr *)&inet_telnum,
        sizeof(struct sockaddr_in)) == -1){
        perror("inet_sock connect failed: ");
        exit(2);
      }
      th_arg = malloc(sizeof(TH_ARG));
      th_arg->my_chan = connected_ch[connect_cnt];
      th_arg->my_node_id = my_chan = my_role;
      //printf("\nconnected from %s to %s\n", host_list[my_role], host_list[1]);

	    if((errno = pthread_create(&thread_ids[th_index++], NULL,
                chan_monitor, (void *)th_arg)) != 0){
        perror("pthread_create channel monitor failed ");
        exit(3);
      }
	    break; // accepts and 1 connect
		
    case 3:

      // need 2 connects, access hostname array for connect targets

      if((heptr = gethostbyname( host_list[1] )) == NULL){
        perror("gethostbyname failed: ");
        exit(1);
      }
      if((connected_ch[connect_cnt] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("connect channel allocation failed: ");
        exit(2);
      }

      bcopy(heptr->h_addr, &inet_telnum.sin_addr, heptr->h_length);
      inet_telnum.sin_family = AF_INET;
      inet_telnum.sin_port = htons( (u_short)PORT );

      if(connect(connected_ch[connect_cnt], (struct sockaddr *)&inet_telnum,
                          sizeof(struct sockaddr_in)) == -1){
        perror("inet_sock connect failed: ");
        exit(2);
      }
      th_arg = malloc(sizeof(TH_ARG));
      th_arg->my_chan = connected_ch[connect_cnt];
      th_arg->my_node_id = my_role;
  //	   printf("\nconnected from %s to %s\n", host_list[my_role], host_list[1]);

      if((errno = pthread_create(&thread_ids[th_index++], NULL,
                    chan_monitor, (void *)th_arg)) != 0){
            perror("pthread_create channel monitor failed ");
            exit(3);
      }
	   ++connect_cnt;

      if((heptr = gethostbyname( host_list[2] )) == NULL){
        perror("gethostbyname failed: ");
        exit(1);
      }
      if((connected_ch[connect_cnt] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("connect channel allocation failed: ");
        exit(2);
      }

      bcopy(heptr->h_addr, &inet_telnum.sin_addr, heptr->h_length);
      inet_telnum.sin_family = AF_INET;
      inet_telnum.sin_port = htons( (u_short)PORT );

      if(connect(connected_ch[connect_cnt], (struct sockaddr *)&inet_telnum,
                          sizeof(struct sockaddr_in)) == -1){
        perror("inet_sock connect failed: ");
        exit(2);
      }
      th_arg = malloc(sizeof(TH_ARG));
      th_arg->my_chan = connected_ch[connect_cnt];
      th_arg->my_node_id = my_role;
//	   printf("\nconnected from %s to %s\n", host_list[my_role], host_list[2]);

      if((errno = pthread_create(&thread_ids[th_index++], NULL,
                    chan_monitor, (void *)th_arg)) != 0){
            perror("pthread_create channel monitor failed ");
            exit(3);
      }
      break; // accepts and 2 connects
		case 4:

// need 3 connects, access hostname array for connect targets

           if((heptr = gethostbyname( host_list[1] )) == NULL){
             perror("gethostbyname failed: ");
             exit(1);
           }
           if((connected_ch[connect_cnt] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
              perror("connect channel allocation failed: ");
              exit(2);
           }

           bcopy(heptr->h_addr, &inet_telnum.sin_addr, heptr->h_length);
           inet_telnum.sin_family = AF_INET;
           inet_telnum.sin_port = htons( (u_short)PORT );

           if(connect(connected_ch[connect_cnt], (struct sockaddr *)&inet_telnum,
                               sizeof(struct sockaddr_in)) == -1){
             perror("inet_sock connect failed: ");
             exit(2);
           }
           th_arg = malloc(sizeof(TH_ARG));
           th_arg->my_chan = connected_ch[connect_cnt];
           th_arg->my_node_id = my_role;
//	   printf("\nconnected from %s to %s\n", host_list[my_role], host_list[1]);

           if((errno = pthread_create(&thread_ids[th_index++], NULL,
                          chan_monitor, (void *)th_arg)) != 0){
                 perror("pthread_create channel monitor failed ");
                 exit(3);
           }
           ++connect_cnt;

           if((heptr = gethostbyname( host_list[2] )) == NULL){
             perror("gethostbyname failed: ");
             exit(1);
           }
           if((connected_ch[connect_cnt] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
              perror("connect channel allocation failed: ");
              exit(2);
           }

           bcopy(heptr->h_addr, &inet_telnum.sin_addr, heptr->h_length);
           inet_telnum.sin_family = AF_INET;
           inet_telnum.sin_port = htons( (u_short)PORT );

           if(connect(connected_ch[connect_cnt], (struct sockaddr *)&inet_telnum,
                               sizeof(struct sockaddr_in)) == -1){
             perror("inet_sock connect failed: ");
             exit(2);
           }
           th_arg = malloc(sizeof(TH_ARG));
           th_arg->my_chan = connected_ch[connect_cnt];
           th_arg->my_node_id = my_role;
//	   printf("\nconnected from %s to %s\n", host_list[my_role], host_list[2]);

           if((errno = pthread_create(&thread_ids[th_index++], NULL,
                          chan_monitor, (void *)th_arg)) != 0){
                 perror("pthread_create channel monitor failed ");
                 exit(3);
           }
           ++connect_cnt;

           if((heptr = gethostbyname( host_list[3] )) == NULL){
             perror("gethostbyname failed: ");
             exit(1);
           }
           if((connected_ch[connect_cnt] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
              perror("connect channel allocation failed: ");
              exit(2);
           }

           bcopy(heptr->h_addr, &inet_telnum.sin_addr, heptr->h_length);
           inet_telnum.sin_family = AF_INET;
           inet_telnum.sin_port = htons( (u_short)PORT );

           if(connect(connected_ch[connect_cnt], (struct sockaddr *)&inet_telnum,
                               sizeof(struct sockaddr_in)) == -1){
             perror("inet_sock connect failed: ");
             exit(2);
           }
           th_arg = malloc(sizeof(TH_ARG));
           th_arg->my_chan = connected_ch[connect_cnt];
           th_arg->my_node_id = my_role;
          //printf("\nconnected from %s to %s\n", host_list[my_role], host_list[3]);

          if((errno = pthread_create(&thread_ids[th_index++], NULL,
                          chan_monitor, (void *)th_arg)) != 0){
                 perror("pthread_create channel monitor failed ");
                 exit(3);
          }
          break; //  3 connects and done
	 }

	sleep(5);
	if((errno = pthread_create(&thread_ids[th_index++], NULL,
                          msg_maker, NULL)) != 0){
                 perror("pthread_create msg_maker failed ");
                 exit(3);
        }

	for(i=0; i< (node_cnt - 1); ++i){
		pthread_join(thread_ids[i], NULL);
	}

  //after connection is constructed implement lamport's algorithm here
  //Initialize Lamport's algorithm

  LAMPORT lamport; //initialize lamport struck
  lamport.clock =0;
  lamport.queue_size=0;
  lamport.first-=1;
  lamport.node_id=node_id;

  //initialize past queue of messages
  for (i=0,i<20;i++){
    lamport.request[i].node_id=-1;
    lamport.request[i].previous=-1;
    lamport.request[i].next=-1;
    lamport.request[i].clock=-1;
    for (j=0;j<4;j++){
      lamport.request[i].reply[j].node_id=-1;
      lamport.request[i].reply[j].clock=-1;
    }
  }

  //send request
  send_req(&msg,&lamport,socket,connected_ch);
  int req_node; //recieved node it
  int req_clock; //recived clock
  int track=0;
  int s=0;
  while (1){
    s=(s+1)%4;
    if (read_msg(connected_ch[s],raw.buf)==-1) continue;

    type_val=ntohl(raw.m.mtype);

    switch (type_val)
    {
    case REQUEST:
      sscanf(raw.m.mbody,"REQUEST node %d clock %d",&req_node,&req_clock);
      add_req(&lamport,req_node,req_clock);
      send_reply(&msg,&lamport,connected_ch[s]);
      break;
    
    case REPLY:
      sscanf(raw.m.mbody,"REPLY node %d clock %d",&req_node,&req_clock);
      add_reply(&lamport,req_node,req_clock);
      break;

    case RELEASE:
      sscanf(raw.m.mbody,"REPLY node_id %d clock %d",&req_node,&req_clock);
      add_release(&lamport,req_node,req_clock)
      break;


    default:
      break;
    }

    if ()
    
  }


// Assuming 5 node example, main thread can exit here,
// full connection is constructed

	printf("\nnode controller finished, goodbye\n");
}



void send_request(MSG * msg,LAMPORT * lamport,int connected_ch[])
{
    int i;
    sprintf(msg->mbody,"REQUEST node_id %d clock %d",lamport->node_id,lamport->clock);
    for (i = 0;i < 3;i++)
    {
        send_message(msg,connected_ch[i],REQUEST);
    }
    add_request(lamport,lamport->node_id,lamport->clock);
    add_reply(lamport,lamport->node_id,lamport->clock);
}



void send_reply(MSG *msg,LAMPORT * lamport, int sock){
  
  sprintf(msg->mbody,"REPLY node_id %d clock %d",lamport->node_id,lamport->clock);
  send_message(msg,sock,MSG_LAMPORT_REPLY);
  lamport->clock++;
  
}




void send_release(MSG * msg,LAMPORT * lamport,int socket_list[])
{
    int i;
    sprintf(msg->mbody,"RELEASE node_id %d clock %d",lamport->node_id,lamport->clock);
    for (i = 0;i < 3;i++)
    {
        send_message(msg,socket_list[i],RELEASE);
    }
    lamport_add_release(lamport,lamport->node_id,lamport->clock);    
}


//
void add_release(LAMPORT * lamport,int node_id,int clock)
{
    int i, index;
    
    index = lamport->first;
    if (lamport->request[index].node_id == node_id)
    {
        lamport->first = lamport->request[index].next;
        lamport->request[index].node_id = -1;
        lamport->request[index].clock = -1;
        lamport->request[index].next = -1;
        lamport->request[index].previous = -1;
        for (i = 0;i<4;i++)
        {
            lamport->request[index].reply[i].clock = -1;
            lamport->request[index].reply[i].node_id = -1;
        }
        lamport->queue_size--;
    }
    else
    {
        printf("Unable to add release node %d clock %d\n",node_id,clock);
        printf("Tried to release something not first on the queue\n");
        lamport_print(lamport);
        exit(5);
    }
    //update clock in nessacary
    if (lamport->clock <= clock)
        lamport->clock = clock + 1;    
}

//what is it doing?
int check_cs(LAMPORT * lamport)
{
    int i;
    if (lamport->request[lamport->first].node_id == lamport->node_id)
    {
        for (i = 0;i < 4;i++)
        {
            if (lamport->request[lamport->first].reply[i].node_id == -1)
                return 0;
        }
        return 1;
    }
    return 0;
}


//what is it doing?
void lamport_print(LAMPORT * lamport)
{
    int i,j,index;
    index = lamport->first;
    printf("Printing Queue...(%d)\n",lamport->queue_size);
    while(1)
    {
        printf("Clock: %2d, Node ID: %d, Replies: ",lamport->request[index].clock,lamport->request[index].node_id);
        for (i = 0;i < 4;i++)
        {
            printf("[%02d/%02d]",lamport->request[index].reply[i].node_id,lamport->request[index].reply[i].clock);
        }
        printf("\n");
        index = lamport->request[index].next;
        if (index == -1)
            break;
    }
    printf("\n");
}



//what is it doing?
void add_request(LAMPORT * lamport,int node_id,int clock)
{
  int i;
  int index;
  int previous;
  int previousprevious;
  int next;
  //add to queue
  if (lamport->first == -1)
  {
      lamport->first = 0;
      index = lamport->first;
      lamport->queue_size = 1;
      lamport->request[index].clock = clock;
      lamport->request[index].node_id = node_id;
      lamport->request[index].next = -1;
      lamport->request[index].previous = -1;
  }
  else
  {
      //find value for index
      for (i = 0;i<20;i++)
      {
          if (lamport->request[i].node_id == -1)
              break;
      }
      lamport->request[i].clock = clock;
      lamport->request[i].node_id = node_id;

      //add it on the end
      index = lamport->first;
      while(1)
      {
          if (lamport->request[index].next == -1)
              break;
          index = lamport->request[index].next;
      }
      lamport->request[index].next = i;
      lamport->request[i].previous = index;
      lamport->request[i].next = -1;

      //slide it down
      while ((lamport->request[i].previous != -1) && (lamport->request[i].clock < lamport->request[lamport->request[i].previous].clock))
      {
          next = lamport->request[i].next;
          previous = lamport->request[i].previous;
          previousprevious = lamport->request[lamport->request[i].previous].previous;
          lamport->request[previous].previous = i;
          lamport->request[previous].next = next;
          lamport->request[i].previous = previousprevious;
          lamport->request[i].next = previous;
          lamport->request[previousprevious].next = i;
      }
      while ((lamport->request[i].previous != -1) &&
          (lamport->request[i].clock == lamport->request[lamport->request[i].previous].clock) &&
          (lamport->request[i].node_id < lamport->request[lamport->request[i].previous].node_id))
      {
          next = lamport->request[i].next;
          previous = lamport->request[i].previous;
          previousprevious = lamport->request[lamport->request[i].previous].previous;
          lamport->request[previous].previous = i;
          lamport->request[previous].next = next;
          lamport->request[i].previous = previousprevious;
          lamport->request[i].next = previous;
          if (previousprevious != -1)
              lamport->request[previousprevious].next = i;
      }
      if (lamport->request[i].previous == -1)
      {
          lamport->first = i;
      }
      lamport->queue_size++; 
  }
  //update clock in nessacary
  if (lamport->clock <= clock)
      lamport->clock = clock + 1;
}








void*	chan_monitor(void *my_arg){
	int 	my_chan, type_val, timestamp, node_id, my_node_id;
	MSG     msg;
        MBUF    raw;



	my_chan = ((TH_ARG *)my_arg)->my_chan;
	my_node_id = ((TH_ARG *)my_arg)->my_node_id;

//	printf("\nmy_chan is %d, my node_id is %d\n", my_chan, my_node_id);


//	if(my_node_id == 3){
	   make_msg(&msg, CONNECTED, my_node_id, 0, 0);
           if(write(my_chan, &msg, (4*sizeof(int))) == -1){
            perror("connected channel write failed: ");
            exit(3);
           }
//	}

	while(1){
	     read_msg(my_chan, &raw.buf);
	     type_val = ntohl(raw.m.mtype);
              node_id = ntohl(raw.m.mid);
	    timestamp = ntohl(raw.m.mdonut_num);
/***********************************************
	  pro_node_id = ntohl(raw.m.mnode_id);
***********************************************/

/***** what type of message has the client sent to us ??     *****/


          switch(type_val){
		case CONNECTED:
	   printf("\nchannel monitor is connected to node %d\n", node_id);
	   make_msg(&msg, CONN_ACK, my_node_id, 0, 0);
	   if(write(my_chan, &msg, (4*sizeof(int))) == -1){
            perror("connected channel write failed: ");
            exit(3);
           }

	   break;
		case CONN_ACK:
	   printf("\nreceived CONN_ACK from node %s\n", host_list[node_id]);
	   break;	
		default:
	   printf("\nchannel monitor received unknown message type: %d \n", type_val);
	  }
	}
}

void*  msg_maker(void * argx){
        int     my_chan, type_val, timestamp, node_id, my_node_id;
        MSG     msg;
        MBUF    raw;
	int	in_msg, ch_index;

  while(1){
	printf("\nenter 0 to quit, 1 for REQUEST, 2 for REPLY, 3 for RELEASE: ");
	scanf("%d", &in_msg);
	if(in_msg == 0)exit(0);
	make_msg(&msg, (in_msg+20), my_node_id, 0, 0);
	ch_index = 0;
	while(connected_ch[ch_index]){
           if(write(connected_ch[ch_index++], &msg, (4*sizeof(int))) == -1){
            perror("connected channel write failed: ");
            exit(3);
           }
	}
  }
}
