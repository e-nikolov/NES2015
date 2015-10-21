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


#include "dev/button-sensor.h"
#include "dev/light-sensor.h"
#include "dev/leds.h"


/*---------------------------------------------------------------------------*/
// Setup the network when button is pressed.

// If button is pressed, wait for a broadcast from an actuator.


static struct broadcast_conn broadcast;

static const struct broadcast_callbacks broadcast_callbacks = {};

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

		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor); // wait for button press event
	}

	PROCESS_END();
}


