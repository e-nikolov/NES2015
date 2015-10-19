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
char done[8] = {0,0,0,0,0,0,0,0};
static int timeout_cnt = 0;
int k = 0;
int unicast_counter = 0;

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

static int nr_neighbors = 0;
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
/*static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	  char * rcv_bfr;
	  char * address = "address";

	  struct neighbor *n;
	  struct broadcast_message *m;
	  uint8_t seqno_gap;
}*/
/* This function is called for every incoming unicast packet. */
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  char * receive_msg = "null";
  char * address = "address";

  struct neighbor *n;
  struct broadcast_message *m;
  uint8_t seqno_gap;
  /* Grab the pointer to the incoming data. */
  receive_msg = packetbuf_dataptr();
  printf("unicast received with message %s\n",receive_msg);
  /* We have two message types, UNICAST_TYPE_PING and
	 UNICAST_TYPE_PONG. If we receive a UNICAST_TYPE_PING message, we
	 print out a message and return a UNICAST_TYPE_PONG. */
  if((strcmp(receive_msg,address)==0) && (flag == 0)) {
	printf("unicast ping received from %d.%d\n",
		   from->u8[0], from->u8[1]);
	flag = 1;
	//msg->type = UNICAST_TYPE_PONG;
	//packetbuf_copyfrom(msg, sizeof(struct unicast_message));
	/* Send it back to where it came from. */
	//_unicast(c, from);
  }
  if ((flag == 1) && (strcmp(receive_msg,address)==0)){
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
	    nr_neighbors = nr_neighbors + 1;
	    printf("neighbor added\n");
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

  }
  receive_msg = "null";
}
static void _unicast(struct unicast_conn *c, const linkaddr_t *receiver)
{
	unicast_send(c, receiver);
	printf("unicast_counter: %d\n", ++unicast_counter);
}
static const struct broadcast_callbacks broadcast_call = {};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;
  char * msg;
  struct neighbor *n;
  int randneighbor, i, j, x;


  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 1);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    switch (flag){
    case 0:
    	broadcast_open(&broadcast, 129, &broadcast_call);
        packetbuf_copyfrom("req", 4);
        broadcast_send(&broadcast);
        printf("broadcast message sent\n");
        unicast_open(&unicast, 146, &unicast_callbacks);
    	break;
    case 1:
        if(list_length(neighbors_list) > 0) {
			  if(k == nr_neighbors)
			  {
				  k = 0;
			  }
        	  n = list_head(neighbors_list);
        	  while((done[k]==1) && (k<nr_neighbors))
        	  {
        		  k++;
        	  }
        	  if(k < nr_neighbors)
        	  {
				  for(j = 0; j < k; j++){
					  n = list_item_next(n);
				  }
				  done[k] = 1;
				  msg = "address_ack";
				  printf("sending unicast to %d.%d with message %s\n", n->addr.u8[0], n->addr.u8[1], msg);
				  packetbuf_copyfrom(msg, (strlen(msg)+1));
				  _unicast(&unicast, &n->addr);
        	  }
        	  printf("%d, %d\n",k,nr_neighbors);
        }
        if(timeout_cnt>(nr_neighbors+10))
        {
        	flag = 2;
        	k=0;
        }
		timeout_cnt++;

    break;
    case 2:
        if(list_length(neighbors_list) > 0) {
              if(k >= nr_neighbors)
              {
          	      k = 0;
              }
        	  n = list_head(neighbors_list);
        	  for(j = 0; j < k; j=j+1){
        		  n = list_item_next(n);
        	  }
        	  k=k+1;
        	  msg = "schedule";
        	  printf("sending unicast to %d.%d with message %s\n", n->addr.u8[0], n->addr.u8[1], msg);

          	  packetbuf_copyfrom(msg, (strlen(msg)+1));
          	  _unicast(&unicast, &n->addr);
        }
        if(k == nr_neighbors)
        {
        	flag = 4;
        	printf("schedule sent");
        }
    	break;
    case 4:
    break;
    default:
    	break;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
