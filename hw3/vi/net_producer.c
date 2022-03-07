/** net_producer.c - for distributed donuts process implementations **/

#include "ddonuts.h"
#include "buf_mgr.h"


int	main(int argc, char *argv[])
{

	int		i,j,k,nsigs;
	struct timeval 		randtime;
	unsigned short 		xsub1[3];
        MSG     msg;
        MBUF    raw;
        int     inet_sock, local_file, donut_num, node_id, inet_sock_node;
        int     type_val, id_val, read_val, local_size, my_id;
        char    *buffer_ptr, *token_ptr, *last_token_ptr;
        char    full_file_path_name[256];
        union type_size;
        struct sockaddr_in_buf inet_telnum_buf;
				struct sockaddr_in_node inet_telnum_node;
				struct hostent_node *heptr_node, *gethostbyname();
        struct hostent_buf *heptr_buf, *gethostbyname();

	if(argc < 4){
	  printf("\nUSAGE: net_producer BM_host_name prod_id node_id\n");
	  exit(2);
	}

	my_id = atoi(argv[2]);
  node_id = atoi(argv[3]);


	if((heptr_node = gethostbyname( argv[1] )) == NULL){
		perror("gethostbyname failed: ");
		exit(1);
	}
	bcopy(heptr_node->h_addr, &inet_telnum_node.sin_addr, heptr_node->h_length);
	inet_telnum_node.sin_family = AF_INET;
	inet_telnum_node.sin_port = htons( (u_short)PORTNODE );				// getting port number for nodes


        if((heptr_buf = gethostbyname( argv[1] )) == NULL){
          perror("gethostbyname failed: ");
          exit(1);
        }

        bcopy(heptr_buf->h_addr, &inet_telnum_buf.sin_addr, heptr_buf->h_length);
        inet_telnum_buf.sin_family = AF_INET;
        inet_telnum_buf.sin_port = htons( (u_short)PORT );				// getting port number for buffer manager

     	gettimeofday(&randtime, NULL);

        xsub1[0] = (ushort)randtime.tv_usec;
        xsub1[1] = (ushort)(randtime.tv_usec >> 16);
        xsub1[2] = (ushort)(getpid());

	donut_num = 1;
	printf("\n starting producer %d on node %d\n", my_id, node_id);
	while(1){
	  j=nrand48(xsub1) & 3;
// calling the node controller to ask for the permission

if((inet_sock_node=socket(AF_INET, SOCK_STREAM, 0)) == -1){
	perror("inet_sock allocation failed: ");
	exit(1);
	if(connect(inet_sock_node, (struct sockaddr *)&inet_telnum_node,
														 sizeof(struct sockaddr_in_node)) == -1){
					perror("inet_sock connect failed: ");
					exit(2);
				}
}
make_msg(&msg, j, my_id, donut_num++, node_id);
			if(write(inet_sock_node, &msg, (4*sizeof(int))) == -1){
				perror("inet_sock write failed: ");
				exit(3);
			}

			read_msg(inet_sock_node, &raw.buf);


// calling the buffer manager to produce the donut
type_val1 = ntohl(raw.m.mtype);
if(type_val1==RELEASE)
{

          if((inet_sock=socket(AF_INET, SOCK_STREAM, 0)) == -1){
            perror("inet_sock allocation failed: ");
            exit(1);
          }

	  if(connect(inet_sock, (struct sockaddr *)&inet_telnum_buf,
                               sizeof(struct sockaddr_in_buf)) == -1){
            perror("inet_sock connect failed: ");
            exit(2);
          }

	  make_msg(&msg, j, my_id, donut_num++, node_id);
          if(write(inet_sock, &msg, (4*sizeof(int))) == -1){
            perror("inet_sock write failed: ");
            exit(3);
          }

          read_msg(inet_sock, &raw.buf);

           type_val = ntohl(raw.m.mtype);

	  if(type_val != P_ACK)printf("\nBAD REPLY FROM BUFFER MANAGER\n");
	  close(inet_sock);
}
// sleep between each donut made

	  usleep(10000);
	} // while 1
}
