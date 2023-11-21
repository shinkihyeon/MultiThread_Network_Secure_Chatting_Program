#include "Net_Project.h"

void* handle_clnt(void* arg);
void ErrorHandling(char* message);
//for manage client socket
int clnt_cnt;
int clnt_socks[MAX_CLNT];

pthread_mutex_t mutx;

int main(int argc, char* argv[]) {
    void* thread_return;
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    pthread_t t_id;
    if (argc != 2) {
        printf("server_Error! Usage:%s <port>\n", argv[0]);
        exit(1);
    }
    pthread_mutex_init(&mutx, NULL);

    //SOCKET 통신 기본 세팅
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //주소 할당
    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        ErrorHandling("bind() error!");
    }

    //연결요청 가능한 상태로 변경
    if (listen(serv_sock, 5) == -1) {
        ErrorHandling("listen() error!");
    }
    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);
        //연결요청의 수락
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if (clnt_sock == -1) {
            ErrorHandling("accept() error!\n");
        }
        else {
            printf("Connected client!\n");
        }
        //임계영역의 문제 MUTEX로 THREAD 동기화해서 해결
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt] = clnt_sock;
        clnt_cnt += 1;
        //DEADLOCK에 빠지는 위험을 막기 위해서 UNLOCK 필수!
        pthread_mutex_unlock(&mutx);

        //THREAD 생성
        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);

        //THREAD 함수 종료시, 자동으로 자원을 반납하도록 수행
        //pthread_join() 함수와 기능은 비슷하나
        //pthread_join()은 쓰레드가 종료될 때까지 블록킹 상태에 놓인다는 차이가 있음
        pthread_detach(t_id);
    }
    pthread_mutex_destroy(&mutx);
    close(serv_sock);
    return 0;
}

//main 함수에서 생성한 thread들이 할 일을 수행하도록 하는 함수
//client로 부터 받는 메세지 HMAC을 통해서 무결성, 인증 과정 거친후
//다시 연결되어 있는 client들에게 받은 메세지 다 뿌려주는 함수
void* handle_clnt(void* arg) {
    int work_sock = *((int*)arg);
    uint32_t strLen = 0;
    uint32_t cnt_i = 0;
    message client_msg;
    memset(&client_msg, 0x00, sizeof(client_msg));
    uint8_t HMAC_KEY[20] = { 0x00, };
    memset(HMAC_KEY, 0x0b, 20);
    uint8_t HMAC_Value[32] = { 0x00, };
    uint32_t check = 0;
    uint32_t i = 0;
    uint32_t v_result = 0;
    while (1) {
        strLen = readn(work_sock, &client_msg, sizeof(client_msg));
        if (strLen > 0) {
            client_msg.msg_len = ntohl(client_msg.msg_len);
            //compute HMAC VALUE 
            HMAC(client_msg.payload, client_msg.msg_len, HMAC_KEY, 20, HMAC_Value);

            //받은 HMAC값과 직접 계산한 HMAC값을 비교함으로서
            //무결성, 인증 확인
            v_result = memcmp(HMAC_Value, client_msg.payload + BUF_SIZE, 32);
            printf("SERVER_Validation: %d\n", v_result);

            //send to client
            client_msg.msg_len = htonl(client_msg.msg_len);

            //임계영역의 문제 MUTEX로 THREAD 동기화해서 해결
            pthread_mutex_lock(&mutx);
            for (i = 0; i < clnt_cnt; i++) {
                check = writen(clnt_socks[i], &client_msg, sizeof(client_msg));
                if (check == -1) {
                    ErrorHandling("writen() error!\n");
                }
            }
            //DEADLOCK에 빠지는 위험을 막기 위해서 UNLOCK 필수!
            pthread_mutex_unlock(&mutx);
        }
        else {
            ErrorHandling("Error!!!\n");
            break;
        }
    }
    //}
    //remove desconnected client
    pthread_mutex_lock(&mutx);
    for (cnt_i = 0; cnt_i < clnt_cnt; cnt_i++) {
        if (work_sock == clnt_socks[cnt_i]) {
            while (cnt_i++ < (clnt_cnt - 1)) {
                clnt_socks[cnt_i] = clnt_socks[(cnt_i + 1)];
            }
            break;
        }
    }
    clnt_cnt -= 1;
    pthread_mutex_unlock(&mutx);

    close(work_sock);
    return NULL;
}

void ErrorHandling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}