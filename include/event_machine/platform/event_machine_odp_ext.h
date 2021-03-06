/*
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

/**
 * @file
 *
 * Event Machine ODP API extensions
 *
 */

#ifndef EVENT_MACHINE_ODP_EXT_H
#define EVENT_MACHINE_ODP_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <odp_api.h>
#include <event_machine/api/event_machine_types.h>
#include <event_machine/platform/event_machine_hw_types.h>

/**
 * Get the associated ODP queue.
 *
 * @param queue          EM queue
 *
 * @return odp queue if successful, ODP_QUEUE_INVALID on error
 */
odp_queue_t
em_odp_queue_odp(const em_queue_t queue);

/**
 * Get the associated EM queue.
 *
 * @param queue          ODP queue
 *
 * @return em queue if successful, EM_QUEUE_UNDEF on error
 */
em_queue_t
em_odp_queue_em(const odp_queue_t queue);

/**
 * Get EM event header size.
 *
 * Needed when user has to configure separate pool for packet I/O and allocate
 * EM events from there.
 *
 * @return em event header size.
 */
uint32_t
em_odp_event_hdr_size(void);

/**
 * Convert EM event handle to ODP event handle.
 *
 * @return ODP event handle.
 */
odp_event_t
em_odp_event2odp(em_event_t event);

/**
 * Convert EM event handles to ODP event handles
 *
 * @return ODP event handle array
 */
odp_event_t *
em_odp_events2odp(em_event_t events[]);

/**
 * Convert ODP event handle to EM event handle.
 *
 * @return EM event handle.
 */
em_event_t
em_odp_event2em(odp_event_t event);

/**
 * Convert ODP event handles to EM event handles.
 *
 * @return EM event handle array.
 */
em_event_t *
em_odp_events2em(odp_event_t odp_events[]);

/**
 * Enqueue packets into EM (from outside of EM, not allocated by em_alloc())
 */
int
pkt_enqueue(odp_packet_t pkt_tbl[], const int num, const em_queue_t queue);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_MACHINE_ODP_EXT_H */
