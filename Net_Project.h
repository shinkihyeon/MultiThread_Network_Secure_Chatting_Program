#pragma once
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "readnwrite.h"

#define BLOCKLEN 16
#define MAXTHREAD 256
#define MAX_CLNT 256
#define BUF_SIZE 4096
#define MAC_SIZE 32
#define PAYLOAD_SIZE (BUF_SIZE+MAC_SIZE) 

typedef struct _message_ {
	uint8_t payload[PAYLOAD_SIZE];
	uint32_t msg_len;
}message;

//for Multithread_CTR struct
typedef struct _DATA_ {
	uint8_t text_msg[1024];
	uint32_t msg_size;
	uint32_t sum_size;
}DATA;

//ARIA
void Add_SubstOdd_Layer(uint8_t* input, uint8_t* key);
void Add_SubstEven_Layer(uint8_t* input, uint8_t* key);
void DiffLayer(uint8_t* input);
void Round_odd(uint8_t* input, uint8_t* rk);
void Round_even(uint8_t* input, uint8_t* rk);
void Final_Round(uint8_t* input, uint8_t* key12, uint8_t* key13);
void Rot_L(uint8_t* input, uint8_t* output, int num);
void ROT_XOR(uint8_t* input, uint8_t* op, uint8_t* output, int num);
void Enc_KEY_Expansion(uint8_t* mk, uint8_t rk[13][16]);
void Dec_KEY_Expansion(uint8_t* mk, uint8_t dk[13][16]);
void ARIA_EnCrypt(uint8_t* pt, uint8_t* ct, uint8_t* key);
void ARIA_DeCrypt(uint8_t* ct, uint8_t* pt, uint8_t* key);

//CTR
void CTR_INC(uint8_t* ICTR);
void* ARIA_CTR(void* arg);

//SHA_256
void cut_st(uint8_t* input, uint32_t* output);
void SHA256_padding(uint8_t* input, uint32_t pt_len, uint32_t MSG_LEN);
void SHA256(uint8_t* MSG, uint32_t MSG_len, uint8_t* output);
void WordToByte(uint32_t* input, uint8_t* output);

//HMAC
void HMAC(uint8_t* text, uint32_t text_len, uint8_t* h_key, int keylen, uint8_t* output);

//NETWORK
void* handle_clnt(void* arg);
void* send_msg(void* arg);
void* recv_msg(void* arg);
void ErrorHandling(char* message);
