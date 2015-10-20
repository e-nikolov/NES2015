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
static int check = 0;
char done[8] = {0,0,0,0,0,0,0,0};
static int timeout_cnt = 0;

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


/* These two defines are used for computing the moving average for the
   broadcast sequence number gaps. */
#define SEQNO_EWMA_UNITY 0x100
#define SEQNO_EWMA_ALPHA 0x040

#define MAX_RETRANSMISSIONS 4
#define NUM_HISTORY_ENTRIES 4

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
struct history_entry {
  struct history_entry *next;
  linkaddr_t addr;
  uint8_t seq;
};
LIST(history_table);
MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);
/*---------------------------------------------------------------------------*/
static void
recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  /* OPTIONAL: Sender history */
  struct history_entry *e = NULL;
  for(e = list_head(history_table); e != NULL; e = e->next) {
    if(linkaddr_cmp(&e->addr, from)) {
      break;
    }
  }
  if(e == NULL) {
    /* Create new history entry */
    e = memb_alloc(&history_mem);
    if(e == NULL) {
      e = list_chop(history_table); /* Remove oldest at full history */
    }
    linkaddr_copy(&e->addr, from);
    e->seq = seqno;
    list_push(history_table, e);
  } else {
    /* Detect duplicate callback */
    if(e->seq == seqno) {
      printf("runicast message received from %d.%d, seqno %d (DUPLICATE)\n",
	     from->u8[0], from->u8[1], seqno);
      return;
    }
    /* Update existing history entry */
    e->seq = seqno;
  }

  char * receive_msg = "null";
  char * address = "address";

  struct neighbor *n;
  struct broadcast_message *m;
  uint8_t seqno_gap;
  /* Grab the pointer to the incoming data. */
  receive_msg = packetbuf_dataptr();
  printf("runicast received with message %s\n",receive_msg);
  /* We have two message types, UNICAST_TYPE_PING and
	 UNICAST_TYPE_PONG. If we receive a UNICAST_TYPE_PING message, we
	 print out a message and return a UNICAST_TYPE_PONG. */
  if((strcmp(receive_msg,address)==0) && (flag == 0)) {
	printf("runicast ping received from %d.%d\n",
		   from->u8[0], from->u8[1]);
	flag = 2;
	//msg->type = UNICAST_TYPE_PONG;
	//packetbuf_copyfrom(msg, sizeof(struct unicast_message));
	/* Send it back to where it came from. */
	//unicast_send(c, from);
  }
  if ((flag == 2) && (strcmp(receive_msg,address)==0)){
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
static const struct broadcast_callbacks broadcast_call = {};
static struct broadcast_conn broadcast;
static void
sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
  check = 0;
}
static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
  check = 1;
}
static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     sent_runicast,
							     timedout_runicast};
static struct runicast_conn runicast;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;
  char * msg;
  struct neighbor *n;
  int randneighbor, i, j, x;
  static int k;

  PROCESS_EXITHANDLER(runicast_close(&runicast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(flag != 4) {
    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    switch (flag){
    case 0:
    	broadcast_open(&broadcast, 129, &broadcast_call);
        packetbuf_copyfrom("req", 4);
        broadcast_send(&broadcast);
        printf("broadcast message sent\n");
        runicast_open(&runicast, 146, &runicast_callbacks);
        broadcast_close(&broadcast);
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
        	  k++;
        	  msg = "schedule";
        	  printf("sending runicast to %d.%d with message %s\n", n->addr.u8[0], n->addr.u8[1], msg);

          	  packetbuf_copyfrom(msg, (strlen(msg)+1));
          	  runicast_send(&runicast, &n->addr, MAX_RETRANSMISSIONS);
			  if(check == 1){printf("blablablabla \n"); check = 0;}
        }
        if(k == nr_neighbors)
        {
        	flag = 4;
        	printf("schedule sent\n");
        }
    	break;
    default:
    	break;
    }
  }
  // out of init loop, into the schedule loop
  // schedule loop has to be divided into the amount of neighbours + 1. there is a maximum of 16 neighbours so we take a total period of 170 sec
  // in this way, each communication has 10 sec if there is a full neighbours list
  k = 0;

  while(1){
	    etimer_set(&et, CLOCK_SECOND * 10);

	    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	    // proces node n
	    n = list_head(neighbors_list);
	    for(j = 0; j < k; j=j+1)
	    {
	    	n = list_item_next(n);
	    }
	    if(k < nr_neighbors)
	    {
  	  	    msg = "data_send_req";
	    	printf("runicast data request %s\n", n->addr.u8[0], n->addr.u8[1], msg);
    	    packetbuf_copyfrom(msg, (strlen(msg)+1));
    	    runicast_send(&runicast, &n->addr, MAX_RETRANSMISSIONS);
		    if(check == 1){printf("blablablabla \n"); check = 0;}
	    }
	    printf("k = %d\n",k);
	    k++;
	    if(k == 17){k=0;}
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
