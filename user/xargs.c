#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"
#define buf_size 512
int main(int argc, char *argv[]) {
  char buf[buf_size + 1] = {0};//滑动窗口
  uint occupy = 0;             //字节大小
  char *xargv[MAXARG] = {0};
  int stdin_end = 0;           //字符末尾标识符

  for (int i = 1; i < argc; i++) {
    xargv[i - 1] = argv[i];
  }

  while (!(stdin_end && occupy == 0)) {
    // try read from left-hand program    
    if (!stdin_end) {
      int remain_size = buf_size - occupy;
      int read_bytes = read(0, buf + occupy, remain_size);
      if (read_bytes < 0) {
        fprintf(2, "xargs: read returns -1 error\n");
      }
      //如果读取到 0 字节（即已到达标准输入的末尾），则关闭标准输入并设置为真
      if (read_bytes == 0) {
        close(0);
        stdin_end = 1;
      }
      occupy += read_bytes;
    }
    // 处理读取的行 
    char *line_end = strchr(buf, '\n');//每一行的结束
    while (line_end) {
      char xbuf[buf_size + 1] = {0};
      memcpy(xbuf, buf, line_end - buf);
      xargv[argc - 1] = xbuf;
      int ret = fork();
      if (ret == 0) {
        //子进程使用 exec 执行由 argv[1] 指定的程序，并将 xargv 作为参数传递      
	      if (!stdin_end) {
          close(0);
        }
        if (exec(argv[1], xargv) < 0) {
          fprintf(2, "xargs: exec fails with -1\n");
          exit(1);
        }
      } else {
        // 父进程从滑动窗口中移除已处理的行
	      memmove(buf, line_end + 1, occupy - (line_end - buf) - 1);
        occupy -= line_end - buf + 1;
        memset(buf + occupy, 0, buf_size - occupy);
        // 回收       
	      int pid;
        wait(&pid);
        //继续查找和处理下一行
        line_end = strchr(buf, '\n');
      }
    }
  }
  exit(0);}
