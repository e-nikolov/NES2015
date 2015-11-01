/*
 * actuator.h
 *
 *  Created on: 1 Nov 2015
 *      Author: enikolov
 */

#ifndef ACTUATOR_H_
#define ACTUATOR_H_


static void
sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions);

static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions);


static void
recv_runicast_data(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno);


static const struct runicast_callbacks runicast_data_callbacks = {recv_runicast_data,
							     	 	 	 	 	 	 	 sent_runicast,
															 timedout_runicast};




#endif /* ACTUATOR_H_ */
