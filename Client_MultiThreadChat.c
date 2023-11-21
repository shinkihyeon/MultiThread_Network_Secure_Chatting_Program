#include "Net_Project.h"
pthread_mutex_t mutx;
message msg;
message dec_msg;
pthread_t thread[4];//why declare global variable? => use two part:send_msg,recv_msg
DATA enc_data[4];

int main(int argc, char* argv[]) {
	int sock;
	//MUTEX 생성
	pthread_mutex_init(&mutx, NULL);
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void* thread_return;
	if (argc != 3) {
		printf("Usage:%s <IP> <PORT>\n", argv[0]);
		exit(1);
	}

	//SOCKET 통신 기본 세팅
	sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	//connect to server
	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		ErrorHandling("connect() error!\n");
	}

	//메세지를 받을 역할을 할 THREAD
	//메세지를 보낼 역할을 할 THREAD
	//각각 생성
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);

	//THREAD 함수 종료시, 자동으로 자원을 반납하도록 수행
	//THREAD가 종료될 때까지 블록킹 상태에 놓임
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);

	//MUTEX 소멸
	pthread_mutex_destroy(&mutx);
	close(sock);
	return 0;
}

//CTR운영모드에 필요한 함수
//Counter +1씩 해주는 함수
void CTR_INC(uint8_t* ICTR) {
	int cnt_i;
	for (cnt_i = 15; cnt_i > 7; cnt_i--) {
		if (ICTR[cnt_i] != 0xff) {
			ICTR[cnt_i] += 1;
			break;
		}
		else {
			ICTR[cnt_i] = 0x00;
		}
	}
}


//ARIA_CTR을 멀티스레드로 CPU_Parallel 해주는 함수!
void* ARIA_CTR(void* arg) {
	uint32_t cnt_i;
	//thread number setting
	uint32_t thread_num = *(uint32_t*)arg;
	//ctr_crypto start
	uint8_t CTR_KEY[16] = { 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff };
	uint32_t enc_len = (enc_data[thread_num].msg_size % 16 == 0) ? enc_data[thread_num].msg_size : enc_data[thread_num].msg_size + (16 - enc_data[thread_num].msg_size % 16);
	uint8_t temp[1024] = { 0x00, };//카운터값 암호화한 값 들어가는 변수
	uint8_t result[1024] = { 0x00, };//xor result
	uint8_t iv[16] = { 0x00, };
	uint8_t IV[1024] = { 0X00, };//증가된 CTR값이 들어가는 변수
	uint32_t enc_start = 16 * (((enc_data[thread_num].sum_size >> 2) * thread_num) >> 4);
	uint32_t enc_end = (((enc_data[thread_num].sum_size >> 2) * thread_num) + enc_data[thread_num].msg_size - 1) + (16 - ((((enc_data[thread_num].sum_size >> 2) * thread_num) + enc_data[thread_num].msg_size - 1)) % 16);

	//CTR을 암호화하기전의 해당부분의 CTR값으로 만드는 작업
	for (cnt_i = 0; cnt_i < (enc_start >> 4); cnt_i++) {
		CTR_INC(iv);
	}

	//필요한 만큼의 CTR+@를 생성하는 작업
	for (cnt_i = 0; cnt_i < ((enc_end - enc_start) >> 4); cnt_i++) {
		memcpy(IV + (16 * cnt_i), iv, 16);
		CTR_INC(iv);
	}

	//CTR암호화
	for (cnt_i = 0; cnt_i < ((enc_end - enc_start) >> 4); cnt_i++) {
		ARIA_EnCrypt(IV + (16 * cnt_i), temp + (16 * cnt_i), CTR_KEY);//필요한만큼 잘라쓸거야!
	}

	uint32_t temp_start = ((enc_data[thread_num].sum_size >> 2) * thread_num) - enc_start;
	//ct -> pt ^ ENC(CTR)
	for (cnt_i = 0; cnt_i < enc_data[thread_num].msg_size; cnt_i++) {//필요한만큼 잘라서 계산!
		result[cnt_i] = enc_data[thread_num].text_msg[cnt_i] ^ temp[cnt_i + temp_start];
	}

	//마지막 처리
	memcpy(msg.payload + ((enc_data[thread_num].sum_size >> 2) * thread_num), result, enc_data[thread_num].msg_size);
	return NULL;
}

//Server로 메세지 보내주는 함수
void* send_msg(void* arg) {
	int send_sock = *((int*)arg);
	//variables setting
	uint32_t cnt_i;
	uint8_t HMAC_KEY[20] = { 0x00, };
	memset(HMAC_KEY, 0x0b, 20);
	uint8_t HMAC_V[32] = { 0x00, };
	uint32_t check = 0;
	uint32_t msg_size = 0;
	uint8_t mid_msg[BUF_SIZE];
	memset(mid_msg, 0x00, BUF_SIZE);
	uint32_t index[4] = { 0,1,2,3 };//thread number

	//send msg to server
	while (1) {
		fgets(mid_msg, BUF_SIZE, stdin);
		if (!strcmp(mid_msg, "q\n") || !strcmp(mid_msg, "Q\n")) {
			close(send_sock);
			exit(0);
		}
		msg_size = strlen(mid_msg) - 1;//EXCEPT '\n'

		//create DATA ARRAY & create thread
		for (cnt_i = 0; cnt_i < 4; cnt_i++) {
			//if,else -> last data(pt part) can be not 16byte!
			//msg -> enc_data(size 16 bytes)
			if (cnt_i == 3) {
				memcpy(enc_data[cnt_i].text_msg, mid_msg + ((msg_size >> 2) * cnt_i), msg_size - ((msg_size >> 2) * 3));
				enc_data[cnt_i].msg_size = msg_size - ((msg_size >> 2) * 3);
			}
			else {
				memcpy(enc_data[cnt_i].text_msg, mid_msg + ((msg_size >> 2) * cnt_i), msg_size >> 2);
				enc_data[cnt_i].msg_size = msg_size >> 2;
			}
			enc_data[cnt_i].sum_size = msg_size;
			pthread_create(&thread[cnt_i], NULL, ARIA_CTR, (void*)&index[cnt_i]);
		}

		//THREAD 함수 종료시, 자동으로 자원을 반납하도록 수행
		//THREAD가 종료될 때까지 블록킹 상태에 놓임
		for (cnt_i = 0; cnt_i < 4; cnt_i++) {
			pthread_join(thread[cnt_i], NULL);
		}

		//compute HMAC
		HMAC(msg.payload, msg_size, HMAC_KEY, 20, HMAC_V);
		//C||HMAC(C)
		memcpy((msg.payload + 4096), HMAC_V, 32);

		//msg_len
		msg.msg_len = msg_size;
		msg.msg_len = htonl(msg.msg_len);
		//SEND TO SERVER
		check = writen(send_sock, &msg, sizeof(msg));
		if (check == -1) {
			ErrorHandling("writen() error!");
		}
	}
	return NULL;
}

//Server로부터 메세지를 받는 함수
void* recv_msg(void* arg) {
	int recv_sock = *((int*)arg);
	//send_msg에서 전역변수인 msg에 암호문을 넣어놨기 때문에 recv_msg에서 전역변수 msg에 접근(복호화된 데이타를 주입)하기 
	//전에 초기화 시키는작업
	memset(&msg, 0x00, sizeof(msg));

	//send_msg에서 전역변수인 enc_data에 암호문을 넣어놨기 때문에 recv_msg에서 전역변수 enc_data에 접근하기 
	//전에 초기화 시키는작업
	memset(&enc_data, 0x00, sizeof(enc_data));

	//variables setting
	uint8_t HMAC_KEY[20] = { 0x00, };
	memset(HMAC_KEY, 0x0b, 20);
	uint8_t HMAC_V[32] = { 0x00, };
	uint32_t v_result = 0;
	uint32_t recvn_check = 0;
	uint32_t cnt_i;
	uint32_t index[4] = { 0,1,2,3 };//thread number

	while (1) {
		recvn_check = readn(recv_sock, &dec_msg, (sizeof(dec_msg)));
		if (recvn_check > 0) {
			dec_msg.msg_len = ntohl(dec_msg.msg_len);
			HMAC(dec_msg.payload, dec_msg.msg_len, HMAC_KEY, 20, HMAC_V);

			//validation part
			//받은 HMAC값과 직접 계산한 HMAC값을 비교함으로서
			//무결성, 인증 확인
			v_result = memcmp(HMAC_V, dec_msg.payload + BUF_SIZE, 32);
			printf("CLIENT_VALIDATION: %d\n", v_result);

			//CREATE DATA ARRAY & CREATE THREAD
			for (cnt_i = 0; cnt_i < 4; cnt_i++) {
				if (cnt_i == 3) {
					memcpy(enc_data[cnt_i].text_msg, dec_msg.payload + ((dec_msg.msg_len >> 2) * cnt_i), dec_msg.msg_len - ((dec_msg.msg_len >> 2) * 3));
					enc_data[cnt_i].msg_size = dec_msg.msg_len - ((dec_msg.msg_len >> 2) * 3);
				}
				else {
					memcpy(enc_data[cnt_i].text_msg, dec_msg.payload + ((dec_msg.msg_len >> 2) * cnt_i), dec_msg.msg_len >> 2);
					enc_data[cnt_i].msg_size = dec_msg.msg_len >> 2;
				}
				enc_data[cnt_i].sum_size = dec_msg.msg_len;

				//기밀성 제공을 위해서 ARIA 메세지 병렬 암호화
				pthread_create(&thread[cnt_i], NULL, ARIA_CTR, (void*)&index[cnt_i]);
			}
			for (cnt_i = 0; cnt_i < 4; cnt_i++) {
				pthread_join(thread[cnt_i], NULL);
			}
			msg.payload[dec_msg.msg_len] = '\0';
			printf("msg from server(decryption result):");
			puts(msg.payload);
		}
		else if (recvn_check == -1) {
			ErrorHandling("readn Error!!!");
		}
	}
	return NULL;
}

void ErrorHandling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}