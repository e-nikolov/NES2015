/*
 * common.h
 *
 *  Created on: 20 Oct 2015
 *      Author: enikolov
 */

#ifndef COMMON_H_
#define COMMON_H_
#endif /* COMMON_H_ */

#define SLEEP_THREAD(time) \
	{ \
		static struct etimer SLEEP_TIMER_IN_SLEEP_MACRO; \
		etimer_set(&SLEEP_TIMER_IN_SLEEP_MACRO,  (time * CLOCK_SECOND) / 1000);  \
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&SLEEP_TIMER_IN_SLEEP_MACRO));\
	};

