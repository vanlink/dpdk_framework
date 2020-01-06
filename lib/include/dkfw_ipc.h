#ifndef _DPDK_FRAMEWORK_IPC_H
#define _DPDK_FRAMEWORK_IPC_H
#include <rte_ring.h>

#define IPC_TO_CORE_RING 0
#define IPC_TO_BACK_RING 1

#define IPC_RESULT_OK 0
#define IPC_RESULT_ERROE -1

typedef struct _DKFW_IPC_MSG_TAG {
    int core_role;
    int core_seq;

    int msg_type;
    uint32_t msg_id;

    int msg_result;

    uint64_t msg_timestamp;

    char msg_body[1];
} DKFW_IPC_MSG;

#define IPC_MSG_BODY(msg) &(msg)->msg_body[0]

extern int dkfw_ipc_client_init(char *nuique_name);
extern void dkfw_ipc_client_exit(void);
extern DKFW_IPC_MSG *dkfw_ipc_rcv_msg(void);
extern DKFW_IPC_MSG *dkfw_ipc_msg_alloc(void);
extern void dkfw_ipc_msg_free(DKFW_IPC_MSG *msg);
extern int dkfw_ipc_client_send_msg_to_core(int core_role, int core_seq, DKFW_IPC_MSG *msg);
extern DKFW_IPC_MSG *dkfw_ipc_client_rcv_msg_from_core(int core_role, int core_seq, int timeout);
extern int dkfw_ipc_send_response_msg(DKFW_IPC_MSG *msg);
#endif

