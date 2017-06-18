#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_SIZE 8192

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))


pthread_t stats_thread; 
pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

/*Thread handling for shortest path */
void *shortest_path(void *vargp)
{

    sigset_t sigs_to_catch;	
    int caught;	
		
    sigemptyset(&sigs_to_catch);	
    sigaddset(&sigs_to_catch, SIGUSR1);	
    for (;;) {	
	  sigwait(&sigs_to_catch, &caught);	
	  /* Proceed to lock mutex and display statistics */	
	  pthread_mutex_lock(&stats_lock);	
          struct Graph* graph = (struct Graph*) vargp;
          printf("::This is a shortest path finding thread \n");
          dijkstra(graph, 0);
	  pthread_mutex_unlock(&stats_lock);	
    }
    return NULL;
}

/*Thread handling for spanning tree */
void *minimum_spanning_tree(void *vargp)
{


    sigset_t sigs_to_catch;	
    int caught;	
		
    sigemptyset(&sigs_to_catch);	
    sigaddset(&sigs_to_catch, SIGUSR2);	
    for (;;) {	
	  sigwait(&sigs_to_catch, &caught);	
	  /* Proceed to lock mutex and display statistics */	
	  pthread_mutex_lock(&stats_lock);	
          struct Graph* graph = (struct Graph*) vargp;
          printf("::This is a spanning tree finding thread \n");
          PrimMST(graph);
	  pthread_mutex_unlock(&stats_lock);	
    }

    return NULL;
}



int ut_update_file(char* file_p) { 

    int input_fd;               /* Input file descriptor */
    ssize_t ret_in, ret_out;    /* Number of bytes returned by read() and write() */
    char buffer[BUF_SIZE];      /* Character buffer */


    /* Create input file descriptor */
    input_fd = open (file_p, O_RDONLY);
    if(input_fd == -1){
        perror("::error in file updateopen");
        return 0;
    }

    /* Copy process */
    while((ret_in = read (input_fd, &buffer, BUF_SIZE)) > 0){
            ret_out = write (STDIN_FILENO, &buffer, (ssize_t) ret_in);
            if(ret_out != ret_in){
                /* Write error */
                perror("::error in file update write");
                return 0;
            }
    }

    /* Close file descriptors */
    close (input_fd);

    return 0;
}



/*Thread handling for file changes update path */
void *file_changes_update(void *vargp)
{
    int length, i = 0;
    int fd;
    int wd;
    char buffer[BUF_LEN];
    char *file_p = (char*) vargp;

    while(1) {
     fd = inotify_init();

     if (fd < 0) {
        perror("inotify_init");
     }

     wd = inotify_add_watch(fd, ".",
        IN_MODIFY | IN_CREATE | IN_DELETE);
     length = read(fd, buffer, BUF_LEN);
     if (length < 0) {
        perror("read");
     }

      while (i < length) {
          struct inotify_event *event =
              (struct inotify_event *) &buffer[i];
          if (event->len) {
              if (event->mask & IN_CREATE) {
                  printf("The file %s was created.\n", event->name);
                  if(strncmp(event->name,file_p,9) == 0) {
                       ut_update_file(file_p); 
                   }
              } else if (event->mask & IN_DELETE) {
                  printf("::This is a file_changes_update thread \n");
                  printf("The file %s was deleted.\n", event->name);
              } else if (event->mask & IN_MODIFY) {
                  printf("::This is a file_changes_update thread \n");
                  printf("The file %s was modified.\n", event->name);
                  if(strncmp(event->name,file_p,9) == 0) {
                       ut_update_file(file_p); 
                   }
              }
          }
          i += EVENT_SIZE + event->len;
       }

    (void) inotify_rm_watch(fd, wd);
    (void) close(fd);
    }

    return 0;
} 

