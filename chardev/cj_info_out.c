/**
 *  This program is for getting information about cj_cdev.
 *   
 *   the 1 is the index of cj_list being requested.
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "cj_cdev_user.h"

int main(int argc, const char* argv[]) {

  unsigned int max_s;
  int index;
  char *dev_fp = "/dev/cj";            // device file path
  if (argc != 2 && argc != 3) {
    printf("usage: cj_info_out index [max_size]\n");
    return -1;
  } 
  index = atoi(argv[1]);

  if (argc == 2) {
    max_s = 5024;                // default value
  }

  if (argc == 3) {
    max_s = atoi(argv[2]);
    if (!max_s) {
      printf("invalid max size\n");
      return -1;
    }
  }
  
  // open the device file
  int dev_fd = open(dev_fp, O_RDWR);
  if (dev_fd < 0) {
    printf("can't open device file: %s\n", dev_fp);
    return -1;
  }

  // prepare a buffer
  char *data = malloc(sizeof(char)*max_s);
  long cdsize;
  long total_size;
  // let default be error. If it is written to 0, that means it succeeded
  int ret = -1;                               

  struct pcmd_arg buf = {
    .li = index,
    .data = data,
    .cdsize = &cdsize,
    .max_s = max_s,
    .total_size = &total_size,
    .ret = &ret,
  };

  // obtaining data and printing it out
  ret = ioctl(dev_fd, CJ_READ, &buf);
  if (ret) {
    printf("there is an error when getting the data. ret: %d\n", ret);
  }
  printf(" total_size: %li\n cdsize: %li\n data: \n--------\n%s\n---------\n", 
	 total_size, cdsize, buf.data);

  // free the allocated memory
  free(data);
}

