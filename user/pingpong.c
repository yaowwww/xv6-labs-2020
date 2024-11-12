#include "kernel/types.h"
//#include "kernel/stat.h"
#include "user/user.h"

//0是read端，1是write端
int main(int argc, char *argv[])       {
    int p1[2],p2[2];       //两个文件描述符
    pipe(p1);           //父管道
    pipe(p2);           //子管道
    char buf[8];         //缓冲区
//int exit_status = 0;   //判断是否有异常退出情况

    if(fork() < 0){   //出错
	fprintf(2, "fork() error!\n");
	close(p1[0]);
	close(p1[1]);
	close(p2[0]);
	close(p2[1]);
	exit(1);
    }else if(fork() == 0){       //fork()返回0即为子进程，否则为父进程
	close(p1[1]);
	close(p2[0]);
	read(p1[0],buf,4);     //从父管道的读端读出父的数据写进缓冲区
	printf("%d:receive %s\n",getpid(),buf);
	write(p2[1],"pong",strlen("pong"));       //子管道写
	close(p1[0]);
	close(p2[1]);
    }else{                  //父进程
        close(p1[0]);
	close(p2[1]);
	write(p1[1],"ping",strlen("ping"));
	wait(0);   //调用 wait() 函数等待子进程完成操作后退出，传入参数 0 或者 NULL
	read(p2[0],buf,4);
	printf("%d:receive %s\n",getpid(),buf);
	close(p1[1]);
	close(p2[0]);
    }
    exit(0);
}
