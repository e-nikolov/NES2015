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


/*---------------------------------------------------------------------------*/
PROCESS(data_sender_process, "Data Sender");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(data_sender_process, ev, data)
{
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();

//	PROCESS_WAIT_EVENT_UNTIL(flag == 1);
//
//	printf("Flag == 1\n");
//
//	PROCESS_WAIT_WHILE(daddy_addr == NULL);
//	printf("sensor_cast_process: done waiting for daddy_addr\n");
//
//	printf("receiving from: %d\n", daddy_addr->u16);
//	printf("receiving from: %d %d\n", daddy_addr->u8[0], daddy_addr->u8[1]);
//	printf("daddy addr is %s.\n", daddy_addr == NULL ? "NULL" : "not NULL");
//	printf("daddy addr is %s.\n", &daddy_addr == NULL ? "NULL" : "not NULL");
//
//	runicast_open(&runicast, 129, &runicast_callbacks);
//
//	time_delay = 2 * (random_rand() % 8);
//
//	while(1)
//	{
//		SLEEP_THREAD(time_delay);
//
//		printf("sensor_cast_process: main loop\n");
//		struct runicast_message msg;
//
//		printf("sending runicast to %d.%d\n", daddy_addr->u8[0], daddy_addr->u8[1]);
//
//		msg.type = RUNICAST_TYPE_TEMP;
//		msg.data = random_rand() % 10;
//
//		packetbuf_copyfrom(&msg, sizeof(msg));
//		runicast_send(&runicast, &daddy_addr, MAX_RETRANSMISSIONS);
//	}


	PROCESS_END();
}

