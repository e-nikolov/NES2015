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
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
static int flag = 0;
static int nr_tries = 1;
/* This is the structure of broadcast messages. */
struct broadcast_message {
  uint8_t seqno;
};

/* This is the structure of unicast ping messages. */
struct unicast_message {
  uint8_t type;
};

/* These are the types of unicast messages that we can send. */
enum {
  UNICAST_TYPE_PING,
  UNICAST_TYPE_PONG,
  UNICAST_TYPE_ACK
};

/* This structure holds information about neighbors. */
struct neighbor {
  /* The ->next pointer is needed since we are placing these on a
     Contiki list. */
  struct neighbor *next;

  /* The ->addr field holds the Rime address of the neighbor. */
  linkaddr_t addr;

  /* The ->last_rssi and ->last_lqi fields hold the Received Signal
     Strength Indicator (RSSI) and CC2420 Link Quality Indicator (LQI)
     values that are received for the incoming broadcast packets. */
  uint16_t last_rssi, last_lqi;

  /* Each broadcast packet contains a sequence number (seqno). The
     ->last_seqno field holds the last sequenuce number we saw from
     this neighbor. */
  uint8_t last_seqno;

  /* The ->avg_gap contains the average seqno gap that we have seen
     from this neighbor. */
  uint32_t avg_seqno_gap;

};

/* This #define defines the maximum amount of neighbors we can remember. */
#define MAX_NEIGHBORS 16

/* This MEMB() definition defines a memory pool from which we allocate
   neighbor entries. */
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);

/* The neighbors_list is a Contiki list that holds the neighbors we
   have seen thus far. */
LIST(neighbors_list);

/* These hold the broadcast and unicast structures, respectively. */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

/* These two defines are used for computing the moving average for the
   broadcast sequence number gaps. */
#define SEQNO_EWMA_UNITY 0x100
#define SEQNO_EWMA_ALPHA 0x040
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  char * rcv_bfr;
  char * Request = "req";

  struct neighbor *n;
  struct broadcast_message *m;
  uint8_t seqno_gap;

  rcv_bfr = (char *)packetbuf_dataptr();
  if((strcmp(rcv_bfr, Request)==0) && (flag == 0))
  {
	     printf("broadcast message received from %d.%d: '%s'\n",
	    		 from->u8[0], from->u8[1], rcv_bfr);
         flag = 1;
  }
  if ((flag == 1) && (strcmp(rcv_bfr, Request)==0)){
	  /* The packetbuf_dataptr() returns a pointer to the first data byte
	     in the received packet. */
	  m = packetbuf_dataptr();

	  /* Check if we already know this neighbor. */
	  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {

	    /* We break out of the loop if the address of the neighbor matches
	       the address of the neighbor from which we received this
	       broadcast message. */
	    if(linkaddr_cmp(&n->addr, from)) {
	      break;
	    }
	  }

	  /* If n is NULL, this neighbor was not found in our list, and we
	     allocate a new struct neighbor from the neighbors_memb memory
	     pool. */
	  if(n == NULL) {
	    n = memb_alloc(&neighbors_memb);

	    /* If we could not allocate a new neighbor entry, we give up. We
	       could have reused an old neighbor entry, but we do not do this
	       for now. */
	    if(n == NULL) {
	      return;
	    }

	    /* Initialize the fields. */
	    linkaddr_copy(&n->addr, from);
	    n->last_seqno = m->seqno - 1;
	    n->avg_seqno_gap = SEQNO_EWMA_UNITY;

	    /* Place the neighbor on the neighbor list. */
	    list_add(neighbors_list, n);
	    printf("actuator added to neighbor list\n");
	  }

	  /* We can now fill in the fields in our neighbor entry. */
	  n->last_rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	  n->last_lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);

	  /* Compute the average sequence number gap we have seen from this neighbor. */
	  seqno_gap = m->seqno - n->last_seqno;
	  n->avg_seqno_gap = (((uint32_t)seqno_gap * SEQNO_EWMA_UNITY) *
	                      SEQNO_EWMA_ALPHA) / SEQNO_EWMA_UNITY +
	                      ((uint32_t)n->avg_seqno_gap * (SEQNO_EWMA_UNITY -
	                                                     SEQNO_EWMA_ALPHA)) /
	    SEQNO_EWMA_UNITY;

	  /* Remember last seqno we heard. */
	  n->last_seqno = m->seqno;
	  flag = 2;
  }
  rcv_bfr = "null";
}
/* This function is called for every incoming unicast packet. */
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  char * receive_msg;
  char * ack = "address_ack";
  struct unicast_message *msg;
  /* Grab the pointer to the incoming data. */
  receive_msg = packetbuf_dataptr();
  printf("actuator unicast received with message %s\n",receive_msg);
  /* We have two message types, UNICAST_TYPE_PING and
	 UNICAST_TYPE_PONG. If we receive a UNICAST_TYPE_PING message, we
	 print out a message and return a UNICAST_TYPE_PONG. */
  switch(flag)
  {
 	  case 2:
 		  if(strcmp(receive_msg,ack)==0) {
 			printf("ack received from actuator, waiting for schedule\n");
 			flag = 3;
 			//msg->type = UNICAST_TYPE_PONG;
 			//packetbuf_copyfrom(msg, sizeof(struct unicast_message));
 			/* Send it back to where it came from. */
 			//unicast_send(c, from);
 		  }
	  break;
 	  default:
      break;
  }
  receive_msg = "null";
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;
  struct unicast_message msg;
  struct neighbor *n;
  char * message = "address";
  int randneighbor, i;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);


  while(1) {
	  //etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % CLOCK_SECOND*8);
	  etimer_set(&et, CLOCK_SECOND * 1);
	  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	  switch(flag)
	  {
	  case 0:
		  break;
	  case 2:
		  broadcast_close(&broadcast);
		  //broadcast_close(&broadcast);
		  unicast_open(&unicast, 146, &unicast_callbacks);
	      randneighbor = random_rand() % list_length(neighbors_list);
          n = list_head(neighbors_list);
          //for(i = 0; i < randneighbor; i++) {
          //  n = list_item_next(n);
          //}          nr_tries++;

          printf("sending unicast to %d.%d with message %s, nr. tries: %d\n", n->addr.u8[0], n->addr.u8[1],message, nr_tries);

          nr_tries++;
		  if(nr_tries>9)
		  {
			  flag = 0;
			  printf("tries overflow!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		  }
          //msg.type = UNICAST_TYPE_ACK;
          packetbuf_copyfrom(message, (strlen(message)+1));
          unicast_send(&unicast, &n->addr);

/*          packetbuf_copyfrom("address", 8);
		  broadcast_send(&broadcast);
		  printf("broadcast to actuator sent\n");*/

	  break;
	  case 3:
		  flag = 3;
	  break;
	  default:
		  flag = 0;
	  break;
	  }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
