#include "contiki.h"
#include "net/rime/rime.h"
#include "sys/node-id.h"
#include "dev/button-sensor.h"
#include "sys/clock.h"
#include "dev/leds.h"
#include "sys/autostart.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(cluster_member2, "Cluster Member 2");
AUTOSTART_PROCESSES(&cluster_member2);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
printf("Yay, data received from him");
}	

static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc2;

PROCESS_THREAD(cluster_member2, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc2));   
  PROCESS_BEGIN();
  uint16_t temp=1;
  static struct etimer et;
  unicast_open(&uc2, 147, &unicast_callbacks);
  temp=temp*(node_id-10);
  etimer_set(&et, (temp)*(RTIMER_ARCH_SECOND/10000));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  while(1) {
    linkaddr_t addr;
    /*packetbuf_copyfrom("APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL  APPLE BALL APPLE BALL APPLE BALL APPLE BALL", 127);*/
//127 bytes to be sent
    packetbuf_copyfrom("aforappleaforappleaforappleaforappleaforappleaforappleaforappleaforappleaforappleaforappleaforappleaforapple",108);
    addr.u8[0] = 3;
    addr.u8[1] = 0;
    unicast_send(&uc2, &addr);
    //printf("sending unicast to %d.%d\n", addr.u8[0], addr.u8[1]);
    etimer_set(&et, (RTIMER_ARCH_SECOND /2000));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

