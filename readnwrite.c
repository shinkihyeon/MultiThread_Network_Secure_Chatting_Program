#include "readnwrite.h"

//���� DATA ũ�� ��ŭ ��� �д� �Լ�
//=>TCP�� DATA�� ��踦 �������� �ʰ� �����͸� �����ϴ� Ư���� ����
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

//������ DATA ũ�� ��ŭ ��� write�ϴ� �Լ�
//=>TCP�� DATA�� ��踦 �������� �ʰ� �����͸� �����ϴ� Ư���� ����
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


