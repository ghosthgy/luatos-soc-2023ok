/**
 * @file
 * UDP API (to be used from TCPIP thread)\n
 * See also @ref udp_raw
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_UDP_H
#define LWIP_HDR_UDP_H

#include "lwip/opt.h"

#if LWIP_UDP /* don't build if not configured for use in lwipopts.h */

#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/ip.h"
#include "lwip/ip6_addr.h"
#include "lwip/prot/udp.h"

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
#include "tcpiphibapi.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define UDP_FLAGS_NOCHKSUM       0x01U
#define UDP_FLAGS_UDPLITE        0x02U
#define UDP_FLAGS_CONNECTED      0x04U
#define UDP_FLAGS_MULTICAST_LOOP 0x08U

struct udp_pcb;

/** Function prototype for udp pcb receive callback functions
 * addr and port are in same byte order as in the pcb
 * The callback is responsible for freeing the pbuf
 * if it's not used any more.
 *
 * ATTENTION: Be aware that 'addr' might point into the pbuf 'p' so freeing this pbuf
 *            can make 'addr' invalid, too.
 *
 * @param arg user supplied argument (udp_pcb.recv_arg)
 * @param pcb the udp_pcb which received data
 * @param p the packet buffer that was received
 * @param addr the remote IP address from which the packet was received
 * @param port the remote port from which the packet was received
 */
typedef int (*udp_recv_fn)(void *arg, struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port);

#if ENABLE_PSIF
typedef void  (*udp_process_oos_fn)(void *arg, u8_t is_oos);
typedef void  (*udp_process_error_fn)(void *arg, err_t error);
void udp_error(struct udp_pcb *pcb, udp_process_error_fn errf, void *err_arg);

int udp_send_ul_state_ind(u32_t bitmap[8], int socket_id, int status);

int udp_send_tcp_close_complete_ind(int32_t socket_id);

#endif


/** the UDP protocol control block */
struct udp_pcb {
/** Common members of all PCB types */
  IP_PCB;

/* Protocol specific PCB members */

  struct udp_pcb *next;

  u8_t flags;
  /** ports are in host byte order */
  u16_t local_port, remote_port;

#if LWIP_MULTICAST_TX_OPTIONS
  /** outgoing network interface for multicast packets */
  ip_addr_t multicast_ip;
  /** TTL for outgoing multicast packets */
  u8_t mcast_ttl;
#endif /* LWIP_MULTICAST_TX_OPTIONS */

#if LWIP_UDPLITE
  /** used for UDP_LITE only */
  u16_t chksum_len_rx, chksum_len_tx;
#endif /* LWIP_UDPLITE */

  /** receive callback function */
  udp_recv_fn recv;
  /** user-supplied argument for the recv callback */
  void *recv_arg;

#if ENABLE_PSIF
  udp_process_oos_fn poosf;
  udp_process_error_fn errf;
  void *err_arg;
#endif

//store the suspend data info
#if LWIP_SUSPEND_UP_DATA_ENABLE
  void *pending_msg;
#endif

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
  u8_t pcb_hib_sleep2_mode_flag;
  u8_t reserved;
#endif
#if ENABLE_PSIF
  u8_t bind_ded_cid;//bind with dedicated cid
  int sockid;
#endif

};
/* udp_pcbs export for external reference (e.g. SNMP agent) */
extern struct udp_pcb *udp_pcbs;

/* The following functions is the application layer interface to the
   UDP code. */
struct udp_pcb * udp_new        (void);
struct udp_pcb * udp_new_ip_type(u8_t type);
void             udp_remove     (struct udp_pcb *pcb);
err_t            udp_bind       (struct udp_pcb *pcb, const ip_addr_t *ipaddr,
                                 u16_t port);
u16_t udp_alloc_local_port(void);

err_t            udp_connect    (struct udp_pcb *pcb, const ip_addr_t *ipaddr,
                                 u16_t port);
void             udp_disconnect (struct udp_pcb *pcb);
void             udp_recv       (struct udp_pcb *pcb, udp_recv_fn recv,
                                 void *recv_arg);
#if ENABLE_PSIF
void udp_poosf(struct udp_pcb *pcb, udp_process_oos_fn poosf);
#endif
err_t            udp_sendto_if  (struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *dst_ip, u16_t dst_port,
                                 struct netif *netif);
err_t            udp_sendto_if_src(struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *dst_ip, u16_t dst_port,
                                 struct netif *netif, const ip_addr_t *src_ip);
err_t            udp_sendto     (struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *dst_ip, u16_t dst_port);
err_t            udp_send       (struct udp_pcb *pcb, struct pbuf *p);

#if LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP
err_t            udp_sendto_if_chksum(struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *dst_ip, u16_t dst_port,
                                 struct netif *netif, u8_t have_chksum,
                                 u16_t chksum);
err_t            udp_sendto_chksum(struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *dst_ip, u16_t dst_port,
                                 u8_t have_chksum, u16_t chksum);
err_t            udp_send_chksum(struct udp_pcb *pcb, struct pbuf *p,
                                 u8_t have_chksum, u16_t chksum);
err_t            udp_sendto_if_src_chksum(struct udp_pcb *pcb, struct pbuf *p,
                                 const ip_addr_t *dst_ip, u16_t dst_port, struct netif *netif,
                                 u8_t have_chksum, u16_t chksum, const ip_addr_t *src_ip);
#endif /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */

#define          udp_flags(pcb) ((pcb)->flags)
#define          udp_setflags(pcb, f)  ((pcb)->flags = (f))

/* The following functions are the lower layer interface to UDP. */
void             udp_input      (struct pbuf *p, struct netif *inp);

void             udp_init       (void);

/* for compatibility with older implementation */
#define udp_new_ip6() udp_new_ip_type(IPADDR_TYPE_V6)

#if LWIP_MULTICAST_TX_OPTIONS
#define udp_set_multicast_netif_addr(pcb, ip4addr) ip_addr_copy_from_ip4((pcb)->multicast_ip, *(ip4addr))
#define udp_get_multicast_netif_addr(pcb)          ip_2_ip4(&(pcb)->multicast_ip)
#define udp_set_multicast_ttl(pcb, value)      do { (pcb)->mcast_ttl = value; } while(0)
#define udp_get_multicast_ttl(pcb)                 ((pcb)->mcast_ttl)
#endif /* LWIP_MULTICAST_TX_OPTIONS */

#if UDP_DEBUG
void udp_debug_print(struct udp_hdr *udphdr);
#else
#define udp_debug_print(udphdr)
#endif

void udp_netif_ip_addr_changed(const ip_addr_t* old_addr, const ip_addr_t* new_addr);

#if ENABLE_PSIF
u8_t IsAnyActiveUdpConn(void);

struct udp_pcb *get_global_udp_pcb_list(void);

#if LWIP_SUSPEND_UP_DATA_ENABLE
u8_t is_any_udp_pcb_pending_ul_data(void);
#endif

#endif

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
u8_t is_any_udp_pcb_pending_data(void);
#endif

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE

u8_t udp_get_curr_hib_sleep2_pcb_num(void);
void udp_add_curr_hib_sleep2_pcb_num(void);
void udp_del_curr_hib_sleep2_pcb_num(void);
void udp_enable_hib_sleep2_mode(struct udp_pcb *pcb, u8_t state);
void udp_disable_hib_sleep2_mode(struct udp_pcb *pcb, u8_t state);
void udp_set_hib_sleep2_state(struct udp_pcb *pcb, u8_t state);
struct udp_pcb *udp_get_hib_sleep2_pcb_list(void);
void udp_set_hib_sleep2_pcb_list(struct udp_pcb * new_pcb);
struct udp_pcb *udp_add_hib_sleep2_context_pcb(void);
void udp_remove_pcb_from_bounds_list(struct udp_pcb* pcb);
void udp_check_hib_sleep2_pcb_active(ip_addr_t *ue_addr);
int udp_get_sock_info(int fd, ip_addr_t *local_ip, ip_addr_t *remote_ip, u16_t *local_port, u16_t *remote_port);
int udp_get_sock_info_by_pcb(struct udp_pcb *pcb, ip_addr_t *local_ip, ip_addr_t *remote_ip, u16_t *local_port, u16_t *remote_port);
int udp_get_hib_sock_id(void);
BOOL udp_check_is_include_list(struct udp_pcb* pcb);
u16_t get_hib_udp_pcb_active_local_port(void);
struct udp_pcb *get_udp_list(void);
#endif

#if ENABLE_PSIF
err_t udp_set_sockid(struct udp_pcb *pcb, u32_t socketid);
void udp_netif_enter_oos_state(const ip_addr_t *address);
void udp_netif_exit_oos_state(const ip_addr_t *address);
struct udp_pcb *get_global_sequence_handler_pcb(void);
void set_global_sequence_handler_pcb(struct udp_pcb *handler_pcb);
struct udp_pcb *get_global_tcp_close_complete_handler_pcb(void);
void set_global_tcp_close_complete_handler_pcb(struct udp_pcb *handler_pcb);
int udp_send_ul_state_ind(u32_t bitmap[8], int socket_id, int status);
int udp_send_dns_resolve_result(u8_t source, u8_t result, const ip_addr_t *addr_info, const char *url);
int udp_send_ul_total_length_status(int32_t socket_id, u32_t ul_total_length);
int udp_send_pkg_to_pcb(u16_t body_len, void *body);
int udp_local_input(const void *data, int size, u16_t toPort, u16_t fromPort);
#endif


#ifdef __cplusplus
}
#endif

#endif /* LWIP_UDP */

#endif /* LWIP_HDR_UDP_H */
