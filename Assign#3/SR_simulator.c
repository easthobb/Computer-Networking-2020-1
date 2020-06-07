
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
   
   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
                        /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
    int seqnum;  // seq # of sending PKT
    int acknum;   // ack # of sending PKT
    int checksum; // checksum
    char payload[20]; 
};

void starttimer(int AorB, float increment); // AorB , increment?
void stoptimer(int AorB); // stoptimer
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define BUFSIZE 64

struct Sender { // 윈도우 캔낫 무브 포워드 해야된댕
    int base; //베이스는 무엇일가 윈도우 처음
    int nextseq; //다음으로 전송할 seq , usable but not yet sent
    int window_size; //이거 입력 받아야한다.
    float estimated_rtt; //이건 왜 들어가있지 아 
    int buffer_next;
    struct pkt packet_buffer[BUFSIZE];
} A;
struct Receiver {
    int expect_seq; //예상하는 시퀀스
    int window_size; // 추가변수 
    struct pkt packet_to_send; // 패킷 버퍼?
    /* hobbs adding */
    int base;
    int nextseq;
    int buffer_next;
    struct pkt packet_buffer[BUFSIZE];
} B;

void send_window(void) // PKT 쏘는 함수 이부분은 고칠게 없다,
{ //패킷 보낸다아아아
    while (A.nextseq < A.buffer_next && A.nextseq < A.base + A.window_size) 
    {
       /* 미리 정의된 packet_buffer에 들어있던 패킷의 포인터 변수를 꺼내와서 layer3로 전송한다.*/
       /* 전송하는 패킷의 seq#, payload 영역을 출력*/ 
        struct pkt *packet = &A.packet_buffer[A.nextseq % BUFSIZE]; //연결리스트 형태로 메시받은 메시지 넣음
        printf("  send_window: send packet (seq=%d): %s\n", packet->seqnum, packet->payload);
        tolayer3(0, *packet); // 이걸로 보내내 
        if (A.base == A.nextseq) 
            starttimer(0, A.estimated_rtt); // 왜 RTT랑 함께 이걸 보내는걸까? 
        ++A.nextseq;
    }
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) 
{
   /* layer 5에서 호출되며 상위의 메시지 출력 및 버퍼에 넣음 */
    if (A.buffer_next - A.base >= BUFSIZE) //cannot move forward
    {
        printf("  A_output: buffer full. drop the message: %s\n", message.data);
        return;
    }
    printf("  A_output: bufferred packet (seq=%d): %s\n", A.buffer_next, message.data);
    struct pkt *packet = &A.packet_buffer[A.buffer_next % BUFSIZE];
    packet->seqnum = A.buffer_next;
    packet->acknum = 0;
    memmove(packet->payload, message.data, 20);//메모리 복사!!
    ++A.buffer_next;
    send_window();//준비되시면 쏘세요~!
}

/* need be completed only for extra credit */
void B_output(struct msg message)
{
    printf("  B_output: uni-directional. ignore.\n");
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) // sender 동작의 핵심.
{
   /*ack 3가지 경우의 수 구현 요망*/
   /*ack 번호가 base 이전, ack의 번호가 base와 base+windowsize(<=buffer)이전, base+window size 바깥*/
   //1.ack 번호 base 이전일시 -> 무적권 디스카드
   //2.ack 적절한 범위 일때, 
   // 안차있을 경우 ->ack 필드 인정 채워주기, 차있을 경우 -> 교수님 드뢉더빗
   // 안차있는데 심지어 base다 ? 그럼 base 앞으로 forward 
   //3. 이런 ack가 올리가 없지 
   // 추가적으로 타이머 고려 필요
   
    
   if (packet.acknum < A.base) //if in duplicated ack situation
   {
      printf("  A_input: got (ack=%d). drop.\n", packet.acknum); 
      return; // ack 번호 출력 및 drop 후 함수 종료

   }//case I clearing

   /* 받은 ack가 적절한 범위 내의 ack 일때*/
   else if(packet.acknum > A.base && packet.acknum <A.base+A.window_size)// 이부분 조건문 개선필요
   {  
      //보내기로 한 pkt 집합 속에서 현재 받은 ack 번호에 해당하는 pkt 꺼내옴
      struct pkt *checkedPacket = &A.packet_buffer[packet.seqnum-1];

      //
      if(packet.acknum == A.base)
      {
         printf("  A_input: got ACK (ack=%d)\n", packet.acknum);
         A.base = packet.acknum + 1;
         checkedPacket->acknum = packet.acknum;

         for(int i=checkedPacket->seqnum;i<A.base+A.window_size;i++)
         {
            //센딩윈도우만큼 ack 검사해줄지말지 고민쓰?
            if(&A.packet_buffer[i].acknum==&A.packet_buffer[i].seqnum)
               A.base++;
            else
               return; 
         }
      }
      else//차있거나 안차있거나
      {
         if(checkedPacket->acknum==0) //안차있을경우
         {
            printf("  A_input: got ACK (ack=%d)\n", packet.acknum);
            checkedPacket->acknum == packet.seqnum;
            return;
         }
         else//차있을경우
         {
            printf("  A_input: got (ack=%d). drop.\n", packet.acknum);
            return;      
         }
      }

   }
    
    printf("  A_input: got ACK (ack=%d)\n", packet.acknum);
    A.base = packet.acknum + 1;

    if (A.base == A.nextseq)
    {
        stoptimer(0);
        printf("  A_input: stop timer\n");
        send_window();
    } 
    else 
    {
        starttimer(0, A.estimated_rtt); // starting
        printf("  A_input: timer + %f\n", A.estimated_rtt);
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void){ 
/*A의 timer interrupt 발생 시 호출되는 함수*/
// 해주는 일 : 선택적 재전송 
// base에 해당하는 PKT를 다시 전송해주면 된다. 그리고 다시 타이머 스타팅.
   struct pkt *packet = &A.packet_buffer[A.base % BUFSIZE]; //인덱스 오버 방지차원에서 해놓은거구만
   printf("  A_timerinterrupt: resend packet (seq=%d): %s\n", packet->seqnum, packet->payload);
   tolayer3(0, *packet);
   starttimer(0, A.estimated_rtt);
   printf("  A_timerinterrupt: timer + %f\n", A.estimated_rtt);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{ // 변수 초기화 하는 곳이구만 다 ~ 아는 놈들이구먼
    A.base = 1;
    A.nextseq = 1;
    A.window_size = 8;
    A.estimated_rtt = 15;
    A.buffer_next = 1;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) //리시버단의 핵심코드 
{/*SR에서 receiver가 해야하는 일 크게  : 1-리시버 윈도우에 해당하는 PKT 올경우 ACK, 2-받은 메시지 출력*/
/* PKT 송신시*/
// 1.receiver window 범위 내에 있는 ack가 도착했을 경우 base+window<buffer
// 1-1 base에 해당하는 PKT 일시 ack 보내주고, base 전진 -> 전진했는데 이미 다음 순서 차있다? 그럼 또 전진
// 1-2 base에 해당하지 않을경우 버퍼에 채워주고 ack 쏴준다
// 2.receiver window 범위 바깥에 있는 ack가 도착했을 경우
// 2-1 base 이전 순서의 PKT의 경우 discard 메시지와 함께 무시
// 2-2 base+windowsize의 PKT 의 경우 어떻게 처리하는지는 설계의 문제이나 안정성을 위해 discard

/*이미 받은 PKT일 경우 discard*/
   if(B.packet_buffer[packet.seqnum-1].seqnum == packet.seqnum)
   {
      printf("  B_input: the PKT is already received. discard PKT: %d\n", packet.seqnum);
      return;
   }
   else
   {   // 일단 포인터 먹히는지 봐야할듯 : packet buffer에 쑤셔넣기
   
   //printf("  B_input: recv packet (seq=%d): %s\n", packet.seqnum, packet.payload);
      if(packet.seqnum == B.base)
      {
         /*pck ack 전송&payload upperlayer 전송&출력*base전진*/
         printf("B_input: recv packet (seq=%d): %s \n",packet.seqnum, packet.payload);
         tolayer5(1, packet.payload);//pkt seq 출력
         packet.acknum = B.base; //PKT에 ack 담아줌
         tolayer3(1,packet);//ack sending
         B.packet_buffer[ packet.seqnum % BUFSIZE] = packet;//패킷 어레이에 넣어주기
         B.base = packet.seqnum + 1; //base 이동
         if(B.packet_buffer[B.base-1].acknum!=0)//이미 다음 slot에 packet이 차있다면
         {
            B.base ++; //base 전진시킴.
         }
         return;
      }
      else
      {  /*출력&payload upperlayer 전송&pck ack 전송*/
         printf("(outoforeder)B_input: recv packet(buffered) (seq=%d): %s\n", packet.seqnum, packet.payload);
         tolayer5(1, packet.payload);//pkt seq 출력
         packet.acknum = B.base; //PKT에 ack 담아줌
         tolayer3(1,packet);//ack sending
         B.packet_buffer[ packet.seqnum % BUFSIZE] = packet;//패킷 어레이에 넣어주기
         return;
      }
   }
}

/* called when B's timer goes off */
void B_timerinterrupt(void) { // 이 부분은 채점기준에 없다.
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    B.expect_seq = 1;
    B.packet_to_send.seqnum = -1;
    B.packet_to_send.acknum = 0;
    memset(B.packet_to_send.payload, 0, 20);
    B.packet_to_send.checksum = get_checksum(&B.packet_to_send);
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the tranmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)
THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

/* 구현에 필요한 변수 선언부 prob - corrupt prob 은 0으로 가정합니다. */
int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped - not used  */
float timeout;     /* the duration of the time for which the timer is started and till it finished */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init(int argc, char **argv);
void generate_next_arrival(void);
void insertevent(struct event *p);

int main(int argc, char **argv) {
    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;

    init(argc, argv);
    A_init();
    B_init();

    while (1) {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2) {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER5) {
            if (nsim < nsimmax) {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 20; i++)
                    msg2give.data[i] = 97 + j;
                msg2give.data[19] = 0;
                if (TRACE > 2) {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        } else if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give); /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        } else if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        } else {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(
            " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
            time, nsim);
}

void init(int argc, char **argv) /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    if (argc != 6) {
        printf("usage: %s  num_sim  prob_loss  timeout  interval  debug_level\n", argv[0]);
        exit(1);
    }
   /* 이부분에 number of message, loss prob, Window size, */
    nsimmax = atoi(argv[1]); // 1.number of message to simulate
    lossprob = atof(argv[2]); // 2.loss probabiltiy
    timeout = atof(argv[3]); //3. the duration of the time from which the timer is stareted and till it's finished
    lambda = atof(argv[4]);//AVG time between message from sender's layer5
    TRACE = atoi(argv[5]);
    corruptprob = 0;
    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("the number of messages to simulate: %d\n", nsimmax)m;
    printf("packet loss probability: %f\n", lossprob);
    printf("packet corruption probability: %f\n", corruptprob);
    printf("average time between messages from sender's layer5: %f\n", lambda);
    printf("TRACE: %d\n", TRACE);


    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75) {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void) {
    double mmm = RAND_MAX;
    float x;                 /* individual students may need to change mmm */
    x = rand() / mmm;        /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void) {
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
                                                             /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p) {
    struct event *q, *qold;

    if (TRACE > 2) {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;      /* q points to header of list in which p struct inserted */
    if (q == NULL) { /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    } else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) { /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        } else if (q == evlist) { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        } else { /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void) {
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
                     q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */) {
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;          /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist) { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            } else { /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to stop timer */, float increment) {
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB /* A or B is trying to stop timer */, struct pkt packet) {
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob) {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2) {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
                     mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
                                      /* finally, compute the arrival time of packet at the other end.
                                         medium can not reorder, so make sure packet arrives between 1 and 10
                                         time units after the latest arrival time of packets
                                         currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob) {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20]) {
    int i;
    if (TRACE > 2) {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}

/*
$ gcc prog2_gbn.c
$ ./a.out
usage: ./a.out  num_sim  prob_loss  prob_corrupt  interval  debug_level
$ ./a.out 20 0.2 0.2 10 2 | less
$ ./a.out 20 0.2 0.2 10 2 | grep 'recv packet'
  B_input: recv packet (seq=1): aaaaaaaaaaaaaaaaaaa
  B_input: recv packet (seq=2): bbbbbbbbbbbbbbbbbbb
  B_input: recv packet (seq=3): ccccccccccccccccccc
  B_input: recv packet (seq=4): ddddddddddddddddddd
  B_input: recv packet (seq=5): eeeeeeeeeeeeeeeeeee
  B_input: recv packet (seq=6): fffffffffffffffffff
  B_input: recv packet (seq=7): ggggggggggggggggggg
  B_input: recv packet (seq=8): hhhhhhhhhhhhhhhhhhh
  B_input: recv packet (seq=9): iiiiiiiiiiiiiiiiiii
  B_input: recv packet (seq=10): jjjjjjjjjjjjjjjjjjj
  B_input: recv packet (seq=11): kkkkkkkkkkkkkkkkkkk
  B_input: recv packet (seq=12): lllllllllllllllllll
  B_input: recv packet (seq=13): mmmmmmmmmmmmmmmmmmm
  B_input: recv packet (seq=14): nnnnnnnnnnnnnnnnnnn
  B_input: recv packet (seq=15): ooooooooooooooooooo
  B_input: recv packet (seq=16): ppppppppppppppppppp
  B_input: recv packet (seq=17): qqqqqqqqqqqqqqqqqqq
  B_input: recv packet (seq=18): rrrrrrrrrrrrrrrrrrr
  B_input: recv packet (seq=19): sssssssssssssssssss
  B_input: recv packet (seq=20): ttttttttttttttttttt
*/