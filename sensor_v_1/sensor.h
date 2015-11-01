/*
 * sensor.h
 *
 *  Created on: 1 Nov 2015
 *      Author: enikolov
 */

#ifndef SENSOR_H_
#define SENSOR_H_

static void
recv_runicast_schedule(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno);

static void
sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions);

static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions);

static void
recv_broadcast_actuator_adv(struct broadcast_conn *c, const linkaddr_t *from);



static const struct broadcast_callbacks actuator_adv_broadcast_callbacks = {recv_broadcast_actuator_adv};

static const struct runicast_callbacks runicast_schedule_callbacks = {recv_runicast_schedule,
							     	 	 	 	 	 	 	 sent_runicast,
															 timedout_runicast};



#endif /* SENSOR_H_ */
