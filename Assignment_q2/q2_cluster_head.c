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
PROCESS(cluster_head_to_sink, "Cluster Head to Sink");
AUTOSTART_PROCESSES(&cluster_head,&cluster_head_to_sink);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  struct head_buffer* temp;  
  char* abc;
  int i=0;
  printf("unicast message received from %d.%d\n",
 from->u8[0], from->u8[1]);
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
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc3;
static struct unicast_conn uc1;
PROCESS_THREAD(cluster_head, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc3));
  PROCESS_BEGIN();
  struct head_buffer* temp;
  struct head_buffer* next;
  int flag=0;
  if(node_id==2){
	flag=146;
	}
  else if(node_id==3){
	flag=147;
	}	
  unicast_open(&uc3, flag, &unicast_callbacks);
  //SET TIMER AND READ DATA FROM SAVED BUFFER
  while(1) {
//SET TIMER AS PER REQUIREMENT
    static struct etimer et;
    linkaddr_t addr;
    etimer_set(&et, 40*RTIMER_ARCH_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    addr.u8[0] = 15;
    addr.u8[1] = 0;
    packetbuf_copyfrom("Let's continue",1);
}
 PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cluster_head_to_sink,ev,data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc1));
  PROCESS_BEGIN();
  struct head_buffer* temp;
  struct head_buffer* next;
  unicast_open(&uc1, 145, &unicast_callbacks);
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
    packetbuf_copyfrom(temp->data,108);
    next=temp->next;
    unicast_send(&uc1, &addr);
    list_remove(cluster_list, temp);
    memb_free(&cluster_memory, temp);
    temp=next;
    }
}
 PROCESS_END();
}
