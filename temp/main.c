/**
 * poke the kernel module. 
 *  argv[1] is assumed to be path to the device.
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>


int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("arg is wrong\n");
    return -1;
  }
  
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf("can't open %s\n", argv[1]);
    return -1;
  }
  close(fd);
  printf("finished poking, use dmesg to check the result\n");
  return 0;
}
