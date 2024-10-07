#ifndef A_CAS_CARD_H
#define A_CAS_CARD_H

//#include "portable.h"
#include <stdint.h>
#include <winscard.h>

typedef struct {
	uint8_t  system_key[32];
	uint8_t  init_cbc[8];
	int64_t  acas_card_id;
	int32_t  card_status;
	int32_t  ca_system_id;
} A_CAS_INIT_STATUS;

typedef struct {
	int64_t* data;
	int32_t  count;
} A_CAS_ID;

typedef struct {
	uint8_t* data;
	int32_t  count;
} CARD_SCRAMBLEKEY;

typedef struct {

	int32_t  s_yy; /* start date : year  */
	int32_t  s_mm; /* start date : month */
	int32_t  s_dd; /* start date : day   */

	int32_t  l_yy; /* limit date : year  */
	int32_t  l_mm; /* limit date : month */
	int32_t  l_dd; /* limit date : day   */

	int32_t  hold_time; /* in hour unit  */

	int32_t  broadcaster_group_id;

	int32_t  network_id;
	int32_t  transport_id;

} A_CAS_PWR_ON_CTRL;

typedef struct {
	A_CAS_PWR_ON_CTRL* data;
	int32_t  count;
} A_CAS_PWR_ON_CTRL_INFO;

typedef struct {
	uint8_t  scramble_key[32];
	uint32_t return_code;
} A_CAS_ECM_RESULT;

typedef struct {

	void* private_data;

	void (*release)(void* acas);

	int (*init)(void* acas);
	int (*re_init)(void* acas);
	int (*get_init_status)(void* acas, A_CAS_INIT_STATUS* stat);
	int (*get_id)(void* acas, A_CAS_ID* dst);
	int (*get_pwr_on_ctrl)(void* acas, A_CAS_PWR_ON_CTRL_INFO* dst);

	int (*proc_ecm)(void* acas, A_CAS_ECM_RESULT* dst, uint8_t* src, int len);
	int (*proc_emm)(void* acas, uint8_t* src, int len);

	int (*scramble_key)(void* acas, A_CAS_ID* casid, CARD_SCRAMBLEKEY* dst);

} A_CAS_CARD;

/*
typedef struct {

	SCARDCONTEXT       mng;
	SCARDHANDLE        card;

	uint8_t* pool;
	LPTSTR             reader;

	uint8_t* sbuf;
	uint8_t* rbuf;

	A_CAS_INIT_STATUS  stat;

	A_CAS_ID           id;
	int32_t            id_max;

	A_CAS_PWR_ON_CTRL_INFO pwc;
	int32_t            pwc_max;

} A_CAS_CARD_PRIVATE_DATA;*/

#ifdef __cplusplus
extern "C" {
#endif

	extern A_CAS_CARD* create_a_cas_card(void);

#ifdef __cplusplus
}
#endif

#endif /* A_CAS_CARD_H */