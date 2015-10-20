/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "dev/button-sensor.h" 
#include "dev/light-sensor.h" 
#include "dev/leds.h"
#include "mycommon.h"

#include <stdio.h> /* For printf() */
static int counter = 0;
static int long long  t = 0;
static int x = 0;

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process2, "Hello world process2");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process2, ev, data)
{

  static struct etimer et;
  PROCESS_BEGIN();

  printf("Process2\n");


  SLEEP_THREAD(5000);


  printf("Process2 ended\n");
  x = 1;

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process, &hello_world_process2);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  
  SENSORS_ACTIVATE(button_sensor); 
  SENSORS_ACTIVATE(light_sensor); // activate sensor  
  printf("Hello, world\n");

  SLEEP_THREAD(2000);

  printf("other timer\n");

  PROCESS_WAIT_UNTIL(x == 1);
  printf("Bye, world\n");

  printf("%lld\n", t = clock_time());
  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor); // wait for button press event

	printf("time: %lld\n", ((clock_time() - t)*1000)/CLOCK_SECOND);
	printf("time: %d\n", CLOCK_SECOND);
	t = clock_time();

    counter++;
    leds_toggle(LEDS_ALL); // toggle all leds
    printf("Light: \%u\n",light_sensor.value(0)); //print out current sensor value
    printf("Button counter: \%d\n",counter);//print out button counter
  }
/*
  PROCESS_WAIT_EVENT();
  if(ev == sensor_event && data == &button_sensor)
  {

  }
*/
  

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
