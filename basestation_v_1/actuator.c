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
#include "random.h"

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
uint8_t address_base=0;
//int address_base[2] = {rimeaddr_node_addr.u8[1], rimeaddr_node_addr.u8[0]};

static void
recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from)
{
	struct broadcast *received_msg;
	received_msg = packetbuf_dataptr();
	struct broadcast br_msg;
	if(received_msg->type == BROADCAST_TYPE_DISCOVERY)
	{
		if(received_msg->data != address_base)
		{
			address_base = received_msg->data;
			printf("base station addr: %d\n",address_base);
			printf("actuator: broadcast to neighbors\n");
			br_msg.type = BROADCAST_TYPE_DISCOVERY;
			br_msg.data = address_base;
			packetbuf_copyfrom(&br_msg, sizeof(br_msg));
			broadcast_send(&broadcast);
		}
	}
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
	struct mesh_message msg;

	while(address_base == 0)
	{
		etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % CLOCK_SECOND);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
	}
	mesh_open(&mesh, 132, &callbacks);

  	while(1)
  	{
  		linkaddr_t addr;
  		msg.data = 1;
  		msg.type = 3;
		etimer_set(&dt, CLOCK_SECOND * 5+random_rand()%128);
		PROCESS_WAIT_UNTIL(etimer_expired(&dt));
		packetbuf_copyfrom(&msg, sizeof(struct mesh_message));
	    addr.u8[0] = address_base;
	    addr.u8[1] = linkaddr_node_addr.u8[1];
		mesh_send(&mesh, &addr);
  	}
  	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
