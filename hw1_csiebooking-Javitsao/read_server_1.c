#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>


#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

#define OBJ_NUM 3

#define FOOD_INDEX 0
#define CONCERT_INDEX 1
#define ELECS_INDEX 2
#define RECORD_PATH "./bookingRecord"
int already_handle_read[1500];
static char* obj_names[OBJ_NUM] = {"Food", "Concert", "Electronics"};
const char* data = "Please enter your id (to check your booking state):\n";
const char* error_info = "[Error] Operation failed. Please try again.\n";
const char* Locked = "Locked.\n";
char obj_data[1000] = {};
int usr_id[1500], enter_exit;
typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;
server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const unsigned char IAC_IP[3] = "\xff\xf4";

static void init_server(unsigned short port);

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id;          // 902001-902020
    int bookingState[OBJ_NUM]; // 1 means booked, 0 means not.
}record;

int handle_read(request* reqP) {
    /*  Return value:
     *      1: read successfully
     *      0: read EOF (client down)
     *     -1: read failed
     */
    int r;
    char buf[512] = {};
    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    int valid = 1;
    for(int i = 0; i < 6; i++){
        if(buf[i] > '9' || buf[i] < '0') valid = 0;
    }
    int read_id = (buf[0] - '0') * 100000 + (buf[1] - '0') * 10000 + (buf[2] - '0') * 1000 + (buf[3] - '0') * 100 + (buf[4] - '0') * 10 + (buf[5] - '0');
    if(!valid || read_id < 902001 || read_id > 902020 || (32 <= buf[6] && buf[6] <= 126)){
        write(reqP->conn_fd, error_info, strlen(error_info));
        enter_exit = 1;
        close(reqP->conn_fd);
        free_request(reqP);
    }
    usr_id[reqP->conn_fd] = 0;
    usr_id[reqP->conn_fd] += read_id;

    
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);
    for(int i = 0; i < maxfd; i++){
        requestP[i].conn_fd = -1;
    }
    struct timeval timeout;
    timeout.tv_sec = 10000;
    timeout.tv_usec = 0;
    
    fd_set readset, writeset;
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(svr.listen_fd, &readset);
    //FD_SET(0, &readset);
    //FD_SET(3, &readset);
    //FD_SET(1, &writeset);
    //FD_SET(2, &writeset);
    record booking;
    int booking_fd = open(RECORD_PATH, O_RDWR);

    //FD_SET(svr.listen_fd, &writeset);
    struct flock lk[20];
    int lock_num[20] = {}, lock[20] = {};
    // Check new connection
    while (1) {
        // TODO: Add IO multiplexing
        for(int i = 0; i < maxfd; i++){
            if(requestP[i].conn_fd != -1) FD_SET(requestP[i].conn_fd, &readset);
        }
        select(maxfd + 1, &readset, NULL, NULL, &timeout);

            //printf("cc = %d\n", cc);
            if(FD_ISSET(svr.listen_fd, &readset)){
                //printf("cc_in = %d\n", cc);
                clilen = sizeof(cliaddr);
                conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                if (conn_fd < 0) {
                    if (errno == EINTR || errno == EAGAIN) continue;  // try again
                    if (errno == ENFILE) {
                        (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                        continue;
                    }
                    ERR_EXIT("accept");
                }
                requestP[conn_fd].conn_fd = conn_fd;
                strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
                write(conn_fd, data, strlen(data));
                FD_SET(requestP[conn_fd].conn_fd, &readset);
                

            }
            else{
                //printf("ttt\n");
                
                for(int ccc = 0; ccc < /*(cc + 10 > maxfd + 1)? maxfd + 1: */maxfd + 1; ccc++){
                    //printf("ccc = %d\n", ccc);
                    int now_fd = requestP[ccc].conn_fd;
                    enter_exit = 0;
                    //if(now_fd != -1){
                    //    FD_SET(now_fd, &readset);
                    
                        if(FD_ISSET(now_fd, &readset)){
                            
                            //printf("ccc_in = %d\n", ccc);
                            if(already_handle_read[now_fd]){
                                
                                
                                char read_exit[512] = {};
                                //int enter_exit = 0; not_need
                                int r = read(now_fd, read_exit, sizeof(read_exit));
                                if (r < 0) continue;
                                if (r == 0){
                                    already_handle_read[now_fd] = 0;
                                    requestP[now_fd].conn_fd = -1;
                                    FD_CLR(now_fd, &readset);
                                    lock_num[usr_id[now_fd] - 902001]--;
                                    //printf("minus_locknum[%d] = %d\n", usr_id[now_fd], lock_num[usr_id[now_fd] - 902001]);
                                    if(lock_num[usr_id[now_fd] - 902001] == 0){
                                        lk[usr_id[now_fd] - 902001].l_type = F_UNLCK;
                                        lock[usr_id[now_fd] - 902001] = fcntl(booking_fd, F_SETLK, &lk[usr_id[now_fd] - 902001]);
                                    }
                                    close(now_fd);
                                    free_request(&requestP[now_fd]);
                                    continue;
                                }
                                char* p1 = strstr(read_exit, "\015\012");
                                int newline_len = 2;
                                if (p1 == NULL) {
                                p1 = strstr(read_exit, "\012");
                                    if (p1 == NULL) {
                                        if (!strncmp(read_exit, IAC_IP, 2)) {
                                            // Client presses ctrl+C, regard as disconnection
                                            fprintf(stderr, "Client presses ctrl+C....\n");
                                            already_handle_read[now_fd] = 0;
                                            requestP[now_fd].conn_fd = -1;
                                            FD_CLR(now_fd, &readset);
                                            lock_num[usr_id[now_fd] - 902001]--;
                                            //printf("minus_locknum[%d] = %d\n", usr_id[now_fd], lock_num[usr_id[now_fd] - 902001]);
                                            if(lock_num[usr_id[now_fd] - 902001] == 0){
                                                lk[usr_id[now_fd] - 902001].l_type = F_UNLCK;
                                                lock[usr_id[now_fd] - 902001] = fcntl(booking_fd, F_SETLK, &lk[usr_id[now_fd] - 902001]);
                                            }
                                            close(now_fd);
                                            free_request(&requestP[now_fd]);
                                            continue;
                                        }
                                        ERR_EXIT("this really should not happen...");
                                    }
                                }
                                size_t len = p1 - read_exit + 1;
                                read_exit[len - 1] = '\0';
                                //printf("exit_len = %lu\n", strlen(read_exit));


                                    //printf("inexit: %d %d\n", read_exit[4]);
                                    if(read_exit[0] == 'E' && read_exit[1] == 'x' && read_exit[2] == 'i' && read_exit[3] == 't' && !(32 <= read_exit[4] && read_exit[4] <= 126)){
                                    //if(strcmp("Exit\n", read_exit) == 0){
                                        already_handle_read[now_fd] = 0;
                                        requestP[now_fd].conn_fd = -1;
                                        FD_CLR(now_fd, &readset);
                                        lock_num[usr_id[now_fd] - 902001]--;
                                        //printf("minus_locknum[%d] = %d\n", usr_id[now_fd], lock_num[usr_id[now_fd] - 902001]);
                                        if(lock_num[usr_id[now_fd] - 902001] == 0){
                                            lk[usr_id[now_fd] - 902001].l_type = F_UNLCK;
                                            lock[usr_id[now_fd] - 902001] = fcntl(booking_fd, F_SETLK, &lk[usr_id[now_fd] - 902001]);
                                        }
                                        close(now_fd);
                                        free_request(&requestP[now_fd]);
                                        //enter_exit = 1; break; not_need
                                    }
                                
                            }
                            if(!already_handle_read[now_fd] && FD_ISSET(now_fd, &readset)){
                                int ret = handle_read(&requestP[now_fd]); // parse data from client to requestP[conn_fd].buf
                                //printf("enter_exit = %d\n", enter_exit);
                                if(enter_exit){
                                    FD_CLR(now_fd, &readset);
                                    requestP[now_fd].conn_fd = -1;
                                    continue;
                                }

                                lk[usr_id[now_fd] - 902001].l_type = F_RDLCK;
                                lk[usr_id[now_fd] - 902001].l_whence = SEEK_SET;
                                lk[usr_id[now_fd] - 902001].l_start = sizeof(record) * (usr_id[now_fd] - 902001);
                                lk[usr_id[now_fd] - 902001].l_len = sizeof(record);
                                lock[usr_id[now_fd] - 902001] = fcntl(booking_fd, F_SETLK, &lk[usr_id[now_fd] - 902001]);
                                if(lock[usr_id[now_fd] - 902001] == -1){
                                    requestP[ccc].conn_fd = -1;
                                    FD_CLR(now_fd, &readset);
                                    write(now_fd, Locked, strlen(Locked));
                                    close(now_fd);
                                    free_request(&requestP[ccc]);
                                    continue;
                                }
                                lock_num[usr_id[now_fd] - 902001]++;
                                //printf("add_locknum[%d] = %d\n", usr_id[now_fd], lock_num[usr_id[now_fd] - 902001]);

                                //printf("%d\n", usr_id[now_fd]);
                                lseek(booking_fd, (usr_id[now_fd] - 902001) * sizeof(record), SEEK_SET);
                                read(booking_fd, &booking, sizeof(booking));
                                sprintf(obj_data, "%s: %d booked\n%s: %d booked\n%s: %d booked\n", obj_names[0], booking.bookingState[0], obj_names[1], booking.bookingState[1], obj_names[2], booking.bookingState[2]);
                                write(now_fd, obj_data, strlen(obj_data));
                                //printf("length = %d\n", strlen(obj_data));

                                char *type_exit = "\n(Type Exit to leave...)\n";
                                write(now_fd, type_exit, strlen(type_exit));

                                fprintf(stderr, "ret = %d\n", ret);
                                if (ret < 0) {
                                    fprintf(stderr, "bad request from %s\n", requestP[now_fd].host);
                                    continue;
                                }
                                already_handle_read[now_fd] = 1;
                            }
                            
                            //if(enter_exit) continue;

                        }
                    //}
                    
                }
                FD_SET(svr.listen_fd, &readset);
                //sleep(1);
            }
        //char* data = "Please enter your id (to check your booking state):";
        
        
        //printf("fd = %d\n", svr.listen_fd);
        
        
        
        
    // TODO: handle requests from clients
#ifdef READ_SERVER      
        
#elif defined WRITE_SERVER
            
#endif
    }
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}
