/**
 *  test cj_cdev_pipe interacting with multiple processes.
 *   Spawn multiple processes. Each process will either read
 *   data or write data. 
 *  
 *  usage:
 *   ./main_fork [number of write processes] [number of read processes]
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

/**
 *  for parent process to handle quiting signals
 */
void pr_signal_handler(int signum) {
  switch (signum) {
  case SIGTERM:
  case SIGINT:
  case SIGQUIT:
  case SIGHUP:
    /* pass the signal to child processes */
    kill(0, signum);
    exit(-1);
    break;
  default:
    printf("parent is getting %d\n", signum);
  }
}

/**
 *  for child processes to handle quiting signals
 */
void ch_signal_handler(int signum) {
  switch (signum) {
  case SIGTERM:
  case SIGINT:
  case SIGQUIT:
  case SIGHUP:
    /* quiting signal */
    exit(-1);
    break;
  default:
    printf("signal %d is sent to process %d\n", signum, getpid());
  }
}


static struct sigaction action;

/**
 *  calculate a reasonable maximum time interval for a ps to do something.
 *  This is for testing purposes. The unit is seconds
 *  @n_ps: number of processes
 */
#define max_itime(n_ps) ((n_ps)*5)

/* number of iterations */
#define max_n_itr 10

const static char *cdev_path = "";
enum my_ps_t {
  WRITE_PS,
  READ_PS,
};

int main(int argc, const char *argv[]) {
  if (argc != 3) {
    printf("wrong arguments");
    return -1;
  }

  int n_wrps = atoi(argv[1]);
  int n_rdps = atoi(argv[2]);

  if (n_wrps == 0 || n_rdps == 0) {
    printf("at least 1 for both of read and write\n");
    return -1;
  }

  int nf;
  pid_t ret_pid;
  int my_num;			    
  enum my_ps_t ps_t = -1;
  int pipefd[2];
  if (pipe(pipefd) < 0) {
    printf("fail to create pipes\n");
    return -1;
  }

  for (nf = 0; nf < n_wrps + n_rdps; ++nf) {
    ret_pid = fork();
    if (ret_pid < 0) {
      printf("can't fork any more?\n");
      break;
    }
    
    if (!ret_pid) {
      /* set the pipe on child side*/
      close(pipefd[0]);
      close(1);
      dup2(pipefd[1], 1);


      /* child process */
      ps_t = (nf < n_wrps) ? WRITE_PS : READ_PS;
      my_num = nf;

      /* register signal handler */
      action.sa_handler = ch_signal_handler;
      if (sigemptyset(&action.sa_mask) < 0) {
	printf("set signal failed?\n");
      }
      action.sa_flags = 0;
    
      if (sigaction(SIGTERM, &action, NULL) < 0) 
	printf("register SIGTERM failed\n");
      if (sigaction(SIGINT, &action, NULL) < 0) 
	printf("register SIGINT failed\n");
      if (sigaction(SIGQUIT, &action, NULL) < 0) 
	printf("register SIGQUIT failed\n");
      if (sigaction(SIGHUP, &action, NULL) < 0) 
	printf("register SIGHUP failed\n");


      break;
    }
  }

  if (ps_t == -1) {
    /* parent process. hang in there for getting data */
    close(pipefd[1]);

    /* register signal handler */
    action.sa_handler = pr_signal_handler;
    if (sigemptyset(&action.sa_mask) < 0) {
      printf("set signal failed?\n");
    }
    action.sa_flags = 0;
    
    if (sigaction(SIGTERM, &action, NULL) < 0) 
      printf("register SIGTERM failed\n");
    if (sigaction(SIGINT, &action, NULL) < 0) 
      printf("register SIGINT failed\n");
    if (sigaction(SIGQUIT, &action, NULL) < 0) 
      printf("register SIGQUIT failed\n");
    if (sigaction(SIGHUP, &action, NULL) < 0) 
      printf("register SIGHUP failed\n");


    /* listening for input from child processes */
    const int buf_size = 512;
    char buf[buf_size+1];
    memset(buf, 0, buf_size+1);
    int n_rd = 0;
    while (1) {
      n_rd = read(pipefd[0], buf_size);
      if (n_rd > 0) {
	buf[n_rd] = '\0';
	printf("%s", buf);
      }
    }

    return 0;
  }

  /* child processes */

  int t_intval = max_itime(n_wrps+n_rdps);        /* actual number of process might be less than n_wrps+n_rdps */
  time_t time_p = time(NULL);                     /* previous time */
  int n_itr = 0;


  int fd = open(cdev_path, O_WRONLY, 0);
  if (fd < 0) {
    printf("%d: open can't it.\n error: %s\n", my_num, strerror(errno));
    return -1;
  }
    
  char out_s[2];
  out_s[0] = (char)my_num + 'a';
  out_s[1] = '\0';
  int ret;
  while (1) {
    if (n_itr >= max_n_itr)
      break;

    /* TODO t_intval should be random generated */
    if (time(NULL) - time_p > t_intval) {
      time_p = time(NULL);

      if (ps_t == WRITE_PS) {
	ret = write(fd, out_s, 1);
	if (ret == 1) 
	  printf("%d wrote %s\n", my_num, out_s);
	else 
	  printf("%d failed to write\n. ret: %d\n", my_num, ret);

      } else {
	ret = read(fd, out_s, 1);
	if (ret == 1) 
	  printf("%d read %s\n", my_num, out_s);
	else 
	  printf("%d failed to read\n. ret: %d\n", my_num, ret);
      }
      ++n_itr;
    }
  }
  
  return 0;
}
