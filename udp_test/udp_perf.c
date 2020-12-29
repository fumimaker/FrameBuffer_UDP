#include "udp_perf_client.h"

extern struct netif server_netif;
static struct udp_pcb *pcb[NUM_OF_PARALLEL_CLIENTS];
static struct perf_stats client;

char *frame_pointer;
u32 ptrCounter = 0;

unsigned int addressUpdate = false;
unsigned int sendFinished = false;

u32 frameCounter = 0;
char *simpleColor = (char *)malloc(sizeof(char) * 1280 * 720 * 3);

#define FINISH 1
#define NOT_FINISH 0

//#define true 1
//#define false 0

/* Report interval time in ms */
#define REPORT_INTERVAL_TIME ((u32)INTERIM_REPORT_INTERVAL * (u32)1000)
/* End time in ms */
#define END_TIME ((u32)UDP_TIME_INTERVAL * (u32)1000)

#define DATA_SIZE UDP_SEND_BUFSIZE - sizeof(int)

void print_app_header(void)
{
    xil_printf("UDP client connecting to %s on port %d\r\n",
               UDP_SERVER_IP_ADDRESS, UDP_CONN_PORT);
    xil_printf("On Host: Run $iperf -s -i %d -u\r\n\r\n",
               INTERIM_REPORT_INTERVAL);
}

static void print_udp_conn_stats(void)
{
    xil_printf("[%3d] local %s port %d connected with ",
               client.client_id, inet_ntoa(server_netif.ip_addr),
               pcb[0]->local_port);
    xil_printf("%s port %d\r\n", inet_ntoa(pcb[0]->remote_ip),
               pcb[0]->remote_port);
    xil_printf("[ ID] Interval\t\tTransfer   Bandwidth\n\r");
}

static void stats_buffer(char *outString, double data, enum measure_t type)
{
    int conv = KCONV_UNIT;
    const char *format;
    double unit = 1024.0;

    if (type == SPEED)
        unit = 1000.0;

    while (data >= unit && conv <= KCONV_GIGA)
    {
        data /= unit;
        conv++;
    }

    /* Fit data in 4 places */
    if (data < 9.995)
    {                        /* 9.995 rounded to 10.0 */
        format = "%4.2f %c"; /* #.## */
    }
    else if (data < 99.95)
    {                        /* 99.95 rounded to 100 */
        format = "%4.1f %c"; /* ##.# */
    }
    else
    {
        format = "%4.0f %c"; /* #### */
    }
    sprintf(outString, format, data, kLabel[conv]);
}

/* The report function of a UDP client session */
static void udp_conn_report(u64_t diff, enum report_type report_type)
{
    u64_t total_len;
    double duration, bandwidth = 0;
    char data[16], perf[16], time[64];

    if (report_type == INTER_REPORT)
    {
        total_len = client.i_report.total_bytes;
    }
    else
    {
        client.i_report.last_report_time = 0;
        total_len = client.total_bytes;
    }

    /* Converting duration from milliseconds to secs,
	 * and bandwidth to bits/sec .
	 */
    duration = diff / 1000.0; /* secs */
    if (duration)
        bandwidth = (total_len / duration) * 8.0;

    stats_buffer(data, total_len, BYTES);
    stats_buffer(perf, bandwidth, SPEED);
    /* On 32-bit platforms, xil_printf is not able to print
	 * u64_t values, so converting these values in strings and
	 * displaying results
	 */
    sprintf(time, "%4.1f-%4.1f sec",
            (double)client.i_report.last_report_time,
            (double)(client.i_report.last_report_time + duration));
    xil_printf("[%3d] %s  %sBytes  %sbits/sec %dfps\n\r", client.client_id,
               time, data, perf, client.i_report.frame_per_sec);

    if (report_type == INTER_REPORT)
        client.i_report.last_report_time += duration;
    else
        xil_printf("[%3d] sent %llu datagrams\n\r",
                   client.client_id, client.cnt_datagrams);
}

static void reset_stats(void)
{
    client.client_id++;
    /* Print connection statistics */
    print_udp_conn_stats();
    /* Save start time for final report */
    client.start_time = get_time_ms();
    client.total_bytes = 0;
    client.cnt_datagrams = 0;
    client.packet_id = 0;

    /* Initialize Interim report parameters */
    client.i_report.start_time = 0;
    client.i_report.total_bytes = 0;
    client.i_report.last_report_time = 0;
    client.i_report.frame_per_sec = 0;
}

static void udp_packet_send(u8_t finished)
{
    int *payload;
    static int packet_id;
    u8_t i;
    u8_t retries = MAX_SEND_RETRY;
    struct pbuf *packet;
    err_t err;
    int id = 0;
    addressUpdate = false;

    for (i = 0; i < NUM_OF_PARALLEL_CLIENTS; i++)
    {
        packet = pbuf_alloc(PBUF_TRANSPORT, UDP_SEND_BUFSIZE, PBUF_POOL);
        if (!packet)
        {
            xil_printf("error allocating pbuf to send\r\n");
            return;
        }
        else
        {
            if (simpleColor != NULL)
            {
                xil_printf("not null\n\r");
            }
            // frame_pointer+ptrCounterの先頭からBUSIZE-int(1436)分payload+1にコピー
            //4byte(int)開けて1436byteのデータをコピーする
            
        }
        id = ptrCounter / (UDP_SEND_BUFSIZE - sizeof(int)); // 1440-4を何回送ったかを計算
        
        if (finished == FINISH){
            int remain = (1280*720*3) - (id);
            packet_id = -1;
            id = -1;
            memcpy((int *)packet->payload + 1, frame_pointer + ptrCounter, remain);
            xil_printf("remain: %d", remain);
        }
        else {
            //memcpy((int*)packet->payload+1, simpleColor+ptrCounter, DATA_SIZE);
            memcpy((int *)packet->payload + 1, frame_pointer + ptrCounter, DATA_SIZE);
        }

        /* always increment the id */
        payload = (int *)(packet->payload);
        //xil_printf("id: %d\n\r",id);

        payload[0] = htonl(id); // 開けておいた4byteに番目を追加

        while (retries)
        {
            err = udp_send(pcb[i], packet); // UDP SEND
            if (err != ERR_OK)
            {
                xil_printf("Error on udp_send: %d\r\n", err);
                retries--;
                usleep(100);
            }
            else
            {
                client.total_bytes += UDP_SEND_BUFSIZE;
                client.cnt_datagrams++;
                client.i_report.total_bytes += UDP_SEND_BUFSIZE;
                client.packet_id = packet_id;
                break;
            }
        }
        if (!retries)
        {
            /* Terminate this app */
            u64_t now = get_time_ms();
            u64_t diff_ms = now - client.start_time;
            xil_printf("Too many udp_send() retries, ");
            xil_printf("Terminating application\n\r");
            udp_conn_report(diff_ms, UDP_DONE_CLIENT);
            xil_printf("UDP test failed\n\r");
            udp_remove(pcb[i]);
            pcb[i] = NULL;
        }
        pbuf_free(packet);
/* For ZynqMP SGMII, At high speed,
		 * "pack dropped, no space" issue observed.
		 * To avoid this, added delay of 2us between each
		 * packets.
		 */
#if defined(__aarch64__) && defined(XLWIP_CONFIG_INCLUDE_AXI_ETHERNET_DMA)
        usleep(2);
#endif /* __aarch64__ */
    }
    //xil_printf("udp_packet_send done\r\n");
    packet_id++; // テストで送ったパケットの識別子、個数

    if (finished == FINISH)
    { //終了、次のフレームのために初期化
        packet_id = 0;
        id = 0;
    }
}

/** Transmit data on a udp session */
void transfer_data(void)
{
    if (!addressUpdate && sendFinished)
    { // アドレスが更新されずに、送信が完了していたら何もせずに帰る。
        return;
    }
    else
    {
        u64_t now = 0;
        if (END_TIME || REPORT_INTERVAL_TIME)
        { //END_TIME
            now = get_time_ms();
            if (REPORT_INTERVAL_TIME)
            {
                if (client.i_report.start_time)
                {
                    u64_t diff_ms = now - client.i_report.start_time;
                    if (diff_ms >= REPORT_INTERVAL_TIME)
                    {
                        client.i_report.frame_per_sec = frameCounter;
                        udp_conn_report(diff_ms, INTER_REPORT);
                        client.i_report.start_time = 0;
                        client.i_report.total_bytes = 0;
                        frameCounter = 0;
                    }
                }
                else
                {
                    client.i_report.start_time = now;
                }
            }
        }

        // １回のサイズに満たない端数になったら端数とFINISH送って終了
        // HDの場合は1436byte*1926(0-1925)回目で500byte余る計算になる。
        if ((SIZE_OF_FRAME - ptrCounter) < (DATA_SIZE))
        {
            //xil_printf("remain %d\n\r", SIZE_OF_FRAME - ptrCounter );
            udp_packet_send(FINISH);
            //xil_printf("finished 0x%x\n\r", frame_pointer);
            sendFinished = true;
            //u64_t diff_ms = now - client.start_time;
            //udp_conn_report(diff_ms, INTER_REPORT);
            ptrCounter = 0;
            return;
        }

        //1~1925回(0~1924)
        udp_packet_send(!FINISH);
        ptrCounter += UDP_SEND_BUFSIZE - sizeof(int); //+=1436byte
    }
}

void start_application(void)
{
    err_t err;
    ip_addr_t remote_addr;
    u32_t i;

    err = inet_aton(UDP_SERVER_IP_ADDRESS, &remote_addr);
    if (!err)
    {
        xil_printf("Invalid Server IP address: %d\r\n", err);
        return;
    }

    for (i = 0; i < NUM_OF_PARALLEL_CLIENTS; i++)
    {
        /* Create Client PCB */
        pcb[i] = udp_new();
        if (!pcb[i])
        {
            xil_printf("Error in PCB creation. out of memory\r\n");
            return;
        }

        err = udp_connect(pcb[i], &remote_addr, UDP_CONN_PORT);
        if (err != ERR_OK)
        {
            xil_printf("udp_client: Error on udp_connect: %d\r\n", err);
            udp_remove(pcb[i]);
            return;
        }
    }

    /* Wait for successful connection */
    usleep(10);
    reset_stats();

    xil_printf("\r\n\r\nstart\r\n\r\n");

    //	for(u64 i=0; i<1280*720*3; i++){
    //		*(simpleColor+i) = 0xFF;
    //	}

    xil_printf("\r\n\r\nstart_application\r\n\r\n");
}

//Write完了の割り込みのたびに呼ばれる。
//前のフレームのポインターを渡してくるのでframe_pointerに入れる。
void update_address(char *pointer)
{
    // frame_pointerにコピペ
    addressUpdate = true;
    sendFinished = false;
    frame_pointer = pointer;
    frameCounter++;
    //xil_printf("frame_pointer: 0x%x\n\r",frame_pointer);
}
