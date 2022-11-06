#include "contiki.h"
#include "net/rime/rime.h"
#include "sys/node-id.h"
#include "dev/button-sensor.h"
#include "sys/clock.h"
#include "dev/leds.h"
#include "sys/autostart.h"
#include <stdio.h>
#define MAX_DATA 5
struct head_buffer{
struct head_buffer *next;
linkaddr_t addr;
char data[127];
};

LIST(cluster_list);
MEMB(cluster_memory, struct head_buffer, MAX_DATA);
/*---------------------------------------------------------------------------*/
PROCESS(cluster_head, "Cluster Head");
PROCESS(cluster_member1, "Cluster Member 1");
PROCESS(cluster_member2, "Cluster Member 2");
AUTOSTART_PROCESSES(&cluster_head, &cluster_member1, &cluster_member2);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  struct head_buffer* temp;  
  char* abc;
  int i=0;
  if((node_id==2)||(node_id==3)){
	temp=memb_alloc(&cluster_memory);
	if(temp != NULL) {
	    linkaddr_copy(&temp->addr, from);
	    abc=(char*)packetbuf_dataptr();
            for(i=0;i<=126;i++)
		{
		temp->data[i] = abc[i];
		}
	    list_add(cluster_list, temp);
	  }
	printf("unicast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);
}
  else if(node_id==1){
	printf("Sink saving data from cluster head%d.%d\n",
	 from->u8[0], from->u8[1]);
}	
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc1;
static struct unicast_conn uc2;
static struct unicast_conn uc3;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cluster_member1, ev, data)
{
  static struct etimer et;
  PROCESS_EXITHANDLER(unicast_close(&uc1));
  PROCESS_BEGIN();
  unicast_open(&uc1, 146, &unicast_callbacks);
  if((node_id>=4)||(node_id<=9)){
  while(1) {
    etimer_set(&et, (RTIMER_ARCH_SECOND / 10000));
    linkaddr_t addr;
    packetbuf_copyfrom("APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL apple", 127);
//127 bytes to be sent
    addr.u8[0] = 2;
    addr.u8[1] = 0;
    if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
      unicast_send(&uc1, &addr);
    }
  }
}
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cluster_head, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc3));
  PROCESS_BEGIN();
  struct head_buffer* temp;
  struct head_buffer* next;
  unicast_open(&uc3, 145, &unicast_callbacks);
  if((node_id==3)||(node_id==2)){
  //SET TIMER AND READ DATA FROM SAVED BUFFER
  while(1) {
//SET TIMER AS PER REQUIREMENT
    static struct etimer et;
    linkaddr_t addr;
    etimer_set(&et, 60*RTIMER_ARCH_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    addr.u8[0] = 1;
    addr.u8[1] = 0;
    temp=list_head(cluster_list);
    while(temp!=NULL){
    packetbuf_copyfrom(temp->data,127);
    next=temp->next;
    if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
      unicast_send(&uc1, &addr);
    list_remove(cluster_list, temp);
    memb_free(&cluster_memory, temp);
    temp=next;
    }
    }
  }
}
 PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cluster_member2, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc2));   
  PROCESS_BEGIN();
  uint16_t temp=100;
  static struct etimer et;
  unicast_open(&uc2, 147, &unicast_callbacks);
  if((node_id>=10)||(node_id<=15)){
  temp=temp*(node_id-10);
  etimer_set(&et, (temp)*(RTIMER_ARCH_SECOND / 10000));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  while(1) {
    linkaddr_t addr;
    packetbuf_copyfrom("APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL APPLE BALL apple", 127);
//127 bytes to be sent
    addr.u8[0] = 3;
    addr.u8[1] = 0;
    if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
      unicast_send(&uc2, &addr);
    }
    etimer_set(&et, (RTIMER_ARCH_SECOND / 10000));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }
}
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

