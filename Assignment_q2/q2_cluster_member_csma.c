#include "contiki.h"
#include "net/rime/rime.h"
#include "sys/node-id.h"
#include "dev/button-sensor.h"
#include "sys/clock.h"
#include "dev/leds.h"
#include "sys/autostart.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(cluster_member1, "Cluster Member 1");
AUTOSTART_PROCESSES(&cluster_member1);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
printf("Yay, data received");
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc1;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cluster_member1, ev, data)
{
  static struct etimer et;
  PROCESS_EXITHANDLER(unicast_close(&uc1));
  PROCESS_BEGIN();
  unicast_open(&uc1, 146, &unicast_callbacks);
  while(1) {
    etimer_set(&et, (RTIMER_ARCH_SECOND/10000));
    linkaddr_t addr;
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    packetbuf_copyfrom("aforappleaforappleaforappleaforappleaforappleaforappleaforappleaforappleaforappleaforappleaforappleaforapple",108);
//127 bytes does not work; nothing beyond 108 bytes works for the given time spilt (0.1ms)
    addr.u8[0] = 2;
    addr.u8[1] = 0;
    unicast_send(&uc1, &addr);
    //printf("sending unicast to %d.%d\n", addr.u8[0], addr.u8[1]);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

