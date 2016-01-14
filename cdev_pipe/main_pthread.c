/**
 *  test cj_cdev_pipe interacting with multiple threads.
 *   each thread will either read or write data from the 
 *   device.
 *  
 *  usage:
 *   ./main_fork [number of write threads] [number of read threads]
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>

#define MAX_THREAD_NUM 15
#define ITR_NUM 20

const char *cdev_path = "/dev/cj";

/* data structure being past to each threads */
struct thread_data {
  int wr_thread;         	               /* true if it is a write thread, otherwise false  */
  int unique;				       /* unique number for this thread */
};

void *io_cj(void *arg) {
  struct thread_data *data = (struct thread_data *) arg;

  int fd = open(cdev_path, O_RDWR);
  if (fd < 0) {
    printf("thread %d can't open the file\n", data->unique);
    return NULL;
  }
  
  int bufsize = 512;
  char buf[bufsize];
  int i;
  int ret;
  if (data->wr_thread) {
    for (i = 0; i < ITR_NUM; ++i) {
      ret = sprintf(buf, "hi, from thread %d\n", data->unique);
      ret = write(fd, buf, ret);
      if (ret <= 0) 
	printf("thread %d failed to write data\n", data->unique);
    }
  } else {
    for (i = 0; i < ITR_NUM; ++i) {
      ret = read(fd, buf, bufsize-1);
      buf[ret] = '\0';
      if (ret > 0) 
	printf("thread %d read \"%s\"\n", data->unique, buf);
      else 
	printf("thread %d failed to read data\n", data->unique);
    }
  }
  return NULL;
}

int main(int argc, const char *argv[]) {

  if (argc != 3) {
    printf("wrong number of arguments\n");
    return -1;
  }

  int n_wrth = atoi(argv[1]);
  int n_rdth = atoi(argv[2]);

  if (n_wrth + n_rdth > MAX_THREAD_NUM) {
    printf("too many threads\n");
    return -1;
  }

  /* creating threads */
  pthread_t *threads = malloc(sizeof(threads)*(n_wrth+n_rdth));
  struct thread_data *thread_arg = malloc(sizeof(struct thread_data)*(n_wrth+n_rdth));
  int thread_i;
  for (thread_i = 0; thread_i < n_wrth+n_rdth; thread_i++) {
    thread_arg[thread_i].wr_thread = (thread_i > n_wrth) ? 1 : 0;
    thread_arg[thread_i].unique = thread_i;
    int ret = pthread_create(&threads[thread_i], NULL, io_cj, (void*) &thread_arg[thread_i]);
    if (ret) 
      printf("failed to create thread %d\n", thread_i);
  }
  
  /* waiting threads to finish */
  for (thread_i = 0; thread_i < n_wrth+n_rdth; thread_i++) {
    int ret = pthread_join(threads[thread_i], NULL);
    if (!ret)
      printf("thread %d exiting\n", thread_i);
  }
  
  free(threads);
  free(thread_arg);
  return 0;
}
