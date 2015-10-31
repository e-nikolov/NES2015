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

#include "sensor_data_sender.h"
#include "sensor_node_setup.h"
#include "../mycommon.h"

MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);
LIST(history_table);


linkaddr_t *daddy_addr;


// Receive new time delay.
static void
recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
//	struct history_entry *e = NULL;
//
//	for(e = list_head(history_table); e != NULL; e = e->next)
//	{
//		if(linkaddr_cmp(&e->addr, from))
//		{
//			break;
//		}
//	}
//
//	if(e == NULL)
//	{
//		/* Create new history entry */
//		e = memb_alloc(&history_mem);
//		if(e == NULL)
//		{
//			e = list_chop(history_table); /* Remove oldest at full history */
//		}
//		linkaddr_copy(&e->addr, from);
//		e->seq = seqno;
//		list_push(history_table, e);
//	}
//	else
//	{
//		/* Detect duplicate callback */
//		if(e->seq == seqno)
//		{
//			printf("runicast message received from %d.%d, seqno %d (DUPLICATE)\n",
//					from->u8[0], from->u8[1], seqno);
//		}
//		/* Update existing history entry */
//		e->seq = seqno;
//	}

	struct runicast_message *received_msg = packetbuf_dataptr();

	printf("runicast message received from %d, data: %d\n",
			from->u16, received_msg->data);


	clock_time_t asd = -1;
	printf("lalalala %l\n", (clock_time_t)(1));
	printf("lalalala %lu\n", (clock_time_t)(1));
	printf("lalalala %d\n", (clock_time_t)(1));


	if (received_msg->type == RUNICAST_TYPE_SCHEDULE)
	{
		schedule_set = 1;
		time_delay = (clock_time_t)(received_msg->data);
		printf("should send again in %lu\n", time_delay);
		time_delay *= 1000;

		printf("should send again in %lu\n", time_delay);
		process_post(&data_sender_process, NEW_TIMER_RECEIVED_EVENT, time_delay);
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
}

static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
			to->u8[0], to->u8[1], retransmissions);
}



/*
 * now we define what to do on receiving, sending or timing out a runicast_msg or broadcast
 */

static void
recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from)
{
	//printf("daddy addr is %s.\n", daddy_addr == NULL ? "NULL" : "not NULL");
	//printf("daddy addr is %s.\n", &daddy_addr == NULL ? "NULL" : "not NULL");


	// if this is a network setup packet, sent from an actuator
    leds_toggle(LEDS_ALL); // toggle all leds

    linkaddr_copy(&daddy_addr, from);

	printf("receiving from %d\n", from->u16);
	printf("daddy_addr: %d\n", daddy_addr);

	char * received_msg = packetbuf_dataptr();

	printf("received: %s\n", received_msg);

    // Start data sending process.
    process_start(&data_sender_process, NULL);

    // remove the broadcast listener.
	broadcast_close(&broadcast);

    // endif
}

/*---------------------------------------------------------------------------*/
// Setup the network when button is pressed.

// If button is pressed, wait for a broadcast from an actuator.

PROCESS(sensor_node_setup_process, "sensor cast");
AUTOSTART_PROCESSES(&sensor_node_setup_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_node_setup_process, ev, data)
{

	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();

	SENSORS_ACTIVATE(button_sensor);
	SENSORS_ACTIVATE(light_sensor); // activate sensor


	while(1) {
		// Wait for broadcast from actuator.

	    process_exit(&data_sender_process);

		printf("sensor_cast_process: waiting for daddy_addr\n");
		broadcast_open(&broadcast, 129, &broadcast_callbacks);

		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor); // wait for button press event
	}

	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS(data_sender_process, "Data Sender");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(data_sender_process, ev, data)
{
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();


	runicast_open(&runicast, 130, &runicast_callbacks);

	while(1)
	{
		if(!schedule_set) {
			// TODO select a suitable random time;
			//time_delay = 2 * (random_rand() % 8);
			time_delay = 5000;
		}

		static struct etimer et;
		etimer_set(&et,  (time_delay * CLOCK_SECOND) / 1000);

		// Wait either for a timeout or for an event from the runicast receiver.
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et) || ev == NEW_TIMER_RECEIVED_EVENT);

		if(ev == NEW_TIMER_RECEIVED_EVENT) {
			// time_delay = data;

			printf("sleeping for %lu\n", time_delay);
			SLEEP_THREAD(time_delay);
		}

		printf("sensor_cast_process: main loop\n");
		struct runicast_message msg;

		printf("sending runicast to %d.%d\n", daddy_addr);

		msg.type = RUNICAST_TYPE_TEMP;
		msg.data = random_rand() % 10;

		packetbuf_copyfrom(&msg, sizeof(msg));
		runicast_send(&runicast, &daddy_addr, MAX_RETRANSMISSIONS);

	}


	PROCESS_END();
}
