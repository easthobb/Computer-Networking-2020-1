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
    int checksum; // 주어진 문제에서 corrupt rate을 0으로 가정하기에 사용하지 않습니다.
    char payload[20]; //packet의 DATA 영역 5layer의 msg에 해당합니다. 
};

/* callable Routine의 함수 선언부, 문제에서 제공한 4가지 함수를 이용하여 SR을 구현합니다. */
void starttimer(int AorB, float increment); // timer를 시작하는 함수, A - 0 ,B - 1로 입력받으며 increment 만큼의 시간이 경과시 Engine에 의해 timeout event 수행 
void stoptimer(int AorB); // timer를 종료하는 함수, A - 0 ,B - 1로 입력받으며 실행 즉시 starttimer로 실행되고 있던 timer를 중지시켜 timeout event 방지
void tolayer3(int AorB, struct pkt packet); //A 또는 B에서 packet을 layer 3로 내려서 전송이 이루어지게 하는 함수/
void tolayer5(int AorB, char datasent[20]); //A 또는 B에서 packet을 layer 5로 올려줍니다 */

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
/* 구현부분 - 작성자 : 동국대학교 컴퓨터공학전공 2015112167 김동호 */

#define BUFSIZE 64 // MAX

/* 구조체 선언 - A-Sender와 B-Recevier */
struct Sender
{
    int base; // sending window의 처음을 가르키는 인덱스 변수
    int nextseq; //다음으로 전송할 seq , usable but not yet sent
    int window_size; //사용자가 입력하고 지정하는 sending window size
    float timeout; //사용자가 입력하고 지정하는 timeout
    int buffer_next; // sending packet_buffer의 뒷 부분을 지정하는 index 변수 
    struct pkt packet_buffer[BUFSIZE]; // packet buffer 변수
    /* Sender 가 만족시켜야 하는 내용*/
    /* 1- Sender window Cannot Move Forward 구현 */
    /* 2- Timeout시 Unacked packet with the lowest seq num 재전송 구현*/
   
} A;
struct Receiver
{
    int expect_seq; // layer3에서 받을 것으로 예상되는 PKT의 seq 인덱스 지정변수
    int window_size; // 사용자가 입력하고 지정하는 receiving window size
    int base; // sending window의 처음을 가르키는 인덱스 변수
    float timeout;//사용자가 입력하고 지정하는 timeout
    struct pkt packet_buffer[BUFSIZE];// packet_buffer의 뒷 부분을 지정하는 index 변수 
    /* Receiver 가 만족시켜야 하는 내용 */
    /* 1- Duplicated PKT 를 Discard 하는 기능 구현 */
    /* 2- Out of Order의 packet을 buffer에 보관 구현 */
    /* 3- Individual ack 구현 여부 */
} B; 



/* 사용자 구현 함수 부분 */ 
/*Sender   :  send_window, A_init,A_input, A_output, A_timerintterupt */
/*Receiver :  B_init, B_input, B_timerintterupt */ 

/* sender에서 PKT을 layer 3로 보내주는 함수 */
void send_window(void)
{   /* sending window 에 해당하는 PKT들을 순회하면서 전송하는 while 문 */
    while (A.buffer_next > A.nextseq && A.nextseq < A.base + A.window_size)
    {  
       /* 미리 정의된 packet_buffer에 들어있던 패킷의 포인터 변수를 꺼내와서 layer3로 전송한다.*/
       /* 전송하는 패킷의 seq#, payload 영역을 출력*/ 
       if(A.packet_buffer[A.nextseq % BUFSIZE].acknum==0) //중복전송 방지, 이미 ack를 받은 PKT일 경우 안보냅니다.
       {
            struct pkt *packet = &A.packet_buffer[A.nextseq % BUFSIZE]; //연결리스트 형태로 메시받은 메시지 넣습니다.인덱스 초과 방지로 % BUFSIZE
            printf("  send_window: send packet (seq=%d): %s\n", packet->seqnum, packet->payload);
            tolayer3(0, *packet); // layer 3 로 Packet을 넣습니다.  
            
            if (A.base == A.nextseq) // 현재 sending window에서 base라면 timer 시작 
            {starttimer(0, A.timeout);} // timeout만큼의 시간 할당
            ++A.nextseq;
        }
        else//PKT 이미 보내고, ack를 받은 패킷을 경우 passs
        {   //++A.nextseq;
             //if (A.base == A.nextseq) // 현재 sending window에서 base라면 timer 시작 
            //{starttimer(0, A.timeout);} // timeout만큼의 시간 할당
             continue;
        }
    }
}

/* called from layer 5, passed the data to be sent to other side */
/* layer 5에서 MSG를 받고, 패킷 버퍼에 넣고 그 버퍼에 받은 PKT를 layer3 쏘는 함수 자동 호출,*/
void A_output(struct msg message) 
{
   /* layer 5에서 호출되며 상위의 메시지 출력 및 버퍼에 넣음 */
    if (A.buffer_next - A.base >= BUFSIZE) //버퍼보다 큰 사이즈의 Number of simulate를 시도할 경우
    {
        printf("  A: buffer full. drop the message: %s\n", message.data);
        return;
    }

    printf("  A: got from layer5 packet (seq=%d): %s, base:%d\n", A.buffer_next, message.data,A.base);
    struct pkt *packet = &A.packet_buffer[A.buffer_next % BUFSIZE]; // 버퍼에서 받은 패킷의 값을 초기화 하기위해 가져옵니다.
    packet->seqnum = A.buffer_next; //seq number를 set
    packet->acknum = 0; //ack number를 set
    memmove(packet->payload, message.data, 20);//메모리 복사!!
    ++A.buffer_next;//버퍼 인덱스 증가.
    send_window();//sending window 범위 패킷 전송
}

/* need be completed only for extra credit */
/* 본 문제에서는 B가 sender로 작동하지 않으므로 구현하지 않습니다 */
void B_output(struct msg message)
{
    printf("  B: uni-directional. ignore.\n");
}

/* called from layer 3, when a packet arrives for layer 4 */
/* A(sender)가 layer 3에서 PKT(ack)를 받을경우 호출되는 함수, PKT의 ack를 확인해 버릴지 받을지 결정 */
void A_input(struct pkt packet) 
{
   /*ack 2가지 경우의 수 구현*/
   /*ack 번호가 base 이전, ack의 번호가 base와 base+windowsize(<=buffer)이전, base+window size 바깥*/
   //1.ack 번호 base 이전일시 -> 무적권 디스카드
   //2.ack 적절한 범위(base~base+window_size) 일때, 
   // buffer에 저장된 PKT의 ack 확인 -> 안차있을 경우 -> ack 넣음, 차있을 경우 -> drop
   // 안차있는데 심지어 base다. 그럼 base 앞으로 forward 

   
   /* Case I. Ack 번호가 Base 이전 */
   if (packet.acknum < A.base) //if in duplicated ack situation
   {
      printf("A: got (ack=%d), it was duplicated drop. Now Base is %d \n", packet.acknum,A.base); 
      return; // ack 번호 출력 및 drop 후 함수 종료

   }

   /* 받은 ack가 적절한 범위 내의 ack 일때*/
   else if(A.base <= packet.acknum && packet.acknum <= A.base+A.window_size)// 이부분 조건문 개선필요
   {  
      //보내기로 한 pkt 집합 속에서 현재 받은 ack 번호에 해당하는 pkt 꺼내옴
      struct pkt *checkedPacket = &A.packet_buffer[packet.seqnum-1];
      if(packet.acknum == A.base)//base에 해당하는 ack 가 올 경우
      {
        /* ack 출력 후, base 전진 */
         printf("A: got ACK (ack=%d) forward base : %d, seqnum:%d\n", packet.acknum,A.base,packet.seqnum);
         stoptimer(0);
         A.base++;
         for(int i=A.base;i<A.base+A.window_size;i++)
         {
            // 전진한 base에 이미 ack가 차있다면.
            if(A.packet_buffer[i-1].acknum!=0)
               A.base++;
            else
               return; 
         }
      }
      else//base에 해당하지 않는 ack 올 경우
      {
         if(checkedPacket->acknum==0) //안차있을경우
         {
            printf("A: got ACK buffer ack! (ack=%d) seqnum:%d base:%d \n", packet.acknum,packet.seqnum,A.base);
            checkedPacket->acknum = packet.seqnum;
            return;
         }
         else//차있을경우
         {
            printf("  A : got duplicated (ack=%d). drop.\n", packet.acknum);
            return;      
         }
      }
   }
}

/* called when A's timer goes off */
/*A의 timer interrupt 발생 시 호출되는 함수*/
void A_timerinterrupt(void){ 

// 해주는 일 : 선택적 재전송 
// base에 해당하는 PKT를 다시 전송해주면 된다. 그리고 다시 타이머 스타팅.
   struct pkt *packet = &A.packet_buffer[A.base % BUFSIZE]; //인덱스 오버 방지차원에서 해놓은거구만
   printf("\n>>>A:timeout! resend packet (seq=%d): %s\n", packet->seqnum, packet->payload);
   tolayer3(0, *packet);
   starttimer(0, A.timeout);
   printf("A_timerinterrupt: timer + %f\n", A.timeout);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
/* sender에 해당하는 A 구조체를 초기화하는 함수*/
/* Input : window_size, timeout , Output:None */
void A_init(int window_size,float timeout)
{
    A.base = 1; // 교재에서 SR 설명 부분 중 send_base에 해당하는 부분입니다.sending widonw에서 가장 낮은 index의 부분의 flag입니다.
    A.nextseq = 1; //교재에서 SR설명 부분 중 nextseqnum에 해당하는 변수입니다. 전송하지 않은 패킷 인덱스 중 가장 뒷 부분의 flag입니다
    A.window_size = window_size; //Selective Repeat 의 window size입니다. base + window_size만큼의 공간이 sender window입니다.
    A.timeout = timeout; // 사용자가 입력한 timeout, timeout에 해당하는 시간이 sender에서 경과할때 까지 pkt을 받지 못하면 resend event 발생.
    A.buffer_next = 1; // sender에서 보낼 패킷들의 buffer stack에 대한 flag 입니다.
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
/* layer 3에서 PKT이 올 경우 호출됨, PKT의 seq를 순서를 확인하고 적절하면 ack layer3로 다시 보냄*/
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
   if(B.packet_buffer[packet.seqnum-1].acknum == packet.seqnum)
   {  
      printf("B: the PKT is already received. discard PKT: %d\n", packet.seqnum);
      return;
   }
   else
   {   
      if(packet.seqnum == B.base)//적당하게 들어올경우
      {
         /*pck ack 전송&payload upperlayer 전송&출력*base전진*/
         printf("\nB: recv packet (seq=%d): %s \n",packet.seqnum, packet.payload);
         tolayer5(1, packet.payload);//pkt seq 출력
         packet.acknum = packet.seqnum; //PKT에 ack 담아줌
         tolayer3(1,packet);//ack sending
        B.packet_buffer[ packet.seqnum % BUFSIZE] = packet;//패킷 어레이에 넣어주기
         printf("\nB: send ack packet:%d (seq=%d): %s,base :%d\n",packet.acknum,packet.seqnum, packet.payload,B.base);
        B.base++; //base 이동
         if(B.packet_buffer[B.base].acknum!=0)//이미 다음 slot에 packet이 차있다면
         {
            // B.base ++; //base 전진시킴.
         }
         return;
      }
      else //순서 이상하게 들어올경우 - indivdual Ack
      {  /*출력&payload upperlayer 전송&pck ack 전송*/
         printf("\nB:input outoforder PKT recv packet(buffered) (seq=%d): %s,base:%d\n", packet.seqnum, packet.payload,B.base);
         tolayer5(1, packet.payload);//pkt seq 출력
         packet.acknum = packet.seqnum; //PKT에 ack 담아줌
         tolayer3(1,packet);//ack sending
         printf("\nB:but send Ack:%d sended individually\n", packet.acknum);
         B.packet_buffer[ packet.seqnum % BUFSIZE] = packet;//패킷 어레이에 넣어주기
         return;
      }
   }
   
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{ // 이 부분은 구현하지 않습니다..
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
/* receiver에 해당하는 B 구조체를 초기화하는 함수입니다.*/
/* Input : window_size, timeout , Output:None */
void B_init(int window_size,float timeout)
{
   B.base = 1;
   B.window_size = window_size;
   B.timeout = timeout;
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
/* main 함수 수정부분 - 변수 timeout 추가 및 A_init,B_init() 인자 추가 */
int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
int window_size = 0; /* + size of SR's window */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped - not used  */
float timeout;     /* + the duration of the time for which the timer is started and till it finished */
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
   
   /*(수정)SR 수행을 위한 initializing 수행 */
   /* A-sender, B-receiver */
    init(argc, argv);
    A_init(window_size,timeout); // 문제의 조건에 해당하는 windowsize, timeout을 추가합니다.
    B_init(window_size,timeout); // 문제의 조건에 해당하는 windowsize, timeout을 추가합니다.

    while (1){
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

    if (argc != 7) {
        printf("usage: %s  Num_Message  prob_loss  window_size  AVGintervalfromsender'layer5  timeout  debug_level\n", argv[0]);
        exit(1);
    }
   /* 이부분에 number of message, loss prob, Window size, */
   /* SR 입력변수 정의부 - argv array 를 통해 입력 받은 변수들을 점검하고 사용자에게 확인 후 SR procedure을 진행합니다*/
    nsimmax = atoi(argv[1]); // 1.number of message to simulate
    lossprob = atof(argv[2]); // 2.loss probabiltiy
    window_size = atoi(argv[3]); // 3.window size of SR
    lambda = atof(argv[4]);//AVG time between message from sender's layer5
    timeout = atof(argv[5]); //3. the duration of the time from which the timer is stareted and till it's finished
    TRACE = atoi(argv[6]);
    corruptprob = 0; // 실습에서는 corrupt 가능성을 0으로 가정합니다.

    printf("-----  Selective Repeat Simulator by KIM DONG HO-------- \n\n");
    printf("the number of messages to simulate: %d\n", nsimmax);
    printf("packet loss probability: %f\n", lossprob);
    printf("Seltive Repeat's WINDOW SIZE: %d\n", window_size);
    printf("average time between messages from sender's layer5: %f\n", lambda);
    printf("timeout(duration of the time): %f\n", timeout);
    printf("for MY DEBUGGING TRACE: %d\n\n\n\n", TRACE);


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