#ifndef STEWARD_IPC_HH__
#define STEWARD_IPC_HH__

#define DEFAULT_SERVER_PATH "/tmp/trace-ipc.skt"

/*
 * Enumeration of all supported calls 
 */
typedef enum {

    C_PIPE = 0,
    C_DUP,
    C_DUP2,
    C_SOCKETPAIR,

    C_SOCKET,
    C_CONNECT,
    C_BIND,
    C_ACCEPT,
    C_CLOSE,

    C_EXIT,

    C_READ,
    C_RECV,
    C_RECVFROM,
    C_RECVMSG,
    C_READ_CHK,

    C_WRITE,
    C_SEND,
    C_SENDTO,
    C_SENDMSG,
    C_SENDFILE64,
    C_SENDFILE,
    C_WRITEV,

    C_INVALID

} call_t;

typedef enum {
    FD_NET = 0,
    FD_PIPE,
    FD_DISK,
    FD_ENUM_SIZE
} fd_t;

/*
 * Minimal amount of information passed from client to server
 */
struct call_info {
    call_t      call;       /* the traced call */
    int         pid;        /* pid of process that made the call */
    time_t      tv_s;       /* tv.tv_sec of invocation */
    suseconds_t tv_us;      /* tv.tv_usec of invocation */
};

int log_call (int server_fd, struct call_info *info, void *params);

int server_init (const char *skt_name);
int server_fini ();
int client_init (const char *skt_name);
int client_fini ();

const char * call2str (call_t call);

#endif /* STEWARD_IPC_HH__ */
