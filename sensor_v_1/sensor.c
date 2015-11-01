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
#include "sensor.h";

MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);
LIST(history_table);


linkaddr_t *actuator_address;


// Receive new time delay.
static void
recv_runicast_schedule(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
	struct runicast_message *received_msg = packetbuf_dataptr();

	printf("Runicast received from actuator with address %d, data: %d\n",
			from->u16, received_msg->data);

	if (received_msg->type == RUNICAST_TYPE_SCHEDULE)
	{
		schedule_set = 1;
		time_delay = (clock_time_t)(received_msg->data);
		printf("Should send again in %lu seconds\n", time_delay);
		time_delay *= 1000;
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

	printf("Runicast message sent to %d, attempts: %d\n",
			to->u16, retransmissions);

}

static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("Runicast message timed out when sending to %d, attempts: %d\n",
			to->u16, retransmissions);
}



/*
 * now we define what to do on receiving, sending or timing out a runicast_msg or broadcast
 */

static void
recv_broadcast_actuator_adv(struct broadcast_conn *c, const linkaddr_t *from)
{
    leds_toggle(LEDS_ALL); // toggle all leds

    linkaddr_copy(&actuator_address, from);

	char * received_msg = packetbuf_dataptr();

	printf("Receiving an actuator advertisement from %d\n", from->u16);
//	printf("Received: %s\n", received_msg);

    // Start data sending process.
    process_start(&data_sender_process, NULL);

    // remove the broadcast listener.
	broadcast_close(&broadcast);
}

/*---------------------------------------------------------------------------*/
// Setup the network when button is pressed.

// If button is pressed, wait for a broadcast from an actuator.

PROCESS(sensor_node_setup_process, "sensor node setup process");
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

		printf("Waiting for an actuator advertisement.\n");
		broadcast_open(&broadcast, 129, &actuator_adv_broadcast_callbacks);

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


	runicast_open(&runicast, 130, &runicast_schedule_callbacks);

	while(1)
	{
		if(!schedule_set) {
			// Random time between 5 and 15 seconds
			time_delay = 5000 + (abs(random_rand() % 10000));
			//time_delay = 5000;

			printf("Will wait for a schedule for a random time (%lu seconds) before retrying.\n", time_delay / 1000);
		}

		static struct etimer et;
		etimer_set(&et,  (time_delay * CLOCK_SECOND) / 1000);

		// Wait either for a timeout or for an event from the schedule receiver.
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et) || ev == NEW_TIMER_RECEIVED_EVENT);

		if(ev == NEW_TIMER_RECEIVED_EVENT) {
			// time_delay = data;

			printf("Sleeping for %lu seconds.\n", time_delay / 1000);
			SLEEP_THREAD(time_delay);
		}

		struct runicast_message msg;

		printf("Sending data to actuator\n", actuator_address->u16);

		msg.type = RUNICAST_TYPE_TEMP;
		msg.data = random_rand() % 10;

		packetbuf_copyfrom(&msg, sizeof(msg));
		runicast_send(&runicast, &actuator_address, MAX_RETRANSMISSIONS);

	}


	PROCESS_END();
}
