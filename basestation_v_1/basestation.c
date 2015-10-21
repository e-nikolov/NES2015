/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         An example of how the Mesh primitive can be used.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "net/rime/mesh.h"
#include "net/rime/collect.h"
#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
#include <string.h>

static struct mesh_conn mesh;
/*---------------------------------------------------------------------------*/
PROCESS(basestation_process, "base station");
AUTOSTART_PROCESSES(&basestation_process);
/*---------------------------------------------------------------------------*/
struct broadcast
{
	uint8_t type;
	int16_t data;
};
struct mesh_message
{
	int type;
	int data;
};
enum
{
	BROADCAST_TYPE_DISCOVERY,
	MESH_TYPE_TEMP,
	MESH_TYPE_HUMID
};

static struct broadcast_conn broadcast;
uint16_t time_delay;
//int address_base[2] = {rimeaddr_node_addr.u8[1], rimeaddr_node_addr.u8[0]};

static void
recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from)
{
	// do nothing for now
}

static void
sent(struct mesh_conn *c)
{
  printf("packet sent\n");
}

static void
timedout(struct mesh_conn *c)
{
  printf("packet timedout\n");
}

static void
recv(struct mesh_conn *c, const linkaddr_t *from, uint8_t hops)
{
  struct mesh_message * received_message;

  received_message = packetbuf_dataptr();
  uint8_t *data = received_message->type;
  printf("Type == %d\n",received_message->type);
  printf("Data received from %d.%d: %d (%d)\n",
	 from->u8[0], from->u8[1], &received_message->data, packetbuf_datalen());

  //packetbuf_copyfrom(received_message, sizeof(received_message));
  //mesh_send(&mesh, from);
}
static const struct broadcast_callbacks broadcast_callbacks = {recv_broadcast};
const static struct mesh_callbacks callbacks = {recv, sent, timedout};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(basestation_process, ev, data)
{
	PROCESS_EXITHANDLER(mesh_close(&mesh);)
	PROCESS_BEGIN();

	broadcast_open(&broadcast, 55, &broadcast_callbacks);

	static struct etimer et, dt;
	struct broadcast br_msg;

	etimer_set(&et, CLOCK_SECOND * 1);

	PROCESS_WAIT_UNTIL(etimer_expired(&et));
	printf("basestation: broadcast to neighbors\n");
	br_msg.type = BROADCAST_TYPE_DISCOVERY;
	br_msg.data = linkaddr_node_addr.u8[0];
	packetbuf_copyfrom(&br_msg, sizeof(br_msg));
	broadcast_send(&broadcast);

	mesh_open(&mesh, 132, &callbacks);

  	while(1)
  	{
  		etimer_set(&dt, CLOCK_SECOND * 1);
  		PROCESS_WAIT_UNTIL(etimer_expired(&dt));
  	}
  	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
