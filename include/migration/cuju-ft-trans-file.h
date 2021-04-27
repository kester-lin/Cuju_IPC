/*
 *
 * Copyright (c) 2017 ITRI
 *
 * Authors:
 *  Yi-feng Sun         <pkusunyifeng@gmail.com>
 *  Wei-Chen Liao       <ms0472904@gmail.com>
 *  Po-Jui Tsao         <pjtsao@itri.org.tw>
 *  Yu-Shiang Lin       <YuShiangLin@itri.org.tw> 
 *
 */

#ifndef QEMU_CUJU_FT_TRANS_FILE_H
#define QEMU_CUJU_FT_TRANS_FILE_H

#include "hw/hw.h"

enum CUJU_QEMU_VM_TRANSACTION_STATE {
    CUJU_QEMU_VM_TRANSACTION_NACK = -1,
    CUJU_QEMU_VM_TRANSACTION_INIT,
    CUJU_QEMU_VM_TRANSACTION_BEGIN,
    CUJU_QEMU_VM_TRANSACTION_CONTINUE,
    CUJU_QEMU_VM_TRANSACTION_COMMIT,
    CUJU_QEMU_VM_TRANSACTION_CANCEL,
    CUJU_QEMU_VM_TRANSACTION_ATOMIC, // 5
    CUJU_QEMU_VM_TRANSACTION_ACK,
    CUJU_QEMU_VM_TRANSACTION_ACK1,
    CUJU_QEMU_VM_TRANSACTION_COMMIT1,
    CUJU_QEMU_VM_TRANSACTION_NOCOPY,
    CUJU_QEMU_VM_TRANSACTION_DEV_HEADER,
    CUJU_QEMU_VM_TRANSACTION_DEV_STATES,
    CUJU_QEMU_VM_TRANSACTION_ALIVE,
    CUJU_QEMU_VM_TRANSACTION_CHECKALIVE
};

/*
this variable is for CUJU_QEMU_VM_TRANSACTION_STATE 
and this command in our TCP transmission is 16 bit
Only Sending when hmp 'cuju-migrate-cancel'
*/   
#define CUJU_FT_ALIVE_HEADER 15     

enum CUJU_FT_MODE {
    CUJU_FT_ERROR = -1,
    CUJU_FT_OFF,
    CUJU_FT_INIT,                    // 1
    CUJU_FT_TRANSACTION_PRE_RUN,
    CUJU_FT_TRANSACTION_ITER,
    CUJU_FT_TRANSACTION_ATOMIC,
    CUJU_FT_TRANSACTION_RECV,        // 5
    CUJU_FT_TRANSACTION_HANDOVER,
    CUJU_FT_TRANSACTION_SPECULATIVE,
    CUJU_FT_TRANSACTION_FLUSH_OUTPUT,
    CUJU_FT_TRANSACTION_TRANSFER,
    CUJU_FT_TRANSACTION_SNAPSHOT,    // 10
    CUJU_FT_TRANSACTION_RUN,
};

extern enum CUJU_FT_MODE cuju_ft_mode;

typedef ssize_t (CujuFtTransPutBufferFunc)(void *opaque, const void *data, size_t size);
typedef int (CujuFtTransGetBufferFunc)(void *opaque, uint8_t *buf, int64_t pos, size_t size);
typedef ssize_t (CujuFtTransPutVectorFunc)(void *opaque, const struct iovec *iov, int iovcnt);
typedef int (CujuFtTransPutReadyFunc)(void);
typedef int (CujuFtTransGetReadyFunc)(void *opaque);
typedef void (CujuFtTransWaitForUnfreezeFunc)(void *opaque);
typedef int (CujuFtTransCloseFunc)(void *opaque);


/* ipc_mode */
extern char *haproxy_ipc;
extern char *incoming;

#define IPC_PACKET_CNT 1
#define IPC_PACKET_SIZE 2
#define IPC_TIME 3
#define IPC_SIGNAL 4
#define IPC_CUJU 5

#define SEND_SAME_IP 1
#define IP_LENGTH 4
#define DEFAULT_NIC_CNT 3
#define CONNECTION_LENGTH 12
#define DEFAULT_CONN_CNT 3
#define TOTAL_NIC   8
#define TOTAL_CONN   8
//#define DEFAULT_IPC_ARRAY  (24 + (IP_LENGTH*DEFAULT_NIC_CNT) + (CONNECTION_LENGTH * DEFAULT_CONN_CNT))

#define DEFAULT_IPC_ARRAY  (20 + (IP_LENGTH*TOTAL_NIC) + (CONNECTION_LENGTH * TOTAL_CONN))



/* IPC PROTO */
struct proto_ipc
{
    uint32_t ipc_mode:8;
    uint32_t cuju_ft_arp:2;
    uint32_t cuju_ft_mode:6;
    uint32_t gft_id:16;
    uint32_t epoch_id;
    uint32_t flush_id;
    uint32_t packet_cnt:16;
    uint32_t packet_size:16;
    uint32_t time_interval;
    uint32_t nic_count:16;
    uint32_t conn_count:16;
    unsigned int nic[TOTAL_NIC];
    unsigned char conn[TOTAL_CONN][CONNECTION_LENGTH];   
};

/* a list of buf to be sent */
struct cuju_buf_desc {
    void *opaque;
    void *buf;
    size_t size;
    size_t off;
    struct cuju_buf_desc *next;
};

typedef struct CujuFtTransHdr
{
    uint64_t serial;
    uint32_t magic;
    uint32_t payload_len;
    uint16_t cmd;
    uint16_t id;
    uint16_t seq;
} CujuFtTransHdr;

#define CUJU_FT_HDR_MAGIC    0xa5a6a7a8
#define ENABLE_LOOP_SEND_IP 0

typedef struct CujuQEMUFileFtTrans
{
    CujuFtTransPutBufferFunc *put_buffer;
    CujuFtTransGetBufferFunc *get_buffer;
    CujuFtTransPutReadyFunc *put_ready;
    CujuFtTransGetReadyFunc *get_ready;
    CujuFtTransWaitForUnfreezeFunc *wait_for_unfreeze;
    CujuFtTransCloseFunc *close;
    void *opaque;
    QEMUFile *file;

    unsigned long ft_serial;

    enum CUJU_QEMU_VM_TRANSACTION_STATE state;
    uint32_t seq;
    uint16_t id;

    int has_error;

    bool freeze_output;
    bool freeze_input;
    bool is_sender;
    bool is_payload;

    uint8_t *buf;
    size_t buf_max_size;
    size_t put_offset;
    size_t get_offset;

    struct cuju_buf_desc _buf_header;
    struct cuju_buf_desc *buf_header;
    struct cuju_buf_desc *buf_tail;

    CujuFtTransHdr header;
    size_t header_offset;

    int last_cmd;
    int index;

    void *ram_buf;
    int ram_buf_size;
    int ram_buf_put_off;

    void *ram_hdr_buf;
    int ram_hdr_buf_size;
    int ram_hdr_buf_put_off;

    // sent by sender in COMMIT1.payload_len
    // -1 if not received.
    int ram_buf_expect;

    int ram_hdr_fd;

    int ram_fd;
    int ram_fd_recved;  // reset to -1
    int ram_fd_expect;  // reset to -1
    int ram_fd_ack;     // should ram_fd handler send back ack?
    bool cancel;
    bool check;
} CujuQEMUFileFtTrans;

void *cuju_process_incoming_thread(void *opaque);

extern void *cuju_ft_trans_s1;
extern void *cuju_ft_trans_s2;
extern void *cuju_ft_trans_curr;
extern void *cuju_ft_trans_next;

#define CUJU_FT_TRANS_ERR_UNKNOWN        0x01 /* Unknown error */
#define CUJU_FT_TRANS_ERR_SEND_HDR       0x02 /* Send header failed */
#define CUJU_FT_TRANS_ERR_RECV_HDR       0x03 /* Recv header failed */
#define CUJU_FT_TRANS_ERR_SEND_PAYLOAD   0x04 /* Send payload failed */
#define CUJU_FT_TRANS_ERR_RECV_PAYLOAD   0x05
#define CUJU_FT_TRANS_ERR_FLUSH          0x06 /* Flush buffered data failed */
#define CUJU_FT_TRANS_ERR_STATE_INVALID  0x07 /* Invalid state */


void cuju_ft_trans_init(void);
void cuju_ft_trans_set(int index, void *s);
void cuju_ft_trans_extend(void *opaque);

int cuju_ft_trans_commit1(void *opaque, int ram_len, unsigned long serial);
int cuju_ft_trans_flush_output(void *opaque);
int cuju_ft_trans_begin(void *opaque);
int cuju_ft_trans_commit(void *opaque);
int cuju_ft_trans_cancel(void *opaque);
int cuju_ft_trans_is_sender(void *opaque);
void cuju_ft_trans_set_buffer_mode(int on);
int cuju_ft_trans_is_buffer_mode(void);
void cuju_ft_trans_flush_buffer(void *opaque);
void cuju_ft_trans_init_buf_desc(QemuMutex *mutex, QemuCond *cond);
void cuju_ft_trans_flush_buf_desc(void *opaque);
int cuju_ft_trans_receive_ack1(void *opaque);
int cuju_ft_trans_recv_ack(void *opaque);
int cuju_ft_trans_send_begin(void *opaque);
void cuju_qemu_set_last_cmd(void *file, int cmd);

void cuju_ft_trans_read_pages(void *opaque);
void cuju_ft_trans_skip_pages(void *opaque);
void cuju_ft_trans_read_headers(void *opaque);
int cuju_ft_trans_send_header(CujuQEMUFileFtTrans *s,
                            enum CUJU_QEMU_VM_TRANSACTION_STATE state,
                            uint32_t payload_len);

QEMUFile *cuju_qemu_fopen_ops_ft_trans(void *opaque,
                                  CujuFtTransPutBufferFunc *put_buffer,
                                  CujuFtTransGetBufferFunc *get_buffer,
                                  CujuFtTransPutReadyFunc *put_ready,
                                  CujuFtTransGetReadyFunc *get_ready,
                                  CujuFtTransWaitForUnfreezeFunc *wait_for_unfreeze,
                                  CujuFtTransCloseFunc *close,
                                  bool is_sender,
                                  int ram_fd,
                                  int ram_hdr_fd);
extern int cuju_is_load;
extern QemuMutex cuju_load_mutex;
extern QemuCond cuju_load_cond;

void cuju_socket_set_nodelay(int fd);
void cuju_socket_unset_nodelay(int fd);
void cuju_socket_set_quickack(int fd);

int cuju_proxy_init(const char *p, int failover);
int cuju_proxy_ipc_send_cmd(char* addr, unsigned int epoch_id, unsigned int cuju_ft_mode);
void cuju_proxy_ipc_epoch_timer(unsigned int epoch_id);
void cuju_proxy_ipc_epoch_commit(unsigned int epoch_id);
void cuju_proxy_ipc_notify_snapshot(unsigned int epoch_id);
void cuju_proxy_ipc_notify_ft(unsigned int epoch_id);
void cuju_proxy_ipc_notify_failover(unsigned int epoch_id);
void cuju_proxy_ipc_init_info(unsigned int epoch_id);
void recv_snapshot_signal(void);
#if ENABLE_LOOP_SEND_IP 
int cuju_proxy_ipc_send_time_trig(char* addr);
#endif
int cuju_proxy_ipc_open_arp_file(void);
void cuju_proxy_ipc_close_arp_file(void);
char *arp_get_ip(const char *req_mac);
uint32_t parseIPV4string(char* str);
#endif
