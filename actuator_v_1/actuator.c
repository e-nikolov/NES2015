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


#include "dev/button-sensor.h"
#include "dev/light-sensor.h"
#include "dev/leds.h"

#include "../mycommon.h"


/*---------------------------------------------------------------------------*/
// Setup the network when button is pressed.

// If button is pressed, wait for a broadcast from an actuator.

/* This MEMB() definition defines a memory pool from which we allocate
   neighbor entries. */
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);

/* The neighbors_list is a Contiki list that holds the neighbors we
   have seen thus far. */
LIST(neighbors_list);


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


static void
recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
	printf("receiving runicast from sensor %d\n", from->u16);
	struct neighbor *n;
	struct runicast_message *m;
	uint8_t seqno_gap;

	/* The packetbuf_dataptr() returns a pointer to the first data byte
	 in the received packet. */
	m = packetbuf_dataptr();

	int node_id = 0;
	/* Check if we already know this neighbor. */
	for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
		/* We break out of the loop if the address of the neighbor matches
		   the address of the neighbor from which we received this
		   broadcast message. */
		if(linkaddr_cmp(&n->addr, from)) {
		  break;
		}
		node_id++;

	}

	// i-th neighbor should transmit at clock_time() + k * INTERVAL + i * (INTERVAL / MAX_SENSORS)
//	long expected_time = clock_seconds() % TIME_INTERVAL + node_id * TIME_INTERVAL / MAX_NEIGHBORS;

	// received = clock_seconds();
	// expected = k * TIME_INTERVAL + node_id * TIME_INTERVAL / MAX_NEIGHBORS; // for some k
	// late = received - expected;
	// early = expected - received;
	// next = TIME_INTERVAL + early;
	//
	int next_time = TIME_INTERVAL - (clock_seconds() % TIME_INTERVAL - node_id * TIME_INTERVAL / MAX_NEIGHBORS);
	if(next_time < TIME_INTERVAL / 2) next_time += TIME_INTERVAL;

	printf("sensor %d should send again in %d seconds\n", from->u16, next_time);

//	long received_time = clock_seconds() % TIME_INTERVAL;



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

	/* Print out a message. */
	printf("runicast message received from %d.%d with seqno %d, RSSI %u, LQI %u, avg seqno gap %d.%02d\n",
		 from->u8[0], from->u8[1],
		 m->seqno,
		 packetbuf_attr(PACKETBUF_ATTR_RSSI),
		 packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY),
		 (int)(n->avg_seqno_gap / SEQNO_EWMA_UNITY),
		 (int)(((100UL * n->avg_seqno_gap) / SEQNO_EWMA_UNITY) % 100));

	struct runicast_message msg;

	printf("sending runicast to %d\n", from->u16);

	msg.type = RUNICAST_TYPE_SCHEDULE;
	msg.data = next_time;

	packetbuf_copyfrom(&msg, sizeof(msg));
	runicast_send(&runicast, from, MAX_RETRANSMISSIONS);
}


static void
recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from)
{
	printf("IGNORE BROADCAST\n");
//	printf("daddy addr is %s.\n", daddy_addr == NULL ? "NULL" : "not NULL");
//	printf("daddy addr is %s.\n", &daddy_addr == NULL ? "NULL" : "not NULL");

    // endif
}


PROCESS(actuator_node_setup_process, "sensor cast");
AUTOSTART_PROCESSES(&actuator_node_setup_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(actuator_node_setup_process, ev, data)
{

	PROCESS_EXITHANDLER()
	PROCESS_BEGIN();

	SENSORS_ACTIVATE(button_sensor);
	SENSORS_ACTIVATE(light_sensor); // activate sensor


	while(1) {


		//	    process_exit(&data_sender_process);

		broadcast_open(&broadcast, 129, &broadcast_callbacks);
		// Wait for broadcast from actuator.


		printf("actuator_node_setup_process: Daddy's here\n");
	    packetbuf_copyfrom("Hello", 6);
		broadcast_send(&broadcast);

		broadcast_close(&broadcast);


		runicast_close(&runicast);
		runicast_open(&runicast, 130, &runicast_callbacks);

		printf("waiting for runicast from sensors\n");

		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor); // wait for button press event
	}

	PROCESS_END();
}


// Wait for unicasts with data. If we get something from a new node, put it in the next available time slot and tell it when it should send data next.
