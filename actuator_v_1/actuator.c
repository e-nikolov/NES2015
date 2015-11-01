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
#include "actuator.h"

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

	printf("Runicast message sent to %d, attempts: %d\n",
			to->u16, retransmissions);
}

static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("Runicast message timed out when sending to %d, attempts: %d\n",
			to->u16, retransmissions);
}


static void
recv_runicast_data(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
	printf("Receiving data from sensor %d\n", from->u16);
	struct neighbor *n;
	struct runicast_message *m;

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

	// received = clock_seconds();
	// expected = k * TIME_INTERVAL + node_id * TIME_INTERVAL / MAX_NEIGHBORS; // for some k
	// late = received - expected;
	// early = expected - received;
	// next = TIME_INTERVAL + early;
	//
	int next_time = TIME_INTERVAL - (clock_seconds() % TIME_INTERVAL - node_id * TIME_INTERVAL / MAX_NEIGHBORS);

	// if the next transmission time is too soon, delay it by 1 TIME_INTERVAL.

	if(next_time < TIME_INTERVAL / 2) next_time += TIME_INTERVAL;

	printf("Sensor %d should send again in %d seconds\n", from->u16, next_time);


	/* If n is NULL, this neighbor was not found in our list, and we
	 allocate a new struct neighbor from the neighbors_memb memory
	 pool. */
	if(n == NULL) {
		n = memb_alloc(&neighbors_memb);

		// If we could not allocate a new neighbor entry, we give up.
		if(n == NULL) {
		  return;
		}

		/* Initialize the fields. */
		linkaddr_copy(&n->addr, from);

		/* Place the neighbor on the neighbor list. */
		list_add(neighbors_list, n);
	}

	struct runicast_message msg;

	printf("Sending back to %d the time it should wait before transmitting again.\n", from->u16);

	msg.type = RUNICAST_TYPE_SCHEDULE;
	msg.data = next_time;

	packetbuf_copyfrom(&msg, sizeof(msg));
	runicast_send(&runicast, from, MAX_RETRANSMISSIONS);
}


static void
recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from)
{
	printf("IGNORE BROADCAST\n");
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
		printf("Press the button in order to broadcast an actuator advertisement.");

		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor); // wait for button press event
		printf("Button pressed.\n");

		//process_exit(&data_sender_process);

		broadcast_open(&broadcast, 129, &broadcast_callbacks);

		// Broadcast an actuator advertisement.
		printf("Broadcasting an actuator advertisement.\n");
	    packetbuf_copyfrom("Hello", 6);
		broadcast_send(&broadcast);
		broadcast_close(&broadcast);

		runicast_close(&runicast);
		runicast_open(&runicast, 130, &runicast_data_callbacks);
	}

	PROCESS_END();
}


// Wait for unicasts with data. If we get something from a new node, put it in the next available time slot and tell it when it should send data next.
