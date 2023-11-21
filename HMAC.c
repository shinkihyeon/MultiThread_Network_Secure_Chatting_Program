#include "Net_Project.h"

//SHA256 ±â¹ÝÀÇ HMAC
void HMAC(uint8_t* text, uint32_t text_len, uint8_t* h_key, int keylen, uint8_t* output) {
	uint32_t cnt_i;
	//HMAC_KEY -> 64BYTES
	uint8_t K0[64] = { 0x00, };
	if (keylen > 64) {
		SHA256(h_key, keylen, K0);
	}
	else {
		memcpy(K0, h_key, sizeof(uint8_t) * keylen);
	}

	//K0 ^ IPAD
	uint8_t k0_ipad[64] = { 0x00, };
	memset(k0_ipad, 0x36, sizeof(uint8_t) * 64);
	for (cnt_i = 0; cnt_i < 64; cnt_i++) {
		k0_ipad[cnt_i] ^= K0[cnt_i];
	}

	//(K0 ^ IPAD) || TEXT
	uint8_t* tki = (uint8_t*)calloc((64 + text_len), sizeof(uint8_t));
	assert(tki != NULL);
	memcpy(tki, k0_ipad, sizeof(uint8_t) * 64);
	memcpy((tki + 64), text, sizeof(uint8_t) * text_len);

	//H((K0 ^ IPAD) || TEXT)
	uint8_t MID_SHA[32] = { 0x00, };
	SHA256(tki, (64 + text_len), MID_SHA);

	//K0 ^ opad
	uint8_t k0_opad[64] = { 0x00, };
	memset(k0_opad, 0x5c, sizeof(uint8_t) * 64);
	for (cnt_i = 0; cnt_i < 64; cnt_i++) {
		k0_opad[cnt_i] ^= K0[cnt_i];
	}

	//(K0 ^ opad) || H((K0 ^ IPAD) || TEXT)
	uint8_t last_value[96] = { 0x00, };
	memcpy(last_value, k0_opad, sizeof(uint8_t) * 64);
	memcpy((last_value + 64), MID_SHA, sizeof(uint8_t) * 32);

	//finalize
	uint8_t FN_SHA[32] = { 0x00, };
	SHA256(last_value, 96, FN_SHA);

	//output
	memcpy(output, FN_SHA, sizeof(uint8_t) * 32);

	//FREE
	free(tki);
	tki = NULL;
}

