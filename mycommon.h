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
#define MAX_NEIGHBORS 12
#define TIME_INTERVAL 60

/* These two defines are used for computing the moving average for the
   broadcast sequence number gaps. */
#define SEQNO_EWMA_UNITY 0x100
#define SEQNO_EWMA_ALPHA 0x040


/*
 * create runicast_msg structure
 * create broadcast_msg structure (unneccesary?)
 * create a sender_history list to detect duplicate messages
 */
struct runicast_message
{
	uint8_t type;
	int16_t data;
	uint8_t seqno;
};


/* This is the structure of broadcast messages. */
struct broadcast_message {
  uint8_t seqno;
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


int schedule_set = 0;

clock_time_t time_delay = -1;


/*---------------------------------------------------------------------------*/
static struct runicast_conn runicast;
static struct broadcast_conn broadcast;


/* This structure holds information about neighbors. */
struct neighbor {
  /* The ->next pointer is needed since we are placing these on a
     Contiki list. */
  struct neighbor *next;

  /* The ->addr field holds the Rime address of the neighbor. */
  linkaddr_t addr;

  /* The ->last_rssi and ->last_lqi fields hold the Received Signal
     Strength Indicator (RSSI) and CC2420 Link Quality Indicator (LQI)
     values that are received for the incoming broadcast packets. */
  uint16_t last_rssi, last_lqi;

  /* Each broadcast packet contains a sequence number (seqno). The
     ->last_seqno field holds the last sequenuce number we saw from
     this neighbor. */
  uint8_t last_seqno;

  /* The ->avg_gap contains the average seqno gap that we have seen
     from this neighbor. */
  uint32_t avg_seqno_gap;

};



#endif /* COMMON_H_ */

