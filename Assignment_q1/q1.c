#include "contiki.h"
#include "net/rime/rime.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"

#include <stdio.h>

#define CHANNEL 135


struct neighbor {
  struct neighbor *next;
  linkaddr_t addr;
  uint16_t last_rssi, last_lqi;
  uint8_t last_seqno;
};

struct broadcast_message {
  uint8_t seqno;
};

#define MAX_NEIGHBORS 16
LIST(neighbors_list);
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);
struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS(q1, "q1");
PROCESS(broadcast_process, "Broadcast_process");
AUTOSTART_PROCESSES(&q1,&broadcast_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  struct neighbor *n;
  struct broadcast_message *m;
  /* The packetbuf_dataptr() returns a pointer to the first data byte
     in the received packet. */
  m = packetbuf_dataptr();

  /* Check if we already know this neighbor. */
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {

    /* We break out of the loop if the address of the neighbor matches
       the address of the neighbor from which we received this
       broadcast message. */
    if(linkaddr_cmp(&n->addr, from)) {
      break;
    }
  }

  /* If n is NULL, this neighbor was not found in our list, and we
     allocate a new struct neighbor from the neighbors_memb memory
     pool. */
  if(n == NULL) {
    n = memb_alloc(&neighbors_memb);

    /* If we could not allocate a new neighbor entry, we give up. We
       could have reused an old neighbor entry, but we do not do this
       for now. */
    if(n == NULL) {
      return;
    }

    /* Initialize the fields. */
    linkaddr_copy(&n->addr, from);
    n->last_seqno = m->seqno - 1;

    /* Place the neighbor on the neighbor list. */
    list_add(neighbors_list, n);
  }

  /* We can now fill in the fields in our neighbor entry. */
  n->last_rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
  n->last_lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);

  /* Remember last seqno we heard. */
  n->last_seqno = m->seqno;

  /* Print out a message. */
  printf("broadcast message received from %d.%d with seqno %d, RSSI %u, LQI %u, avg seqno gap %d.%02d\n",
         from->u8[0], from->u8[1],
         m->seqno,
         packetbuf_attr(PACKETBUF_ATTR_RSSI),
         packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY));
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
/*---------------------------------------------------------------------------*/
/*
 * This function is called at the final recepient of the message.
 */
static void
recv(struct multihop_conn *c, const linkaddr_t *sender,
     const linkaddr_t *prevhop,
     uint8_t hops)
{
  printf("multihop message received '%s' after %d hops from %d.%d\n", (char *)packetbuf_dataptr(),hops,prevhop->u8[0],prevhop->u8[1]);
}
/*
 * This function is called to forward a packet. The function picks a
 * random neighbor from the neighbor list and returns its address. The
 * multihop layer sends the packet to this address. If no neighbor is
 * found, the function returns NULL to signal to the multihop layer
 * that the packet should be dropped.
 */
static linkaddr_t *
forward(struct multihop_conn *c,
	const linkaddr_t *originator, const linkaddr_t *dest,
	const linkaddr_t *prevhop, uint8_t hops)
{
  int num=0, i=0;
  struct neighbor *t=NULL;
  struct neighbor *n=NULL;
  //printf("Destination: %d.%d  ",dest->u8[0],dest->u8[1]);
  if(list_length(neighbors_list) > 0) {
    num=-1000;
    if(list_length(neighbors_list)==1){
	n=list_head(neighbors_list);
	}
    for(t = list_head(neighbors_list); t != NULL; t = t->next) {
      i=t->last_rssi;
      if(i==num){
	continue;
	}
      if((i>num)&&(t->addr.u8[0]!=prevhop->u8[0])){
		n=t;
		num=i;
		}
      uint8_t z = t->addr.u8[0]-dest->u8[0];
      if(z<1){
		n=t;
		break;
	}
    }
    if(n != NULL) {
      printf("%d.%d: Forwarding packet to %d.%d (%d in list), hops %d\n",
	     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
	     n->addr.u8[0], n->addr.u8[1], num,
	     packetbuf_attr(PACKETBUF_ATTR_HOPS));
      return &n->addr;
    }
  }
  printf("%d.%d: did not find a neighbor to foward to\n",
	 linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
  return NULL;
}
static const struct multihop_callbacks multihop_call = {recv, forward};
static struct multihop_conn multihop;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(q1, ev, data)
{
  PROCESS_EXITHANDLER(multihop_close(&multihop);)
    
  PROCESS_BEGIN();
  multihop_open(&multihop, CHANNEL, &multihop_call);
  /* Activate the button sensor. We use the button to drive traffic -
     when the button is pressed, a packet is sent. */
  SENSORS_ACTIVATE(button_sensor);

  /* Loop forever, send a packet when the button is pressed. */
  while(1) {
    linkaddr_t to;

    /* Wait until we get a sensor event with the button sensor as data. */
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
			     data == &button_sensor);

    /* Copy the "Hello" to the packet buffer. */
    packetbuf_copyfrom("Hello", 6);

    /* Set the Rime address of the final receiver of the packet to
       1.0. This is a value that happens to work nicely in a Cooja
       simulation (because the default simulation setup creates one
       node with address 1.0). */
    to.u8[0] = 1;
    to.u8[1] = 0;

    /* Send the packet. */
    multihop_send(&multihop, &to);

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
 
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  /* Send a broadcast every 16-32 seconds */
    static struct etimer et;
  static uint8_t seqno;
  struct broadcast_message msg;
  while(1) {
    etimer_set(&et, (CLOCK_SECOND * 16 + random_rand() % (CLOCK_SECOND * 16)) );
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    msg.seqno = seqno;
    packetbuf_copyfrom(&msg, sizeof(struct broadcast_message));
    broadcast_send(&broadcast);
    seqno++;
}
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
