/*
 * common.h
 *
 *  Created on: 20 Oct 2015
 *      Author: enikolov
 */

#ifndef COMMON_H_
#define COMMON_H_

#define SLEEP_THREAD(time) \
	{ \
		static struct etimer SLEEP_TIMER_IN_SLEEP_MACRO; \
		etimer_set(&SLEEP_TIMER_IN_SLEEP_MACRO,  (time * CLOCK_SECOND) / 1000);  \
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&SLEEP_TIMER_IN_SLEEP_MACRO));\
	};


#define NEW_TIMER_RECEIVED_EVENT        0x01

/*
 * Then we define the values needed for runicast to be reliable ;), wich are
 * the amount of allowable rettransmissions before accepting failure, and
 * the amount of message id's in the history list, wich should be enough
 * to contain at least one message from each parent node (so, one :D)
 */

#define MAX_RETRANSMISSIONS 4
#define NUM_HISTORY_ENTRIES 2
#define MAX_NEIGHBORS 16


/*
 * create runicast_msg structure
 * create broadcast_msg structure (unneccesary?)
 * create a sender_history list to detect duplicate messages
 */
struct runicast_message
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



static void timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions);
static void recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno);
static void sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions);

static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     	 	 	 	 	 	 	 sent_runicast,
															 timedout_runicast};

static void recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from);

static const struct broadcast_callbacks broadcast_callbacks = {recv_broadcast};


linkaddr_t *daddy_addr;
int schedule_set = 0;

uint16_t time_delay = -1;


/*---------------------------------------------------------------------------*/
static struct runicast_conn runicast;
static struct broadcast_conn broadcast;


#endif /* COMMON_H_ */

