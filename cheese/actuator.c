/*
 * This file contains code for a sensor node that, upon initialisation, waits for a
 * broadcast from a parent, saves the adress (rime) of this parent in it's parent_list
 * (containing just 1 parent!) and then sends data to this parent using runicast.
 *
 * Further it contains a counter for the amount of failed runicast, if this reaches
 * a certain threshold it shuts down.
 */

/*
 * first, include the neccesary packages.
 */

#include <stdio.h>

#include "contiki.h"
#include "../mycommon.h"
#include "net/rime/rime.h"

#include "lib/list.h"
#include "lib/memb.h"

#include "dev/leds.h"

/*
 * Then we define the values needed for runicast to be reliable ;), wich are
 * the amount of allowable rettransmissions before accepting failure, and
 * the amount of message id's in the history list, wich should be enough
 * to contain at least one message from each parent node (so, one :D)
 */

#define MAX_RETRANSMISSIONS 4
#define MAX_NEIGHBORS 16
#define NUM_HISTORY_ENTRIES 2

/*---------------------------------------------------------------------------*/


PROCESS(actuator_cast_process, "actuator cast");

AUTOSTART_PROCESSES(&actuator_cast_process);
/*---------------------------------------------------------------------------*/

/*
 * create runicast_msg structure
 * create broadcast_msg structure (unneccesary?)
 * create a sender_history list to detect duplicate messages
 */
struct runicast_message
{
	uint8_t type;
	int16_t data;
	int16_t actuator_id;
};
struct broadcast
{
	uint8_t type;
	int16_t data;
};
/*
struct broadcast_msg
{
	uint8_t type;
};
*/
enum
{
	BROADCAST_TYPE_DISCOVERY,
	RUNICAST_TYPE_SCHEDULE,
	RUNICAST_TYPE_TEMP,
	RUNICAST_TYPE_HUMID
};

linkaddr_t *daddy_addr = NULL;

uint16_t time_delay;
int16_t hop_id = 0;
int16_t flag = 0;
int16_t runicast_flag = 0;

/*---------------------------------------------------------------------------*/
/* This structure holds information about neighbors. */
struct neighbor {
  /* The ->next pointer is needed since we are placing these on a
     Contiki list. */
  struct neighbor *next;

  /* The ->addr field holds the Rime address of the neighbor. */
  linkaddr_t addr;
};
struct history_entry {
  struct history_entry *next;
  linkaddr_t addr;
  uint8_t seq;
};
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);
MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);
LIST(history_table);
LIST(neighbors_list);
static struct runicast_conn runicast;
static struct broadcast_conn broadcast;

/*
 * now we define what to do on receiving, sending or timing out a runicast_msg or broadcast
 */

static void
recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from)
{
	struct broadcast *received_msg = packetbuf_dataptr();
	struct neighbor *n;
	struct broadcast br_msg;
	//check if this is first time of discovery message
	if(hop_id==0)
	{
		printf("new hop_id: %d",hop_id);
		printf("actuator: broadcast to neighbors\n");
		hop_id = received_msg->data + 1;
		br_msg.type = BROADCAST_TYPE_DISCOVERY;
		br_msg.data = hop_id;
		packetbuf_copyfrom(&br_msg, sizeof(br_msg));
		broadcast_send(&broadcast);
		flag = 1;
	}
	// add neighbor if broadcast hop_id is lower than its own hop_id
	//
	if(hop_id == (received_msg->data - 1))
	{
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

		/* Place the neighbor on the neighbor list. */
		list_add(neighbors_list, n);
	}
}

static const struct broadcast_callbacks broadcast_callbacks = {recv_broadcast};

static void
recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
	struct history_entry *e = NULL;

	for(e = list_head(history_table); e != NULL; e = e->next)
	{
		if(linkaddr_cmp(&e->addr, from))
		{
			break;
		}
	}

	if(e == NULL)
	{
		/* Create new history entry */
		e = memb_alloc(&history_mem);
		if(e == NULL)
		{
			e = list_chop(history_table); /* Remove oldest at full history */
		}
		linkaddr_copy(&e->addr, from);
		e->seq = seqno;
		list_push(history_table, e);
	}
	else
	{
		/* Detect duplicate callback */
		if(e->seq == seqno)
		{
			printf("runicast message received from %d.%d, seqno %d (DUPLICATE)\n",
					from->u8[0], from->u8[1], seqno);
		}
		/* Update existing history entry */
		e->seq = seqno;
	}

	printf("Basestation: runicast message received from %d.%d, seqno %d\n",
			from->u8[0], from->u8[1], seqno);
	int16_t k, j;
	int nr_neighbors = list_length(neighbors_list);
	struct runicast_message *received_msg = packetbuf_dataptr();
	if (received_msg->type == (RUNICAST_TYPE_TEMP || RUNICAST_TYPE_HUMID))
	{
		if(nr_neighbors > 0)
		{
			  while ((runicast_flag != 1) && (k != nr_neighbors))
			  {
				  n = list_head(neighbors_list);
				  for(j = 0; j < k; j++)
				  {
					  n = list_item_next(n);
				  }
				  k++;
				  printf("transfer runicast to %d.%d with message %d\n", n->addr.u8[0], n->addr.u8[1], received_msg->data);
				  packetbuf_copyfrom(&received_msg, sizeof(received_msg));
				  runicast_send(c, &n->addr, MAX_RETRANSMISSIONS);
			  }
		}
		else
		{
			printf("no neighbors found!\n");
		}
		runicast_flag = 0;
	}
	else
	{
		printf("I received a runicast message that was not for me!\n");
	}
}

static void
sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{

	printf("runicast message sent to %d.%d, retransmissions %d\n",
			to->u8[0], to->u8[1], retransmissions);
	runicast_flag = 1;
}

static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
			to->u8[0], to->u8[1], retransmissions);
	runicast_flag = 2;
}

static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     	 	 	 	 	 	 	 sent_runicast,
															 timedout_runicast};
/*-----------------------------------------------------------------------------------*/



PROCESS_THREAD(sensor_cast_process, ev, data)
{
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();

	broadcast_open(&broadcast, 55, &broadcast_callbacks);

	//time_delay = 2 * (random_rand() % 8);

	static struct etimer et;
	struct runicast_message ru_msg;


	while(1)
	{
		etimer_set(&et, CLOCK_SECOND * 10+random_rand()%128);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		switch(flag){
		case 0:
			break;
		case 1:
			ru_msg.type = RUNICAST_TYPE_TEMP;
			ru_msg.data = 42;
			ru_msg.actuator_id = random_rand()%9;
			packetbuf_copyfrom(&ru_msg, sizeof(ru_msg));
			runicast_send(&runicast_callbacks, &n->addr, MAX_RETRANSMISSIONS);
			break;
		default:
			break;
		}
	}
	PROCESS_END();
}



