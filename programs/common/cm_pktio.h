/*
 *   Copyright (c) 2012, Nokia Siemens Networks
 *   Copyright (c) 2015, Nokia Solutions and Networks
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CM_PKTIO_H
#define CM_PKTIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include <odp/helper/odph_api.h>
#include <event_machine/platform/env/environment.h>
#include <event_machine/platform/event_machine_odp_ext.h>

#define IPV4_PROTO_UDP  ODPH_IPPROTO_UDP

/**
 * @def PKTIO_MAX_IN_QUEUES
 * @brief Maximum number of odp pktio input queues per interface
 */
#define PKTIO_MAX_IN_QUEUES   16

/**
 * @def PKTIO_MAX_OUT_QUEUES
 * @brief Maximum number of odp pktio output queues per interface
 */
#define PKTIO_MAX_OUT_QUEUES  16

/**
 * @def MAX_PKT_BURST_RX
 * @brief Maximum number of packets received from a pktio input queue
 *        in one burst
 */
#define MAX_PKT_BURST_RX  32

/**
 * @def MAX_PKT_BURST_TX
 * @brief Maximum number of packets bursted onto a pktout queue
 */
#define MAX_PKT_BURST_TX  16

/**
 * @def MAX_TX_BURST_BUFS
 * @brief Maximum number of tx burst buffers per interface
 *
 * Store Tx pkts in output buffers until a buffer has 'MAX_PKT_BURST_TX' pkts,
 * then transmit the whole burst of pkts instead of one by one.
 */
#define MAX_TX_BURST_BUFS EM_MAX_CORES

/**
 * @def MAX_RX_PKT_QUEUES
 * @brief
 */
#define MAX_RX_PKT_QUEUES (4 * 64)

/**
 * @def MAX_RX_POLL_ROUNDS
 * @brief
 */
#define MAX_RX_POLL_ROUNDS 2

/**
 * @def BURST_TX_DRAIN
 * @brief The number of core cycles between timed TX buf drain operations
 */
#define BURST_TX_DRAIN (400000ULL)  /* around 200us at 2 Ghz */

/** Ethernet MAC address */
typedef union {
	uint8_t u8[6];
	uint16_t u16[3];
} mac_addr_t;

/** IPv4 address */
typedef union {
	uint8_t u8[4];
	uint16_t u16[2];
	uint32_t u32;
} ipv4_addr_t;

/**
 * @brief pkt header fields to use as hash key
 *
 * Fields from incoming packets used for destination em-odp queue lookup.
 */
struct pkt_dst_tuple {
	/* uint32_t ip_src;*/
	uint32_t ip_dst;
	/* uint16_t port_src;*/
	uint16_t port_dst;
	uint16_t proto;
} __attribute__((__packed__));

/** Use the struct pkt_dst_tuple as hash key for em-odp queue lookups */
typedef struct pkt_dst_tuple pkt_q_hash_key_t;

/* Keep size multiple of 32-bits for faster hash-crc32 calculation*/
ODP_STATIC_ASSERT(sizeof(pkt_q_hash_key_t) % sizeof(uint32_t) == 0,
		  "HASH_KEY_NOT_MULTIP_OF_32__ERROR");

/**
 * @brief Info about em-odp queue to use, returned by hash lookup
 *
 * Information about an em-odp queue used for pktio, stored in a hash table and
 * used when doing a tbl lookup to determine the destination em-odp queue
 * for a received packet.
 */
typedef struct {
	int pos;
	em_queue_t queue;
} rx_pkt_queue_t;

/**
 * @brief Tx pkt burst buffer
 *
 * Buffer up to 'MAX_PKT_BURST_TX' pkts before bursting them all onto
 * the associated 'pktout_queue' at once.
 */
typedef struct tx_burst {
	/** store tx pkts temporaily in 'queue' before bursting onto tx */
	odp_queue_t queue ODP_ALIGNED_CACHE;
	/** count the number of events in 'queue' */
	env_atomic64_t cnt;
	/** lock needed when dequeueing from 'queue' */
	odp_spinlock_t lock;
	/** store the output interface port also here for easy access */
	int if_port;
	/** Transmit burst using this pktout_queue */
	odp_queue_t pktout_queue;
} tx_burst_t;

/**
 * @brief Rx pkt storage for pkts destined to the same em-odp queue
 *
 * Temporary storage for events to be enqueued onto the _same_ queue
 * after receiving a packet burst on Rx
 */
typedef struct {
	int sent;
	int pkt_cnt;
	em_queue_t queue;
	odp_packet_t pkt_tbl[MAX_PKT_BURST_RX];
} rx_queue_burst_t;

/**
 * @brief Pktio shared memory
 *
 * Collection of shared data used by pktio Rx&Tx
 */
typedef struct {
	/** The number of pktio interfaces used */
	int if_count;
	/** Memory pool for pktio*/
	odp_pool_t pool;
	/** Default queue to use for incoming pkts without a dedicated queue */
	em_queue_t default_queue;
	/** flag set after pktio_start() - prevent pkio rx&tx before started */
	int pktio_started;
	/** ODP pktio handles */
	odp_pktio_t pktio[IF_MAX_NUM];

	/** Number of pktio input queues per interface */
	int pktin_num_queues[IF_MAX_NUM];
	/** All pktio input queues used, per interface */
	odp_pktin_queue_t pktin_queues[IF_MAX_NUM][PKTIO_MAX_IN_QUEUES];
	/** Queue contains the used pktin_queues[][], deq one and use for Rx */
	odp_queue_t pktin_queues_queue;

	/** Number of pktio output queues per interface */
	int pktout_num_queues[IF_MAX_NUM];
	/** All pktio output queues used, per interface */
	odp_queue_t pktout_queues[IF_MAX_NUM][PKTIO_MAX_OUT_QUEUES];
	/** Queue contains the used tx_burst[][],deq one and use for timed Tx*/
	odp_queue_t tx_bursts_queue;

	/** Info about the em-odp queues configured for pktio, store in hash */
	rx_pkt_queue_t rx_pkt_queues[MAX_RX_PKT_QUEUES];

	/** Pkt lookup table, lookup destination em-odp queue for Rx pkts */
	struct {
		odph_table_ops_t ops;
		odph_table_t tbl;
		int tbl_idx;
		odp_ticketlock_t lock;
	} tbl_lookup;

	/** Tx burst buffers per interface  */
	tx_burst_t tx_burst[IF_MAX_NUM][MAX_TX_BURST_BUFS] ODP_ALIGNED_CACHE;
} pktio_shm_t;

/**
 * @brief Pktio core-local memory
 *
 * Collection of core local (not shared) data used by pktio Rx&Tx
 */
typedef struct {
	/** Event contains the currently used pktio input queue */
	odp_event_t pktin_queue_event;
	/** Event contains the currently used tx-burst buffer for timed Tx */
	odp_event_t tx_burst_timed_event;
	/** Determine need for timed drain of pktio Tx queues */
	uint64_t tx_prev_cycles;
	/** Array of hash keys for the current received Rx pkt burst */
	pkt_q_hash_key_t keys[MAX_PKT_BURST_RX];
	/** Array of positions into rx_qbursts[], filled from hash lookup  */
	int positions[MAX_PKT_BURST_RX];
	/** Grouping of Rx pkts per destination em-odp queue */
	rx_queue_burst_t rx_qbursts[MAX_RX_PKT_QUEUES + 1]; /* +1=default Q */
	/** Temporary storage of Tx pkt burst */
	odp_event_t ev_burst[MAX_PKT_BURST_TX];
} pktio_locm_t;

void pktio_mem_reserve(void);
void pktio_mem_lookup(void);
void pktio_mem_free(void);

void pktio_pool_create(int if_count);
void pktio_pool_destroy(void);

void pktio_init(appl_conf_t *const appl_conf);
void pktio_deinit(appl_conf_t *const appl_conf);

int pktio_create(const char *dev, int num_workers);
void pktio_start(void);
void pktio_halt(void);
void pktio_stop(void);
void pktio_close(void);

/**
 * @brief Poll input resources for pkts/events and enqueue into EM queues
 *
 * Given to EM via 'em_conf.input.input_poll_fn' - EM will call this on
 * each core in the dispatch loop.
 * The function is of type 'em_input_poll_func_t'
 *
 * @return number of pkts/events received from input and enqueued into EM
 */
int input_poll(void);

/**
 * @brief Drain buffered output - ensure low rate flows are also sent out.
 *
 * Useful in situations where output is buffered and sent out in bursts when
 * enough output has been gathered - single events or low rate flows may,
 * without this function, never be sent out (or too late) if the buffering
 * threshold has not been reached.
 *
 * Given to EM via 'em_conf.output.output_drain_fn' - EM will call this on
 * each core in the dispatch loop.
 * The function is of type 'em_output_drain_func_t'
 *
 * @return number of events successfully drained and sent for output
 */
int output_drain(void);

/**
 * @brief User provided EM output-queue callback function ('em_output_func_t')
 *
 * Transmit events(pkts) using the given config onto Eth-tx
 *
 * Buffers the given 'events' in a Tx burst buffer and when full transmits
 * the whole burst from the buffer at once.
 *
 * @param events[]        Events to be sent
 * @param num             Number of entries in 'events[]'
 * @param output_queue    EM output queue the events were sent into (em_send*())
 * @param output_fn_args  Function args specific to the output-queue
 *                        Note: here it will be a 'pktio_tx_fn_args_t' pointer
 *
 * @return number of events successfully sent (equal to num if all successful)
 */
int pktio_tx(em_event_t events[], const unsigned int num,
	     const em_queue_t output_queue, void *output_fn_args);
/**
 * @typedef pktio_tx_fn_args_t
 * User defined arguments to the EM output queue callback function
 */
typedef struct {
	/** Pktio Tx interface ID */
	int if_id;
	/* add more if needed */
} pktio_tx_fn_args_t;

/**
 * Associate an EM-queue with a packet-I/O flow.
 *
 * Received packets matching the set destination IP-addr/port
 * will end up in the EM-queue 'queue'.
 */
void pktio_add_queue(uint8_t proto, uint32_t ipv4_dst, uint16_t l4_port_dst,
		     em_queue_t queue);

/**
 * Remove the association between a packet-IO flow and an EM-queue.
 *
 * No further received frames will end up in the EM-queue 'queue'
 */
void pktio_rem_queue(uint8_t proto, uint32_t ipv4_dst, uint16_t l4_port_dst,
		     em_queue_t queue);

/**
 * Set the default EM-queue for packet I/O
 */
int pktio_default_queue(em_queue_t queue);

/**
 * Provide applications a way to do a hash-lookup (e.g. sanity check etc.)
 */
em_queue_t pktio_lookup_sw(uint8_t proto, uint32_t ipv4_dst,
			   uint16_t l4_port_dst);

odp_pool_t pktio_pool_get(void);

static inline odp_packet_t
pktio_odp_packet_get(em_event_t em_event)
{
	return odp_packet_from_event(em_odp_event2odp(em_event));
}

static inline em_event_t
pktio_em_event_get(odp_packet_t odp_pkt)
{
	return em_odp_event2em(odp_packet_to_event(odp_pkt));
}

static inline em_event_t
pktio_alloc(size_t len)
{
	odp_packet_t odp_pkt = odp_packet_alloc(pktio_pool_get(), len);
	em_event_t event = pktio_em_event_get(odp_pkt);

	return event;
}

static inline void
pktio_drop(em_event_t event)
{
	odp_packet_t pkt = pktio_odp_packet_get(event);

	odp_packet_free(pkt);
}

static inline uint8_t *
pktio_get_frame(em_event_t event)
{
	odp_packet_t pkt = pktio_odp_packet_get(event);

	return odp_packet_data(pkt);
}

static inline uint32_t
pktio_get_frame_len(em_event_t event)
{
	odp_packet_t pkt = pktio_odp_packet_get(event);

	return odp_packet_len(pkt);
}

static inline int
pktio_input_port(em_event_t event)
{
	odp_packet_t pkt = pktio_odp_packet_get(event);
	int input_port = odp_packet_input_index(pkt);

	if (unlikely(input_port < 0))
		return 0;

	return input_port;
}

/**
 * Get the protocol, IPv4 destination address and destination L4 port the
 * packet-event was sent to.
 */
static inline void
pktio_get_dst(em_event_t event, uint8_t *proto__out,
	      uint32_t *ipv4_dst__out, uint16_t *l4_port_dst__out)
{
	odp_packet_t pkt = pktio_odp_packet_get(event);
	void *pkt_data;
	odph_ipv4hdr_t *ip;
	odph_udphdr_t *udp;

	/* if (odp_packet_has_ipv4(pkt)) {
	 *	ip = (odph_ipv4hdr_t *)odp_packet_l3_ptr(pkt, NULL);
	 *	*proto__out = ip->proto;
	 *	*ipv4_dst__out = ntohl(ip->dst_addr);
	 * } else {
	 *	*proto__out = 0;
	 *	*ipv4_dst__out = 0;
	 * }
	 *
	 * if (odp_packet_has_udp(pkt)) {
	 *	udp = (odph_udphdr_t *)odp_packet_l4_ptr(pkt, NULL);
	 *	*port_dst__out = ntohs(udp->dst_port);
	 * } else {
	 *	*port_dst__out = 0;
	 * }
	 */

	/* Note: no actual checks if the headers are present */
	pkt_data = odp_packet_data(pkt);
	ip = (odph_ipv4hdr_t *)((uintptr_t)pkt_data +
				sizeof(odph_ethhdr_t));
	udp = (odph_udphdr_t *)((uintptr_t)ip +
				sizeof(odph_ipv4hdr_t));

	*proto__out = ip->proto;
	*ipv4_dst__out = ntohl(ip->dst_addr);
	*l4_port_dst__out = ntohs(udp->dst_port);
}

static inline void
pktio_swap_addrs(em_event_t event)
{
	odp_packet_t pkt = pktio_odp_packet_get(event);
	void *pkt_data;
	odph_ethhdr_t *eth;
	odph_ethaddr_t eth_tmp_addr;
	odph_ipv4hdr_t *ip;
	odp_u32be_t ip_tmp_addr;
	odph_udphdr_t *udp;
	odp_u16be_t udp_tmp_port;

	/*
	 * if (odp_packet_has_eth(pkt)) {
	 *	eth = (odph_ethhdr_t *)odp_packet_l2_ptr(pkt, NULL);
	 *	eth_tmp_addr = eth->dst;
	 *	eth->dst = eth->src;
	 *	eth->src = eth_tmp_addr;
	 * }
	 *
	 * if (odp_packet_has_ipv4(pkt)) {
	 *	ip = (odph_ipv4hdr_t *)odp_packet_l3_ptr(pkt, NULL);
	 *	ip_tmp_addr = ip->src_addr;
	 *	ip->src_addr = ip->dst_addr;
	 *	ip->dst_addr = ip_tmp_addr;
	 * }
	 *
	 * if (odp_packet_has_udp(pkt)) {
	 *	udp = (odph_udphdr_t *)odp_packet_l4_ptr(pkt, NULL);
	 *	udp_tmp_port = udp->src_port;
	 *	udp->src_port = udp->dst_port;
	 *	udp->dst_port = udp_tmp_port;
	 * }
	 */

	/* Note: no actual checks if headers are present */
	pkt_data = odp_packet_data(pkt);
	eth = (odph_ethhdr_t *)pkt_data;
	ip = (odph_ipv4hdr_t *)((uintptr_t)pkt_data +
				sizeof(odph_ethhdr_t));
	udp = (odph_udphdr_t *)((uintptr_t)ip +
				sizeof(odph_ipv4hdr_t));
	eth_tmp_addr = eth->dst;
	eth->dst = eth->src;
	eth->src = eth_tmp_addr;

	ip_tmp_addr = ip->src_addr;
	ip->src_addr = ip->dst_addr;
	ip->dst_addr = ip_tmp_addr;

	udp_tmp_port = udp->src_port;
	udp->src_port = udp->dst_port;
	udp->dst_port = udp_tmp_port;
}

static inline em_event_t
pktio_copy_event(em_event_t event)
{
	odp_packet_t pkt = pktio_odp_packet_get(event);
	odp_pool_t pool = pktio_pool_get();
	odp_packet_t new_pkt = odp_packet_copy(pkt, pool);

	if (unlikely(new_pkt == ODP_PACKET_INVALID)) {
		APPL_EXIT_FAILURE("odp packet copy failed!\n");
		return EM_EVENT_UNDEF;
	}

	return pktio_em_event_get(new_pkt);
}

/**
 * Convert an IP-address to ascii string format.
 */
static inline void
ipaddr_tostr(uint32_t ip_addr, char *const ip_addr_str__out, int strlen)
{
	unsigned char *const ucp = (unsigned char *)&ip_addr;

#if ODP_BYTE_ORDER == ODP_LITTLE_ENDIAN
	snprintf(ip_addr_str__out, strlen, "%d.%d.%d.%d",
		 ucp[3] & 0xff, ucp[2] & 0xff, ucp[1] & 0xff, ucp[0] & 0xff);
#elif ODP_BYTE_ORDER == ODP_BIG_ENDIAN
	snprintf(ip_addr_str__out, strlen, "%d.%d.%d.%d",
		 ucp[0] & 0xff, ucp[1] & 0xff, ucp[2] & 0xff, ucp[3] & 0xff);
#else
	#error ODP_BYTE_ORDER invalid
#endif

	ip_addr_str__out[strlen - 1] = '\0';
}

#ifdef __cplusplus
}
#endif

#endif /* CM_PKTIO_H */
