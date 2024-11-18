// #include "kernel/types.h"
// //#include "kernel/stat.h"
// #include "user/user.h"

// //0是read端，1是write端
// int main(int argc, char *argv[])       {
//     int p1[2],p2[2];       //两个文件描述符
//     pipe(p1);           //父管道
//     pipe(p2);           //子管道
//     char buf[8];         //缓冲区
// //int exit_status = 0;   //判断是否有异常退出情况

//     if(fork() < 0){   //出错
// 	fprintf(2, "fork() error!\n");
// 	close(p1[0]);
// 	close(p1[1]);
// 	close(p2[0]);
// 	close(p2[1]);
// 	exit(1);
//     }else if(fork() == 0){       //fork()返回0即为子进程，否则为父进程
// 	close(p1[1]);
// 	close(p2[0]);
// 	read(p1[0],buf,4);     //从父管道的读端读出父的数据写进缓冲区
// 	printf("%d:receive %s\n",getpid(),buf);
// 	write(p2[1],"pong",strlen("pong"));       //子管道写
// 	close(p1[0]);
// 	close(p2[1]);
//     }else{                  //父进程
//         close(p1[0]);
// 	close(p2[1]);
// 	write(p1[1],"ping",strlen("ping"));
// 	wait(0);   //调用 wait() 函数等待子进程完成操作后退出，传入参数 0 或者 NULL
// 	read(p2[0],buf,4);
// 	printf("%d:receive %s\n",getpid(),buf);
// 	close(p1[1]);
// 	close(p2[0]);
//     }
//     exit(0);
// }
#include "kernel/types.h"
#include "user/user.h"

#define RD 0 //pipe的read端
#define WR 1 //pipe的write端

int main(int argc, char const *argv[]) {
    char buf = 'P'; //用于传送的字节

    int fd_c2p[2]; //子进程->父进程
    int fd_p2c[2]; //父进程->子进程
    pipe(fd_c2p);
    pipe(fd_p2c);

    int pid = fork();
    int exit_status = 0;

    if (pid < 0) {
        fprintf(2, "fork() error!\n");
        close(fd_c2p[RD]);
        close(fd_c2p[WR]);
        close(fd_p2c[RD]);
        close(fd_p2c[WR]);
        exit(1);
    } else if (pid == 0) { //子进程
        close(fd_p2c[WR]);
        close(fd_c2p[RD]);

        if (read(fd_p2c[RD], &buf, sizeof(char)) != sizeof(char)) {
            fprintf(2, "child read() error!\n");
            exit_status = 1; //标记出错
        } else {
            fprintf(1, "%d: received ping\n", getpid());
        }

        if (write(fd_c2p[WR], &buf, sizeof(char)) != sizeof(char)) {
            fprintf(2, "child write() error!\n");
            exit_status = 1;
        }

        close(fd_p2c[RD]);
        close(fd_c2p[WR]);

        exit(exit_status);
    } else { //父进程
        close(fd_p2c[RD]);
        close(fd_c2p[WR]);

        if (write(fd_p2c[WR], &buf, sizeof(char)) != sizeof(char)) {
            fprintf(2, "parent write() error!\n");
            exit_status = 1;
        }

        if (read(fd_c2p[RD], &buf, sizeof(char)) != sizeof(char)) {
            fprintf(2, "parent read() error!\n");
            exit_status = 1; //标记出错
        } else {
            fprintf(1, "%d: received pong\n", getpid());
        }

        close(fd_p2c[WR]);
        close(fd_c2p[RD]);

        exit(exit_status);
    }
}

