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
#define NUM_HISTORY_ENTRIES 2

/*---------------------------------------------------------------------------*/


PROCESS(sensor_cast_process, "sensor cast");

AUTOSTART_PROCESSES(&sensor_cast_process);
/*---------------------------------------------------------------------------*/

/*
 * create runicast_msg structure
 * create broadcast_msg structure (unneccesary?)
 * create a sender_history list to detect duplicate messages
 */
struct runicast_msg
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
	RUNICAST_TYPE_SCHEDULE,
	RUNICAST_TYPE_TEMP,
	RUNICAST_TYPE_HUMID
};

struct history_entry
{
	struct history_entry *next;
	linkaddr_t addr;
	uint8_t seq;
};

linkaddr_t *daddy_addr = NULL;

uint16_t time_delay;

MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);
LIST(history_table);

/*---------------------------------------------------------------------------*/
static struct runicast_conn runicast;
static struct broadcast_conn broadcast;

/*
 * now we define what to do on receiving, sending or timing out a runicast_msg or broadcast
 */

static void
recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from)
{
	if(daddy_addr == NULL)
	{
		linkaddr_copy(&daddy_addr, from);
	}

	broadcast_close(&broadcast);
}

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

	printf("runicast message received from %d.%d, seqno %d\n",
			from->u8[0], from->u8[1], seqno);

	struct runicast_msg *received_msg = packetbuf_dataptr();

	if (received_msg->type == RUNICAST_TYPE_SCHEDULE)
	{
		time_delay = received_msg->data;
	}
	else
	{
		printf("I received a runicast message that was not for me!\n")
	}
}

static void
sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message sent to %d.%d, retransmissions %d\n",
			to->u8[0], to->u8[1], retransmissions);
}

static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
			to->u8[0], to->u8[1], retransmissions);
}

static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     	 	 	 	 	 	 	 sent_runicast,
															 timedout_runicast};

static const struct broadcast_callbacks broadcast_callbacks = {recv_broadcast};

/*-----------------------------------------------------------------------------------*/



PROCESS_THREAD(sensor_cast_process, ev, data)
{
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_callbacks);

	PROCESS_WAIT_UNTILL(daddy_addr != NULL);

	runicast_open(&runicast, 129, &runicast_callbacks);

	time_delay = 2 * (random_rand() % 8);

	while(1)
	{

		static struct etimer et;
		struct runicast_message msg;

		etimer_set(&et, CLOCK_SECOND * time_delay);

		PROCESS_WAIT_UNTIL(etimer_expired(&et));

		printf("sending runicast to %d.%d\n", daddy_addr.u8[0], daddy_addr.u8[1]);

		msg.type = RUNICAST_TYPE_TEMP;
		msg.data = random_rand() % 10;

		packetbuf_copyfrom(&msg, sizeof(msg));
		runicast_send(&runicast, &daddy_addr, MAX_RETRANSMISSIONS);
	}


	PROCESS_END();
}



