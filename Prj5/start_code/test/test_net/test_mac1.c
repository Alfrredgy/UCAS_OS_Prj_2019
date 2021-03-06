#include "mac.h"
#include "irq.h"
#include "type.h"
#include "screen.h"
#include "syscall.h"
#include "sched.h"
#include "test4.h"

queue_t recv_block_queue;
desc_t *send_desc;
desc_t *receive_desc;
uint32_t cnt = 1; //record the time of iqr_mac
//uint32_t buffer[PSIZE] = {0x00040045, 0x00000100, 0x5d911120, 0x0101a8c0, 0xfb0000e0, 0xe914e914, 0x00000801,0x45000400, 0x00010000, 0x2011915d, 0xc0a80101, 0xe00000fb, 0x14e914e9, 0x01080000};
uint32_t buffer[PSIZE] = {0xffffffff, 0x5500ffff, 0xf77db57d, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0004e914, 0x0000, 0x005e0001, 0x2300fb00, 0x84b7f28b, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0801e914, 0x0000};

static void mac_send_desc_init(mac_t *mac)
{
    //init send desc
    int i;
    for (i = 0; i < PNUM - 1; i++) {
        tdes[i].des0 = 0x00000000;
        tdes[i].des1 = 0x61000000 | (4 * PSIZE);
        tdes[i].des2 = ((uint32_t)&tbuf[i]) & 0x1fffffff;
        tdes[i].des3 = ((uint32_t)&tdes[i+1]) & 0x1fffffff;
    }
    tdes[PNUM-1].des0 = 0x00000000;
    tdes[PNUM-1].des1 = 0x63000000 | (4 * PSIZE);
    tdes[PNUM-1].des2 = ((uint32_t)&tbuf[PNUM-1]) & 0x1fffffff;
    tdes[PNUM-1].des3 = ((uint32_t)&tdes[0]) & 0x1fffffff;
    
    mac->saddr = (uint32_t)tbuf;
    mac->saddr_phy = (uint32_t)tbuf & 0x1fffffff;
    mac->td = (uint32_t)tdes;
    mac->td_phy = (uint32_t)tdes & 0x1fffffff;
}

static void mac_recv_desc_init(mac_t *mac)
{
    //init recv desc
    int i;
    for (i = 0; i < PNUM - 1; i++) {
        rdes[i].des0 = 0x00000300;
        rdes[i].des1 = 0x81000000 | (4 * PSIZE);
        rdes[i].des2 = ((uint32_t)&rbuf[i]) & 0x1fffffff;
        rdes[i].des3 = ((uint32_t)&rdes[i+1]) & 0x1fffffff;
    }
    rdes[PNUM-1].des0 = 0x00000300;
    rdes[PNUM-1].des1 = 0x83000000 | (4 * PSIZE);
    rdes[PNUM-1].des2 = ((uint32_t)&rbuf[PNUM-1]) & 0x1fffffff;
    rdes[PNUM-1].des3 = ((uint32_t)&rdes[0]) & 0x1fffffff;
    
    mac->daddr = (uint32_t)rbuf;
    mac->daddr_phy = (uint32_t)rbuf & 0x1fffffff;
    mac->rd = (uint32_t)rdes;
    mac->rd_phy = (uint32_t)rdes & 0x1fffffff;
}

static void mii_dul_force(mac_t *mac)
{
    reg_write_32(mac->dma_addr, 0x80); //?s
                                       //   reg_write_32(mac->dma_addr, 0x400);
    uint32_t conf = 0xc800;            //0x0080cc00;

    // loopback, 100M
    reg_write_32(mac->mac_addr, reg_read_32(mac->mac_addr) | (conf) | (1 << 8));
    //enable recieve all
    reg_write_32(mac->mac_addr + 0x4, reg_read_32(mac->mac_addr + 0x4) | 0x80000001);
}

void dma_control_init(mac_t *mac, uint32_t init_value)
{
    reg_write_32(mac->dma_addr + DmaControl, init_value);
    return;
}

void mac_send_task()
{

    mac_t test_mac;
    uint32_t i;
    uint32_t print_location = 2;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum

    mac_send_desc_init(&test_mac);

    dma_control_init(&test_mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&test_mac);

    mii_dul_force(&test_mac);

    // register_irq_handler(LS1C_MAC_IRQ, mac_irq_handle);

    // irq_enable(LS1C_MAC_IRQ);
    sys_move_cursor(1, print_location);
    printf("> [MAC SEND TASK] start send package.               \n");

    uint32_t cnt = 0;
    i = 4;
    while (i > 0)
    {
        sys_net_send(test_mac.td, test_mac.td_phy);
        cnt += PNUM;
        sys_move_cursor(1, print_location);
        printf("> [MAC SEND TASK] totally send package %d !        \n", cnt);
        i--;
    }
    sys_exit();
}

void mac_recv_task()
{

    mac_t test_mac;
    uint32_t i;
    uint32_t ret;
    uint32_t print_location = 1;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum
    mac_recv_desc_init(&test_mac);

    dma_control_init(&test_mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&test_mac);

    mii_dul_force(&test_mac);

    queue_init(&recv_block_queue);
    sys_move_cursor(1, print_location);
    printf("[MAC RECV TASK] start recv:                    ");
    ret = sys_net_recv(test_mac.rd, test_mac.rd_phy, test_mac.daddr);
    if (ret == 0)
    {
        sys_move_cursor(1, print_location);
        printf("[MAC RECV TASK]     net recv is ok!                          ");
    }
    else
    {
        sys_move_cursor(1, print_location);
        printf("[MAC RECV TASK]     net recv is fault!                       ");
    }

    sys_exit();
}

void mac_init_task()
{
    uint32_t print_location = 1;
    sys_move_cursor(1, print_location);
    printf("> [MAC INIT TASK] Waiting for MAC initialization .\n");
    sys_init_mac();
    sys_move_cursor(1, print_location);
    printf("> [MAC INIT TASK] MAC initialization succeeded.           \n");
    sys_exit();
}
