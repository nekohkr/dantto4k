
//#include "StdAfx.h" Not use precompiled headers, the options are located under Configuration Properties > C/C++ > Precompiled Headers. 

//#include "platcomm.h" //needed for dumpTS, cause error for acas procesing


#include "a_cas_card.h"
#include "a_cas_card_error_code.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>
#define SHA256_DIGEST_LENGTH 32
//https://slproweb.com/products/Win32OpenSSL.html
//Win32 OpenSSL v3.2.1   To compile by visual studio, add folders "include" and "lib" in VB project

#include <winscard.h>
#if defined(_WIN32)
#include <windows.h>
#include <tchar.h>
#else
#define TCHAR char
#define _tcslen strlen
#endif

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 inner structures
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
typedef struct {

	SCARDCONTEXT       mng;
	SCARDHANDLE        card;

	uint8_t* pool;
	LPTSTR             reader;

	uint8_t* sbuf;
	uint8_t* rbuf;
	uint8_t* hash;

	A_CAS_INIT_STATUS  stat;

	A_CAS_ID           id;
	int32_t            id_max;

	CARD_SCRAMBLEKEY   ckc;

	A_CAS_PWR_ON_CTRL_INFO pwc;
	int32_t            pwc_max;



} A_CAS_CARD_PRIVATE_DATA;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 constant values
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static const uint8_t INITIAL_SETTING_CONDITIONS_CMD[] = {
	0x90, 0x30, 0x00, 0x01, 0x00,
};

static const uint8_t CARD_ID_INFORMATION_ACQUIRE_CMD[] = {
	0x90, 0x32, 0x00, 0x01, 0x00,
};

static const uint8_t POWER_ON_CONTROL_INFORMATION_REQUEST_CMD[] = {
	0x90, 0x80, 0x00, 0x01, 0x01, 0x00, 0x00,
};

static const uint8_t ECM_RECEIVE_CMD_HEADER[] = {
	0x90, 0x34, 0x00, 0x01,
};

static const uint8_t EMM_RECEIVE_CMD_HEADER[] = {
	0x90, 0x36, 0x00, 0x01,
};

static const uint8_t CARD_SCRAMBLEKEY_SET_CMD[] = {
	0x90, 0xA0, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x8A, 0xF7,/////////////////////
};

static const uint8_t CARD_SCRAMBLEKEY_PUB_KEY[] = {//random values
	0xD9, 0x25, 0xd8, 0x82, 0x17, 0x67, 0x41, 0x49,
};

static const uint8_t CARD_MASTER_KEY[] = {
		0x4F, 0x4C, 0x7C, 0xEB, 0x34, 0xFE, 0xB0, 0xA3,
		0x1E, 0x41, 0x19, 0x51, 0xE1, 0x35, 0x15, 0x12,
		0x87, 0xD3, 0x3D, 0x33, 0xD4, 0x9B, 0x4F, 0x52,
		0x05, 0x77, 0xF9, 0xEF, 0xE5, 0x56, 0x1F, 0x32,
};

#define A_CAS_BUFFER_MAX (4*1024)


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 function prottypes (interface method)
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static void release_a_cas_card(void* acas);
static int init_a_cas_card(void* acas);
static int re_init_a_cas_card(void* acas);
static int get_init_status_a_cas_card(void* acas, A_CAS_INIT_STATUS* stat);
static int get_id_a_cas_card(void* acas, A_CAS_ID* dst);
static int get_pwr_on_ctrl_a_cas_card(void* acas, A_CAS_PWR_ON_CTRL_INFO* dst);
static int proc_ecm_a_cas_card(void* acas, A_CAS_ECM_RESULT* dst, uint8_t* src, int len);
static int proc_emm_a_cas_card(void* acas, uint8_t* src, int len);
static int scramble_key_cas_card(void* acas, A_CAS_ID* casid, CARD_SCRAMBLEKEY* dst);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 global function implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
A_CAS_CARD* create_a_cas_card(void)
{
	int n;

	A_CAS_CARD* r;
	A_CAS_CARD_PRIVATE_DATA* prv;

	n = sizeof(A_CAS_CARD) + sizeof(A_CAS_CARD_PRIVATE_DATA);
	prv = (A_CAS_CARD_PRIVATE_DATA*)calloc(1, n);
	if (prv == NULL) {
		return NULL;
	}

	r = (A_CAS_CARD*)(prv + 1);

	r->private_data = prv;

	r->release = release_a_cas_card;
	r->init = init_a_cas_card;
	r->re_init = re_init_a_cas_card;
	r->get_init_status = get_init_status_a_cas_card;
	r->get_id = get_id_a_cas_card;
	r->get_pwr_on_ctrl = get_pwr_on_ctrl_a_cas_card;
	r->proc_ecm = proc_ecm_a_cas_card;
	r->proc_emm = proc_emm_a_cas_card;
	r->scramble_key = scramble_key_cas_card;

	return r;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 function prottypes (private method)
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static A_CAS_CARD_PRIVATE_DATA* private_data(void* acas);
static void teardown(A_CAS_CARD_PRIVATE_DATA* prv);
static int change_id_max(A_CAS_CARD_PRIVATE_DATA* prv, int max);
static int change_pwc_max(A_CAS_CARD_PRIVATE_DATA* prv, int max);
static int connect_card(A_CAS_CARD_PRIVATE_DATA* prv, LPCTSTR reader_name);
static void extract_power_on_ctrl_response(A_CAS_PWR_ON_CTRL* dst, uint8_t* src);
static void extract_mjd(int* yy, int* mm, int* dd, int mjd);
static int setup_ecm_receive_command(uint8_t* dst, uint8_t* src, int len);
static int setup_emm_receive_command(uint8_t* dst, uint8_t* src, int len);
static int32_t load_be_uint16(uint8_t* p);
static int64_t load_be_uint48(uint8_t* p);
static int64_t load_be_uint64(uint8_t* p);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 interface method implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static void release_a_cas_card(void* acas)
{
	A_CAS_CARD_PRIVATE_DATA* prv;

	prv = private_data(acas);
	if (prv == NULL) {
		/* do nothing */
		return;
	}

	teardown(prv);
	free(prv);
}

static int init_a_cas_card(void* acas)
{
	int m;
	long ret;
	unsigned long len;

	A_CAS_CARD_PRIVATE_DATA* prv;

	prv = private_data(acas);
	if (prv == NULL) {
		return A_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	teardown(prv);

	ret = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &(prv->mng));
	if (ret != SCARD_S_SUCCESS) {
		return A_CAS_CARD_ERROR_NO_SMART_CARD_READER;
	}

	ret = SCardListReaders(prv->mng, NULL, NULL, &len);
	if (ret != SCARD_S_SUCCESS) {
		return A_CAS_CARD_ERROR_NO_SMART_CARD_READER;
	}
	len += 256;

	m = (sizeof(TCHAR) * len) + (2 * A_CAS_BUFFER_MAX) + (sizeof(int64_t) * 16) + (sizeof(A_CAS_PWR_ON_CTRL) * 16);
	prv->pool = (uint8_t*)malloc(m);
	if (prv->pool == NULL) {
		return A_CAS_CARD_ERROR_NO_ENOUGH_MEMORY;
	}

	prv->reader = (LPTSTR)(prv->pool);
	prv->sbuf = (uint8_t*)(prv->reader + len);
	prv->rbuf = prv->sbuf + A_CAS_BUFFER_MAX;
	prv->hash = prv->rbuf + A_CAS_BUFFER_MAX;
	prv->id.data = (int64_t*)(prv->hash + A_CAS_BUFFER_MAX);
	prv->id_max = 16;
	prv->ckc.data = (uint8_t*)(prv->id.data + A_CAS_BUFFER_MAX); //prv->hash + A_CAS_BUFFER_MAX; //(int8_t*)(prv->hash + A_CAS_BUFFER_MAX);
	prv->pwc.data = (A_CAS_PWR_ON_CTRL*)(prv->ckc.data + prv->id_max);
	prv->pwc_max = 16;



	ret = SCardListReaders(prv->mng, NULL, prv->reader, &len);
	if (ret != SCARD_S_SUCCESS) {
		return A_CAS_CARD_ERROR_NO_SMART_CARD_READER;
	}

	while (prv->reader[0] != 0) {
		if (connect_card(prv, prv->reader)) {
			break;
		}
		prv->reader += (_tcslen(prv->reader) + 1);
	}

	if (prv->card == 0) {
		return A_CAS_CARD_ERROR_ALL_READERS_CONNECTION_FAILED;
	}

	return 0;
}

static int re_init_a_cas_card(void* acas)
{
	int m;
	long ret;
	unsigned long len;

	A_CAS_CARD_PRIVATE_DATA* prv;

	prv = private_data(acas);
	if (prv == NULL) {
		return A_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	ret = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &(prv->mng));
	if (ret != SCARD_S_SUCCESS) {
		return A_CAS_CARD_ERROR_NO_SMART_CARD_READER;
	}

	ret = SCardListReaders(prv->mng, NULL, NULL, &len);
	if (ret != SCARD_S_SUCCESS) {
		return A_CAS_CARD_ERROR_NO_SMART_CARD_READER;
	}


	ret = SCardListReaders(prv->mng, NULL, prv->reader, &len);
	if (ret != SCARD_S_SUCCESS) {
		return A_CAS_CARD_ERROR_NO_SMART_CARD_READER;
	}

	while (prv->reader[0] != 0) {
		if (connect_card(prv, prv->reader)) {
			break;
		}
		prv->reader += (_tcslen(prv->reader) + 1);
	}

	if (prv->card == 0) {
		return A_CAS_CARD_ERROR_ALL_READERS_CONNECTION_FAILED;
	}

	return 0;
}

static int get_init_status_a_cas_card(void* acas, A_CAS_INIT_STATUS* stat)
{
	A_CAS_CARD_PRIVATE_DATA* prv;

	prv = private_data(acas);
	if ((prv == NULL) || (stat == NULL)) {
		return A_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	if (prv->card == 0) {
		return A_CAS_CARD_ERROR_NOT_INITIALIZED;
	}

	memcpy(stat, &(prv->stat), sizeof(A_CAS_INIT_STATUS));

	return 0;
}

static int get_id_a_cas_card(void* acas, A_CAS_ID* dst)
{
	long ret;

	unsigned long slen;
	unsigned long rlen;

	int i, num;

	uint8_t* p;
	uint8_t* tail;

	A_CAS_CARD_PRIVATE_DATA* prv;

	prv = private_data(acas);
	if ((prv == NULL) || (dst == NULL)) {
		return A_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	if (prv->card == 0) {
		return A_CAS_CARD_ERROR_NOT_INITIALIZED;
	}

	slen = sizeof(CARD_ID_INFORMATION_ACQUIRE_CMD);
	memcpy(prv->sbuf, CARD_ID_INFORMATION_ACQUIRE_CMD, slen);
	rlen = A_CAS_BUFFER_MAX;

	ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, slen, NULL, prv->rbuf, &rlen);
	if ((ret != SCARD_S_SUCCESS) || (rlen < 19)) {
		return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
	}

	p = prv->rbuf + 6;
	tail = prv->rbuf + rlen;
	if (p + 1 > tail) {
		return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
	}

	num = p[0];
	if (num > prv->id_max) {
		if (change_id_max(prv, num + 4) < 0) {
			return A_CAS_CARD_ERROR_NO_ENOUGH_MEMORY;
		}
	}

	p += 1;
	for (i = 0; i < num; i++) {
		if (p + 10 > tail) {
			return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
		}

		prv->id.data[i] = load_be_uint64(p + 0);// load_be_uint48(p+2);
		p += 10;
	}

	prv->id.count = num;

	memcpy(dst, &(prv->id), sizeof(A_CAS_ID));

	return 0;
}

static int scramble_key_cas_card(void* acas, A_CAS_ID* casid, CARD_SCRAMBLEKEY* dst)
{
	long ret;

	unsigned long slen;
	unsigned long rlen;


	uint8_t* p;
	uint8_t* tail;
	uint64_t temp;
	unsigned char hashed[SHA256_DIGEST_LENGTH];
	unsigned char hashckc[SHA256_DIGEST_LENGTH];
	int i, r;

	A_CAS_CARD_PRIVATE_DATA* prv;

	prv = private_data(acas);
	if ((prv == NULL) || (dst == NULL)) {
		return A_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	if (prv->card == 0) {
		return A_CAS_CARD_ERROR_NOT_INITIALIZED;
	}

	r = sizeof(CARD_SCRAMBLEKEY_SET_CMD);
	memcpy(prv->sbuf + 0, CARD_SCRAMBLEKEY_SET_CMD, r);
	memcpy(prv->sbuf + r, CARD_SCRAMBLEKEY_PUB_KEY, 8);//8byte
	r += 8;
	prv->sbuf[r] = 0;
	r += 1;

	slen = r;
	rlen = A_CAS_BUFFER_MAX;

	ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, slen, NULL, prv->rbuf, &rlen);
	if ((ret != SCARD_S_SUCCESS) || (rlen < 10)) {
		return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
	}

	// Compute SHA-256 hash
	p = prv->rbuf + 6;
	r = sizeof(CARD_MASTER_KEY);
	memcpy(prv->hash + 0, CARD_MASTER_KEY, r);
	memcpy(prv->hash + r, CARD_SCRAMBLEKEY_PUB_KEY, 8);//8 bytes copy
	r += 8;

	memcpy(prv->hash + r, p, 8);//8 bytes copy from prv->rbuf
	r += 8;
	//SHA256((unsigned char* ) data, strlen(data), hash);
	SHA256(prv->hash, (size_t)r, hashckc);
	//hash again
	memcpy(prv->hash + 0, hashckc, SHA256_DIGEST_LENGTH);
	memcpy(prv->hash + SHA256_DIGEST_LENGTH, CARD_SCRAMBLEKEY_PUB_KEY, 8);
	r = SHA256_DIGEST_LENGTH + 8;
	SHA256(prv->hash, (size_t)r, hashed);

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		if (prv->rbuf[i + 0x0e] != hashed[i]) { return A_CAS_CARD_ERROR_NOT_INITIALIZED; }//confirm hash
		prv->ckc.data[i] = hashckc[i];
	}
	prv->ckc.count = SHA256_DIGEST_LENGTH;
	memcpy(dst, &(prv->ckc), sizeof(CARD_SCRAMBLEKEY));

	return 0;
}

static int get_pwr_on_ctrl_a_cas_card(void* acas, A_CAS_PWR_ON_CTRL_INFO* dst)
{
	long ret;

	unsigned long slen;
	unsigned long rlen;

	int i, num, code;

	A_CAS_CARD_PRIVATE_DATA* prv;

	memset(dst, 0, sizeof(A_CAS_PWR_ON_CTRL_INFO));

	prv = private_data(acas);
	if ((prv == NULL) || (dst == NULL)) {
		return A_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	if (prv->card == 0) {
		return A_CAS_CARD_ERROR_NOT_INITIALIZED;
	}

	slen = sizeof(POWER_ON_CONTROL_INFORMATION_REQUEST_CMD);
	memcpy(prv->sbuf, POWER_ON_CONTROL_INFORMATION_REQUEST_CMD, slen);
	prv->sbuf[5] = 0;
	rlen = A_CAS_BUFFER_MAX;

	ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, slen, NULL, prv->rbuf, &rlen);
	if ((ret != SCARD_S_SUCCESS) || (rlen < 18) || (prv->rbuf[6] != 0)) {
		return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
	}

	code = load_be_uint16(prv->rbuf + 4);
	if (code == 0xa101) {
		/* no data */
		return 0;
	}
	else if (code != 0x2100) {
		return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
	}

	num = (prv->rbuf[7] + 1);
	if (prv->pwc_max < num) {
		if (change_pwc_max(prv, num + 4) < 0) {
			return A_CAS_CARD_ERROR_NO_ENOUGH_MEMORY;
		}
	}

	extract_power_on_ctrl_response(prv->pwc.data + 0, prv->rbuf);

	for (i = 1; i < num; i++) {
		prv->sbuf[5] = (uint8_t)i;
		rlen = A_CAS_BUFFER_MAX;

		ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, slen, NULL, prv->rbuf, &rlen);
		if ((ret != SCARD_S_SUCCESS) || (rlen < 18) || (prv->rbuf[6] != i)) {
			return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
		}

		extract_power_on_ctrl_response(prv->pwc.data + i, prv->rbuf);
	}

	prv->pwc.count = num;

	memcpy(dst, &(prv->pwc), sizeof(A_CAS_PWR_ON_CTRL_INFO));

	return 0;
}

static int proc_ecm_a_cas_card(void* acas, A_CAS_ECM_RESULT* dst, uint8_t* src, int len)
{
	int retry_count, r, i;

	long ret;
	unsigned long slen;
	unsigned long rlen;
	unsigned char hashed[SHA256_DIGEST_LENGTH];
	uint8_t* ecmResponse;

	A_CAS_CARD_PRIVATE_DATA* prv;

	prv = private_data(acas);
	if ((prv == NULL) ||
		(dst == NULL) ||
		(src == NULL) ||
		(len < 1)) {
		return A_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	if (prv->card == 0) {
		return A_CAS_CARD_ERROR_NOT_INITIALIZED;
	}

	slen = setup_ecm_receive_command(prv->sbuf, src, len);
	rlen = A_CAS_BUFFER_MAX;

	retry_count = 0;
	ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, slen, NULL, prv->rbuf, &rlen);
	while (((ret != SCARD_S_SUCCESS) || (rlen < 25)) && (retry_count < 2)) {
		retry_count += 1;
		rlen = A_CAS_BUFFER_MAX;

		ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, slen, NULL, prv->rbuf, &rlen);
	}

	if ((ret != SCARD_S_SUCCESS) || (rlen < 25)) {
		return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
	}

	ecmResponse = prv->rbuf + 6;
	memcpy(prv->hash + 0, prv->ckc.data, SHA256_DIGEST_LENGTH);
	memcpy(prv->hash + SHA256_DIGEST_LENGTH, src + 4, 23);
	r = SHA256_DIGEST_LENGTH + 23;
	SHA256(prv->hash, (size_t)r, hashed);
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		hashed[i] ^= ecmResponse[i];
	}

	memcpy(dst->scramble_key, hashed, 32);
	dst->return_code = 1;// load_be_uint16(prv->rbuf + 4);

	return 0;
}

static int proc_emm_a_cas_card(void* acas, uint8_t* src, int len)
{
	int retry_count;

	long ret;
	unsigned long slen;
	unsigned long rlen;

	A_CAS_CARD_PRIVATE_DATA* prv;

	prv = private_data(acas);
	if ((prv == NULL) ||
		(src == NULL) ||
		(len < 1)) {
		return A_CAS_CARD_ERROR_INVALID_PARAMETER;
	}

	if (prv->card == 0) {
		return A_CAS_CARD_ERROR_NOT_INITIALIZED;
	}

	slen = setup_emm_receive_command(prv->sbuf, src, len);
	rlen = A_CAS_BUFFER_MAX;

	retry_count = 0;
	ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, slen, NULL, prv->rbuf, &rlen);
	while (((ret != SCARD_S_SUCCESS) || (rlen < 6)) && (retry_count < 2)) {
		retry_count += 1;
		//		if(!connect_card(prv, prv->reader)){
		//			continue;
		//		}
		//		slen = setup_emm_receive_command(prv->sbuf, src, len);
		rlen = A_CAS_BUFFER_MAX;

		ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, slen, NULL, prv->rbuf, &rlen);
	}

	if ((ret != SCARD_S_SUCCESS) || (rlen < 6)) {
		return A_CAS_CARD_ERROR_TRANSMIT_FAILED;
	}

	return 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 private method implementation
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static A_CAS_CARD_PRIVATE_DATA* private_data(void* acas)
{
	A_CAS_CARD_PRIVATE_DATA* r;
	A_CAS_CARD* p;

	p = (A_CAS_CARD*)acas;
	if (p == NULL) {
		return NULL;
	}

	r = (A_CAS_CARD_PRIVATE_DATA*)(p->private_data);
	if (((void*)(r + 1)) != ((void*)p)) {
		return NULL;
	}

	return r;
}

static void teardown(A_CAS_CARD_PRIVATE_DATA* prv)
{
	if (prv->card != 0) {
		SCardDisconnect(prv->card, SCARD_LEAVE_CARD);
		prv->card = 0;
	}

	if (prv->mng != 0) {
		SCardReleaseContext(prv->mng);
		prv->mng = 0;
	}

	if (prv->pool != NULL) {
		free(prv->pool);
		prv->pool = NULL;
	}

	prv->reader = NULL;
	prv->sbuf = NULL;
	prv->rbuf = NULL;
	prv->id.data = NULL;
	prv->id_max = 0;
}

static int change_id_max(A_CAS_CARD_PRIVATE_DATA* prv, int max)
{
	intptr_t m;
	intptr_t reader_size;
	int pwctrl_size;

	uint8_t* p;
	uint8_t* old_reader;
	uint8_t* old_pwctrl;

	reader_size = prv->sbuf - prv->pool;
	pwctrl_size = prv->pwc.count * sizeof(A_CAS_PWR_ON_CTRL);

	m = reader_size;
	m += (2 * A_CAS_BUFFER_MAX);
	m += (max * sizeof(int64_t));
	m += (prv->pwc_max * sizeof(A_CAS_PWR_ON_CTRL));
	p = (uint8_t*)malloc(m);
	if (p == NULL) {
		return A_CAS_CARD_ERROR_NO_ENOUGH_MEMORY;
	}

	old_reader = (uint8_t*)(prv->reader);
	old_pwctrl = (uint8_t*)(prv->pwc.data);

	prv->reader = (LPTSTR)p;
	prv->sbuf = p + reader_size;
	prv->rbuf = prv->sbuf + A_CAS_BUFFER_MAX;
	prv->id.data = (int64_t*)(prv->rbuf + A_CAS_BUFFER_MAX);
	prv->id_max = max;
	prv->pwc.data = (A_CAS_PWR_ON_CTRL*)(prv->id.data + prv->id_max);

	memcpy(prv->reader, old_reader, reader_size);
	memcpy(prv->pwc.data, old_pwctrl, pwctrl_size);

	free(prv->pool);
	prv->pool = p;

	return 0;
}

static int change_pwc_max(A_CAS_CARD_PRIVATE_DATA* prv, int max)
{
	intptr_t m;
	intptr_t reader_size;
	int cardid_size;

	uint8_t* p;
	uint8_t* old_reader;
	uint8_t* old_cardid;

	reader_size = prv->sbuf - prv->pool;
	cardid_size = prv->id.count * sizeof(int64_t);

	m = reader_size;
	m += (2 * A_CAS_BUFFER_MAX);
	m += (prv->id_max * sizeof(int64_t));
	m += (max * sizeof(A_CAS_PWR_ON_CTRL));
	p = (uint8_t*)malloc(m);
	if (p == NULL) {
		return A_CAS_CARD_ERROR_NO_ENOUGH_MEMORY;
	}

	old_reader = (uint8_t*)(prv->reader);
	old_cardid = (uint8_t*)(prv->id.data);

	prv->reader = (LPTSTR)p;
	prv->sbuf = p + reader_size;
	prv->rbuf = prv->sbuf + A_CAS_BUFFER_MAX;
	prv->id.data = (int64_t*)(prv->rbuf + A_CAS_BUFFER_MAX);
	prv->pwc.data = (A_CAS_PWR_ON_CTRL*)(prv->id.data + prv->id_max);
	prv->pwc_max = max;

	memcpy(prv->reader, old_reader, reader_size);
	memcpy(prv->id.data, old_cardid, cardid_size);

	free(prv->pool);
	prv->pool = p;

	return 0;
}

static int connect_card(A_CAS_CARD_PRIVATE_DATA* prv, LPCTSTR reader_name)
{
	int m, n;

	long ret;
	unsigned long rlen, protocol;

	uint8_t* p;

	if (prv->card != 0) {
		SCardDisconnect(prv->card, SCARD_RESET_CARD);
		prv->card = 0;
	}

	ret = SCardConnect(prv->mng, reader_name, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1, &(prv->card), &protocol);
	if (ret != SCARD_S_SUCCESS) {
		return 0;
	}

	m = sizeof(INITIAL_SETTING_CONDITIONS_CMD);
	memcpy(prv->sbuf, INITIAL_SETTING_CONDITIONS_CMD, m);
	rlen = A_CAS_BUFFER_MAX;
	ret = SCardTransmit(prv->card, SCARD_PCI_T1, prv->sbuf, m, NULL, prv->rbuf, &rlen);
	if (ret != SCARD_S_SUCCESS) {
		return 0;
	}

	if (rlen < 17) {
		return 0;
	}

	p = prv->rbuf;

	n = load_be_uint16(p + 4);
	if (n != 0x2100) { // return code missmatch
		return 0;
	}

	//memcpy(prv->stat.system_key, p+16, 32);
	memcpy(prv->stat.init_cbc, p + 17, 2);//System management id
	prv->stat.acas_card_id = load_be_uint48(p + 8);
	prv->stat.card_status = load_be_uint16(p + 2);//IC card instruction
	prv->stat.ca_system_id = load_be_uint16(p + 6);

	return 1;
}

static void extract_power_on_ctrl_response(A_CAS_PWR_ON_CTRL* dst, uint8_t* src)
{
	int referrence;
	int start;
	int limit;

	dst->broadcaster_group_id = src[8];
	referrence = (src[9] << 8) | src[10];
	start = referrence - src[11];
	limit = start + (src[12] - 1);

	extract_mjd(&(dst->s_yy), &(dst->s_mm), &(dst->s_dd), start);
	extract_mjd(&(dst->l_yy), &(dst->l_mm), &(dst->l_dd), limit);

	dst->hold_time = src[13];
	dst->network_id = (src[14] << 8) | src[15];
	dst->transport_id = (src[16] << 8) | src[17];
}

static void extract_mjd(int* yy, int* mm, int* dd, int mjd)
{
	int a1, m1;
	int a2, m2;
	int a3, m3;
	int a4, m4;
	int mw;
	int dw;
	int yw;

	mjd -= 51604; // 2000,3/1
	if (mjd < 0) {
		mjd += 0x10000;
	}

	a1 = mjd / 146097;
	m1 = mjd % 146097;
	a2 = m1 / 36524;
	m2 = m1 - (a2 * 36524);
	a3 = m2 / 1461;
	m3 = m2 - (a3 * 1461);
	a4 = m3 / 365;
	if (a4 > 3) {
		a4 = 3;
	}
	m4 = m3 - (a4 * 365);

	mw = (1071 * m4 + 450) >> 15;
	dw = m4 - ((979 * mw + 16) >> 5);

	yw = a1 * 400 + a2 * 100 + a3 * 4 + a4 + 2000;
	mw += 3;
	if (mw > 12) {
		mw -= 12;
		yw += 1;
	}
	dw += 1;

	*yy = yw;
	*mm = mw;
	*dd = dw;
}

static int setup_ecm_receive_command(uint8_t* dst, uint8_t* src, int len)
{
	int r;

	r = sizeof(ECM_RECEIVE_CMD_HEADER);
	memcpy(dst + 0, ECM_RECEIVE_CMD_HEADER, r);
	dst[r] = (uint8_t)(len & 0xff);
	r += 1;
	memcpy(dst + r, src, len);
	r += len;
	dst[r] = 0;
	r += 1;
	return r;
}

static int setup_emm_receive_command(uint8_t* dst, uint8_t* src, int len)
{
	int r;

	r = sizeof(EMM_RECEIVE_CMD_HEADER);
	memcpy(dst + 0, EMM_RECEIVE_CMD_HEADER, r);
	dst[r] = (uint8_t)(len & 0xff);
	r += 1;
	memcpy(dst + r, src, len);
	r += len;
	dst[r] = 0;
	r += 1;

	return r;
}

static int32_t load_be_uint16(uint8_t* p)
{
	return ((p[0] << 8) | p[1]);
}

static int64_t load_be_uint48(uint8_t* p)
{
	int i;
	int64_t r;

	r = p[0];
	for (i = 1; i < 6; i++) {
		r <<= 8;
		r |= p[i];
	}

	return r;
}


static int64_t load_be_uint64(uint8_t* p)
{
	int i;
	int64_t r;

	r = p[0];
	for (i = 1; i < 8; i++) {
		r <<= 8;
		r |= p[i];
	}

	return r;
}