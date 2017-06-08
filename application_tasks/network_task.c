/**************************************************************************
 * (c) Copyright 2009 Actel Corporation.  All rights reserved.
 *
 *  Application demo for Smartfusion
 *
 *
 * Author : Actel Application Team
 * Rev     : 1.0.0.3
 *
 **************************************************************************/

/**************************************************************************/
/* Standard Includes */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>

/**************************************************************************/
/* Driver Includes */
/**************************************************************************/

#include "../drivers/mss_ethernet_mac/mss_ethernet_mac.h"
#include "../drivers/mac/tcpip.h"

/**************************************************************************/
/* RTOS Includes */
/**************************************************************************/

#include "FreeRTOS.h"
#include "semphr.h"

/**************************************************************************/
/*Web Server Includes */
/**************************************************************************/

#include "uip.h"
#include "uip_arp.h"
#include "httpd.h"

/**************************************************************************/
/* Definitions for Ethernet test */
/**************************************************************************/

#define OPEN_IP
#define BUF                       ((struct uip_eth_hdr *)&uip_buf[0])
#ifndef DEFAULT_NETMASK0
#define DEFAULT_NETMASK0          255
#endif

#ifndef DEFAULT_NETMASK1
#define DEFAULT_NETMASK1          255
#endif

#ifndef DEFAULT_NETMASK2
#define DEFAULT_NETMASK2          0
#endif

#ifndef DEFAULT_NETMASK3
#define DEFAULT_NETMASK3          0
#endif
#define TCP_PKT_SIZE              1600
#define IP_ADDR_LEN               4
#define PHY_ADDRESS               1
#define DHCP_ATTEMPTS             4
#define USER_RX_BUFF_SIZE         1600

/**************************************************************************/
/* Extern Declarations */
/**************************************************************************/
extern unsigned char              my_ip[4];
extern unsigned int               num_pkt_rx;
extern unsigned char              ip_known;
extern unsigned char              my_ip[IP_ADDR_LEN];
extern unsigned char              tcp_packet[TCP_PKT_SIZE];
// dhcp_ip_found defined in tcpip.c
extern unsigned char              dhcp_ip_found;
volatile unsigned char            oled_on_irq = 0;
volatile unsigned char            sw1_menu_scroll = 0;
volatile unsigned char            sw2_select = 0;
volatile unsigned char            led_on = 0;

const uint8_t ETH0_MAC_ADDRESS[] = {0xaa,0xbb,0xcc,0x66,0x55,0x44};


/**************************************************************************/
/* Function to Initialize the MAC, setting the MAC address and */
/* fetches the IP address */
/**************************************************************************/

void init_mac()
{
    uint32_t time_out = 0;
    int32_t mac_cfg;
    int32_t rx_size;
    uint8_t rx_buffer[USER_RX_BUFF_SIZE] = {};
    MSS_MAC_init(PHY_ADDRESS);
    /*
     * Configure the MAC.
     */
    mac_cfg = MSS_MAC_get_configuration();

    mac_cfg &= ~( MSS_MAC_CFG_STORE_AND_FORWARD | MSS_MAC_CFG_PASS_BAD_FRAMES );
    mac_cfg |=
    MSS_MAC_CFG_RECEIVE_ALL |
    MSS_MAC_CFG_PROMISCUOUS_MODE |
    MSS_MAC_CFG_FULL_DUPLEX_MODE |
    MSS_MAC_CFG_TRANSMIT_THRESHOLD_MODE |
    MSS_MAC_CFG_THRESHOLD_CONTROL_00;
    MSS_MAC_configure(mac_cfg);
    MSS_MAC_set_mac_address(ETH0_MAC_ADDRESS);

    // Put TCP into listen state
    tcp_init();

    ip_known = 0;
    num_pkt_rx = 0;
    time_out = 0;
    
    // Try and get an IP address via BOOTP
    do
    {
        send_bootp_packet(0);
        do
        {
            rx_size = MSS_MAC_rx_pckt_size();
            time_out++;
            // dhcp_ip_found is a global set in tcpip.c
            if (dhcp_ip_found) {
            	break;
            }
         }while ( rx_size == 0 && (time_out < 3000000));
         MSS_MAC_rx_packet( rx_buffer, USER_RX_BUFF_SIZE, MSS_MAC_BLOCKING );
         num_pkt_rx++;
         process_packet( rx_buffer );
    }while((!dhcp_ip_found) && (time_out < 7000000));

    if (dhcp_ip_found) {
    	printf("IP address = %d.%d.%d.%d\r\n", my_ip[0], my_ip[1], my_ip[2], my_ip[3]);
    }
    else {
    	printf("BOOTP could not assign address\r\n");
    }
    fflush(stdout);
}

/**************************************************************************/
/* Function for uIP initialization */
/**************************************************************************/

void uIP_init()
{
    uip_ipaddr_t ipaddr;
    static struct uip_eth_addr sTempAddr;

    /* init tcp/ip stack */
    uip_init();
    uip_ipaddr(ipaddr, my_ip[0], my_ip[1], my_ip[2], my_ip[3]);
    uip_sethostaddr(ipaddr);

    uip_ipaddr(ipaddr,
               DEFAULT_NETMASK0,
               DEFAULT_NETMASK1,
               DEFAULT_NETMASK2,
               DEFAULT_NETMASK3);
    uip_setnetmask(ipaddr);

    /* set mac address */
    sTempAddr.addr[0] = ETH0_MAC_ADDRESS[0];
    sTempAddr.addr[1] = ETH0_MAC_ADDRESS[1];
    sTempAddr.addr[2] = ETH0_MAC_ADDRESS[2];
    sTempAddr.addr[3] = ETH0_MAC_ADDRESS[3];
    sTempAddr.addr[4] = ETH0_MAC_ADDRESS[4];
    sTempAddr.addr[5] = ETH0_MAC_ADDRESS[5];

    uip_setethaddr(sTempAddr);

    /* init application */
    httpd_init();
}

void network_task(void *para)
{
    long lPeriodicTimer, lARPTimer, lPacketLength;
    unsigned long ulTemp;

    while(1) {
    	// Initialize Ethernet MAC, & try to get an IP address via BOOTP
        init_mac();

        uIP_init();

        lPeriodicTimer = 0;
        lARPTimer = 0;

        while(1) {
            lPacketLength = MSS_MAC_rx_packet( uip_buf,
                                               sizeof(uip_buf),
                                               MSS_MAC_BLOCKING);
            if(lPacketLength > 0) {
                /*  Set uip_len for uIP stack usage.*/
                uip_len = (unsigned short)lPacketLength;

                /*  Process incoming IP packets here.*/
                if(BUF->type == htons(UIP_ETHTYPE_IP))
                {
                    uip_arp_ipin();
                    uip_input();

                    /*  If the above function invocation resulted in data that
                         should be sent out on the network, the global variable
                         uip_len is set to a value > 0.*/

                    if(uip_len > 0)
                    {
                        uip_arp_out();
                        MSS_MAC_tx_packet( uip_buf,
                                           uip_len,
                                           MSS_MAC_NONBLOCKING);
                        uip_len = 0;
                    }
                }

                /*  Process incoming ARP packets here. */
                else if(BUF->type == htons(UIP_ETHTYPE_ARP))
                {
                    uip_arp_arpin();

                    /*  If the above function invocation resulted in data that
                         should be sent out on the network, the global variable
                         uip_len is set to a value > 0  */

                    if(uip_len > 0)
                    {
                        MSS_MAC_tx_packet(uip_buf,
                                          uip_len,
                                          MSS_MAC_NONBLOCKING);
                        uip_len = 0;
                    }
                }
            }

            for(ulTemp = 0; ulTemp < UIP_CONNS; ulTemp++) {
                uip_periodic(ulTemp);
                /* If the above function invocation resulted in data that
                    should be sent out on the network, the global variable
                    uip_len is set to a value > 0 */

                if(uip_len > 0)
                {
                    uip_arp_out();
                    MSS_MAC_tx_packet(uip_buf,
                                      uip_len,
                                      MSS_MAC_NONBLOCKING);
                    uip_len = 0;
                }
            }
            uip_arp_timer();
        }
    }
}
