/*
 * sensor_node_setup.c
 *
 *  Created on: 21 Oct 2015
 *      Author: enikolov
 */


#include <stdio.h>

#include "contiki.h"
#include "../mycommon.h"
#include "net/rime/rime.h"

#include "lib/list.h"
#include "lib/memb.h"


#include "dev/button-sensor.h"
#include "dev/light-sensor.h"
#include "dev/leds.h"
#include "sensor_data_sender.h"


static struct runicast_conn runicast;
static struct broadcast_conn broadcast;

/*
 * now we define what to do on receiving, sending or timing out a runicast_msg or broadcast
 */

static void
recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from)
{
	linkaddr_t daddy_addr;

	// if this is a network setup packet, sent from an actuator
    leds_toggle(LEDS_ALL); // toggle all leds

    linkaddr_copy(&daddy_addr, from);

    // Start data sending process.
    process_start(&data_sender_process, NULL);
    // remove the broadcast listener.
	broadcast_close(&broadcast);

    // endif
}


static const struct broadcast_callbacks broadcast_callbacks = {recv_broadcast};

/*---------------------------------------------------------------------------*/
// Setup the network when button is pressed.

// If button is pressed, wait for a broadcast from an actuator.

PROCESS(sensor_node_setup_process, "sensor node setup");
AUTOSTART_PROCESSES(&sensor_node_setup_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_node_setup_process, ev, data)
{

	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();

	SENSORS_ACTIVATE(button_sensor);
	SENSORS_ACTIVATE(light_sensor); // activate sensor


	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor); // wait for button press event

		// Wait for broadcast from actuator.
		broadcast_open(&broadcast, 129, &broadcast_callbacks);

		printf("sensor_cast_process: waiting for daddy_addr\n");
	}

	PROCESS_END();
}


