#include "readnwrite.h"

//읽을 DATA 크기 만큼 계속 읽는 함수
//=>TCP는 DATA의 경계를 구분하지 않고 데이터를 전송하는 특성이 있음
ssize_t readn(int fd, void* vptr, size_t n) {
    ssize_t nleft;
    ssize_t nread;
    uint8_t* ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        nread = read(fd, ptr, nleft);

        if (nread == -1) {
            return -1;
        }
        else if (nread == 0) {
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);
}

//전송할 DATA 크기 만큼 계속 write하는 함수
//=>TCP는 DATA의 경계를 구분하지 않고 데이터를 전송하는 특성이 있음
ssize_t writen(int fd, void* vptr, size_t n) {
    ssize_t nleft;
    ssize_t nwritten;
    uint8_t* ptr;
    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        nwritten = write(fd, ptr, nleft);

        if (nwritten == -1) {
            return -1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}


