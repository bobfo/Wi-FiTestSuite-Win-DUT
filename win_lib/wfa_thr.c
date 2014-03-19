
/****************************************************************************
 *  (c) Copyright 2007 Wi-Fi Alliance.  All Rights Reserved
 *
 *  Author: Sandeep Mohan Bharadwaj	Email:sbharadwaj@wi-fi.org
 *
 *  LICENSE
 *
 *  License is granted only to Wi-Fi Alliance members and designated
 *  contractors ($B!H(BAuthorized Licensees$B!I(B)..AN  Authorized Licensees are granted
 *  the non-exclusive, worldwide, limited right to use, copy, import, export
 *  and distribute this software:
 *  (i) solely for noncommercial applications and solely for testing Wi-Fi
 *  equipment; and
 *  (ii) solely for the purpose of embedding the software into Authorized
 *  Licensee$B!G(Bs proprietary equipment and software products for distribution to
 *  its customers under a license with at least the same restrictions as
 *  contained in this License, including, without limitation, the disclaimer of
 *  warranty and limitation of liability, below..AN  The distribution rights
 *  granted in clause
 *  (ii), above, include distribution to third party companies who will
 *  redistribute the Authorized Licensee$B!G(Bs product to their customers with or
 *  without such third party$B!G(Bs private label. Other than expressly granted
 *  herein, this License is not transferable or sublicensable, and it does not
 *  extend to and may not be used with non-Wi-Fi applications..AN  Wi-Fi Alliance
 *  reserves all rights not expressly granted herein..AN 
 *.AN 
 *  Except as specifically set forth above, commercial derivative works of
 *  this software or applications that use the Wi-Fi scripts generated by this
 *  software are NOT AUTHORIZED without specific prior written permission from
 *  Wi-Fi Alliance.
 *.AN 
 *  Non-Commercial derivative works of this software for internal use are
 *  authorized and are limited by the same restrictions; provided, however,
 *  that the Authorized Licensee shall provide Wi-Fi Alliance with a copy of
 *  such derivative works under a perpetual, payment-free license to use,
 *  modify, and distribute such derivative works for purposes of testing Wi-Fi
 *  equipment.
 *.AN 
 *  Neither the name of the author nor "Wi-Fi Alliance" may be used to endorse
 *  or promote products that are derived from or that use this software without
 *  specific prior written permission from Wi-Fi Alliance.
 *
 *  THIS SOFTWARE IS PROVIDED BY WI-FI ALLIANCE "AS IS" AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY, NON-INFRINGEMENT AND FITNESS FOR A.AN PARTICULAR PURPOSE,
 *  ARE DISCLAIMED. IN NO EVENT SHALL WI-FI ALLIANCE BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, THE COST OF PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE) ARISING IN ANY WAY OUT OF
 *  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *  ******************************************************************************
 */
/*       Revision History:
 *       2006/11/10  -- initial created by qhu
 *       2007/02/15  -- WMM Extension Beta released by qhu, mkaroshi
 *       2007/03/30  -- 01.40 WPA2 and Official WMM Beta Release by qhu
 *       2007/04/20 --  02.00 WPA2 and Official WMM Release by qhu
 *       2007/08/15 --  02.10 WMM-Power Save release by qhu
 *       2007/10/10 --  02.20 Voice SOHO beta -- qhu
 *       2007/11/07 --  02.30 Voice HSO -- qhu
 *       2007/12/10 --  02.32 no change
 *       2008/03/12 --  02.41 Bug #15, could cause "buffer overflow" in 
 *                            function "wfaSendStatsResp()" by increasing 
 *                            the "BYTE buff[]" size.
 *
 *                            Bug #19, potential race condition in function 
 *                            "sender()". Change the order of calling 
 *                            "wfaWMMPwrOn()" and "usleep()", also add 
 *                            "wfaSetDUTPwrMgmt(psave)" to turn PS ON before 
 *                            confirming start a test in function 
 *                            wfaStaSndConfirm().
 *
 *                            Bug #18, the hello counter limit could fails 
 *                            other tests. Reset num_hello count in function 
 *                            WfaSndHello()
 *
 */

/*
 * For MADWIFI driver, the TOS to 11E queue mapping as:
 *    0x08, 0x20            ----> WME_AC_BK;
 *    0x28, 0xa0            ----> WMC_AC_VI;
 *    0x30, 0xe0 0x88, 0xb8 ----> WME_AC_VO
 *      here 0x88 for UPSD, will be implemented later
 *    all other/default     ----> WME_AC_BE;
 */

#ifdef WFA_WMM_EXT
#include <stdio.h>
#include <sys/types.h>
#ifdef _WINDOWS
#include <winsock.h>
#else
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "wfa_debug.h"

#include "wfa_types.h"
#include "wfa_main.h"
#include "wfa_tg.h"
#include "wfa_debug.h"
#include "wfa_tlv.h"
#include "wfa_sock.h"
#include "wfa_rsp.h"
#include "wfa_wmmps.h"

/*
 * external global thread sync variables
 */
extern tgWMM_t wmm_thr[];
extern int resetsnd;
extern int resetrcv;
extern int newCmdOn;

extern tgStream_t *findStreamProfile(int id);
extern int gxcSockfd,vend;
extern int wfaSetProcPriority(int);
extern tgStream_t gStreams[WFA_MAX_TRAFFIC_STREAMS];
extern DWORD WINAPI wfa_traffic_resend_results_thread(void *direction);

extern int        tgSockfds[WFA_MAX_TRAFFIC_STREAMS];

//extern tgStream_t *gStreams;
extern unsigned short wfa_defined_debug;
extern DWORD recvThr;
extern int tgWMMTestEnable;
int num_stops=0;
int num_hello=0;

int mainSendThread;

#ifdef WFA_WMM_PS_EXT
extern int gtgWmmPS;
extern int psSockfd;
extern int **ac_seq;
extern wfaWmmPS_t wmmps_info;
int msgsize=256;

extern void wfaSetDUTPwrMgmt(int mode);
extern void BUILD_APTS_MSG(int msg, unsigned long *txbuf);
extern void wmmps_wait_state_proc();

extern void mpx(char *m, void *buf_v, int len);
#endif /* WFA_WMM_PS_EXT */

extern unsigned int psTxMsg[512];
extern unsigned int psRxMsg[512];

extern void tmout_stop_send(int);
extern StationProcStatetbl_t stationProcStatetbl[LAST_TEST+1][11];

int nsent;

//extern HANDLE              hStdOut;

/*
 * wfaTGSetPrio(): This depends on the network interface card.
 *               So you might want to remap according to the driver
 *               provided.
 *               The current implementation is to set the TOS/DSCP bits
 *               in the IP header
 */
int wfaTGSetPrio(int sockfd, int tgClass)
{
    int tosval;
    int threadPrio;
    
    int size = sizeof(tosval);
    getsockopt(sockfd, IPPROTO_IP, IP_TOS, (char *)&tosval, &size);

    switch(tgClass)
    {
       case TG_WMM_AC_BK:
       /*Change this value to the ported device*/
#if 0
	   if(vend == WMM_ATHEROS)
		tosval = 0x08;
	   else
#endif
       tosval = TOS_BK;
       threadPrio = THREAD_PRIORITY_BELOW_NORMAL;
       break;
       case TG_WMM_AC_BK2:
       /* Change this value to the ported device */
       tosval = TOS_LE;
       threadPrio = THREAD_PRIORITY_BELOW_NORMAL;       
       break;
       case TG_WMM_AC_VI:
       tosval = TOS_VI;
       threadPrio = THREAD_PRIORITY_NORMAL;       
       break;
       case TG_WMM_AC_VI2:
       /* Change this value to the ported device */
       tosval = TOS_VI4;
       threadPrio = THREAD_PRIORITY_NORMAL;
       break;
       case TG_WMM_AC_UAPSD:
       tosval = 0x88;
       break;
       case TG_WMM_AC_VO:
       /*Change this value to the ported device*/
		   tosval=TOS_VO7;
	   //else
       //tosval = TOS_VO;
       //tosval = 0x30;
//#       tosval = 0x88;
       threadPrio = THREAD_PRIORITY_ABOVE_NORMAL;       
       break;
       case TG_WMM_AC_VO2:
       /*Change this value to the ported device*/
       tosval = TOS_VO7;
       threadPrio = THREAD_PRIORITY_ABOVE_NORMAL;       
       break;
       case TG_WMM_AC_BE:
       tosval = TOS_BE;
       threadPrio = THREAD_PRIORITY_NORMAL;
       break;
       case TG_WMM_AC_BE2:
       tosval = TOS_EE;
       threadPrio = THREAD_PRIORITY_NORMAL;       ;
       break;
       default:
         tosval = 0x00;
           /* default */
         ;
    }

    if(tgWMMTestEnable == 0)
    {
       SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    }
    else
    {
       SetThreadPriority(GetCurrentThread(), threadPrio);
    } 

#ifdef WFA_WMM_EXT 
#ifdef WFA_WMM_PS_EXT
    psTxMsg[1] = tosval;
#endif
#endif
#ifdef _WINDOWS
    if(setsockopt ( sockfd, IPPROTO_IP, IP_TOS, (char *)&tosval, sizeof(tosval)) < 0)
#else
    if(setsockopt ( sockfd, IPPROTO_IP, IP_TOS, &tosval, sizeof(tosval)) != 0)
#endif
    {
       DPRINT_ERR(WFA_ERR, "Failed to set IP_TOS\n");
    }
	printf("set the TOS as %d\n",tosval);
    return (tosval == 0xE0)?0xD8:tosval;
}


#ifdef resolve
/*
 * wfaSetThreadPrio():
 *    Set thread priorities
 *    It is an optional experiment if you decide not necessary.
 */
void wfaSetThreadPrio(int tid, int class)
{
    struct sched_param tschedParam;
    pthread_attr_t tattr;

    pthread_attr_init(&tattr);
    pthread_attr_setschedpolicy(&tattr, SCHED_RR);

    switch(class)
    {
       case TG_WMM_AC_BK:
       tschedParam.sched_priority = 70-3;
       break;
       case TG_WMM_AC_VI:
       tschedParam.sched_priority = 70-1;
       break;
       case TG_WMM_AC_VO:
       tschedParam.sched_priority = 70;
       break;
       case TG_WMM_AC_BE:
       tschedParam.sched_priority = 70-2;
       default:
           /* default */
         ;
    }

    pthread_attr_setschedparam(&tattr, &tschedParam);
}
#endif
/* 
 * collects the traffic statistics from other threads and 
 * sends the collected information to CA
 */
void  wfaSentStatsResp(int sock, BYTE *buf)
{
   int i, total=0, pkLen;
   tgStream_t *allStreams = gStreams;
   dutCmdResponse_t *sendStatsResp = (dutCmdResponse_t *)buf, *first;
   char buff[WFA_BUFF_1K];
   printf("wfaSentStatsResp: gstream is %d\n",gStreams);
   if(sendStatsResp == NULL)
      return;

   first = sendStatsResp;
 
   for(i = 0; i < WFA_MAX_TRAFFIC_STREAMS; i++)
   {
      if((allStreams->id != 0) && (allStreams->profile.direction == DIRECT_SEND) && (allStreams->state == WFA_STREAM_ACTIVE))
      {
          sendStatsResp->status = STATUS_COMPLETE;
          sendStatsResp->streamId = allStreams->id;
          memcpy(&sendStatsResp->cmdru.stats, &allStreams->stats, sizeof(tgStats_t));          

          sendStatsResp++;
          total++;
      } 
      allStreams++;
   } 

#if 0
   printf("%i %i %i %i %i %i\n", first->cmdru.stats.txFrames,
                                  first->cmdru.stats.rxFrames,
                                  first->cmdru.stats.txPayloadBytes,
                                  first->cmdru.stats.rxPayloadBytes); 
#endif

   wfaEncodeTLV(WFA_TRAFFIC_AGENT_SEND_RESP_TLV, total*sizeof(dutCmdResponse_t),
                 (BYTE *)first, (BYTE *)buff);

   pkLen = WFA_TLV_HDR_LEN + total*sizeof(dutCmdResponse_t); 

#if 0
   for(i = 0; i< pkLen; i++)
   printf("%x ", buff[i]);

   printf("\n");
#endif

   if(wfaCtrlSend(sock, (BYTE *)buff, pkLen) != pkLen)
   {
       DPRINT_WARNING(WFA_WNG, "wfaCtrlSend Error\n");
   }
printf("wfaSentStatsResp exit: gstream is %d\n",gStreams);
   return;
}

#ifdef WFA_WMM_PS_EXT
/*
 * sender(): This is a generic function to send a packed for the given dsc 
 *               (ac:VI/VO/BE/BK), before sending the packet the function
 *               puts the station into the PS mode indicated by psave and 
 *               sends the packet after sleeping for sllep_period
 */
int sender(char psave,int sleep_period,int dsc)
{
   int r;

   PRINTF("\nsleeping for %d",sleep_period);
   wfaSetDUTPwrMgmt(psave);
   usleep(sleep_period);
   PRINTF("\nAfter create sending %d\n",dsc);
   create_apts_msg(APTS_DEFAULT, psTxMsg,wmmps_info.my_sta_id);
   wfaTGSetPrio(psSockfd, dsc);
   PRINTF("\nAfter create");
   PRINTF("\nlock met");
   r = sendto(psSockfd, psTxMsg, msgsize, 0, (struct sockaddr *)&wmmps_info.psToAddr, sizeof(struct sockaddr));
   return r;
}
/*
 * wfaStaSndHello(): This function sends a Hello packet 
 *                and sleeps for sleep_period, the idea is
 *                to keep sending hello packets till the console
 *                responds, the function keeps a check on the MAX
 *                Hellos and if number of Hellos exceed that it quits
 */
int WfaStaSndHello(char psave,int sleep_period,int *state)
{
   int r;
   tgWMM_t *my_wmm = &wmm_thr[wmmps_info.ps_thread];

   if(!(num_hello++))
       create_apts_msg(APTS_HELLO, psTxMsg,0);
   r = sendto(psSockfd, psTxMsg, msgsize, 0, (struct sockaddr *)&wmmps_info.psToAddr, sizeof(struct sockaddr));

   usleep(sleep_period);

   pthread_mutex_lock(&my_wmm->thr_flag_mutex);
   if(my_wmm->thr_flag)
   {
       (*state)++;
       num_hello=0;
       my_wmm->thr_flag=0;
   }

   pthread_mutex_unlock(&my_wmm->thr_flag_mutex);
   if(num_hello > MAXHELLO)
   {
       DPRINT_ERR(WFA_ERR, "Too many Hellos sent\n");
       gtgWmmPS = 0;
       num_hello=0;
       close(psSockfd);
       psSockfd = -1;
       signal(SIGALRM, SIG_IGN);
   } 

   return 0;
}

/*
 * wfaStaSndConfirm(): This function sends the confirm packet
 *                which is sent after the console sends the
 *                test name to the station
 */
int WfaStaSndConfirm(char psave,int sleep_period,int *state)
{
   int r;
   static int num_hello=0;
   wfaSetDUTPwrMgmt(psave);
   if(!num_hello)
      create_apts_msg(APTS_CONFIRM, psTxMsg,0);
   r = sendto(psSockfd, psTxMsg, msgsize, 0, (struct sockaddr *)&wmmps_info.psToAddr, sizeof(struct sockaddr));
   (*state)++;

   return 0;
}

/*
 * WfaStaSndVO(): This function sends a AC_VO packet
 *                after the time specified by sleep_period
 *                and advances to the next state for the given test case
 */
int WfaStaSndVO(char psave,int sleep_period,int *state)
{
   int r;
   static int en=1;
   PRINTF("\r\nEnterring WfastasndVO %d",en++);
   if ((r=sender(psave,sleep_period,TG_WMM_AC_VO))>=0)
      (*state)++;
   else
      PRINTF("\r\nError\n");

   return 0;
}

/*
 * WfaStaSnd2VO(): This function sends two AC_VO packets
 *                after the time specified by sleep_period
 *                and advances to the next state for the given test case
 */
int WfaStaSnd2VO(char psave,int sleep_period,int *state)
{
   int r;
   static int en=1;

   PRINTF("\r\nEnterring WfastasndVO %d",en++);
   if ((r=sender(psave,sleep_period,TG_WMM_AC_VO))>=0)
   {
       r = sendto(psSockfd, psTxMsg, msgsize, 0, (struct sockaddr *)&wmmps_info.psToAddr, sizeof(struct sockaddr));
       mpx("STA msg",psTxMsg,64);
       (*state)++;
   }
   else
      PRINTF("\r\nError\n");

   return 0;
}

/*
 * WfaStaSndVI(): This function sends a AC_VI packet
 *                after the time specified by sleep_period
 *                and advances to the next state for the given test case
 */
int WfaStaSndVI(char psave,int sleep_period,int *state)
{
   int r;
   static int en=1;

   PRINTF("\r\nEnterring WfastasndVI %d",en++);
   if ((r=sender(psave,sleep_period,TG_WMM_AC_VI))>=0)
      (*state)++;

   return 0;
}

/*
 * WfaStaSndBE(): This function sends a AC_BE packet
 *                after the time specified by sleep_period
 *                and advances to the next state for the given test case
 */
int WfaStaSndBE(char psave,int sleep_period,int *state)
{
   int r;
   static int en=1;

   PRINTF("\r\nEnterring WfastasndBE %d",en++);
   if ((r=sender(psave,sleep_period,TG_WMM_AC_BE))>=0)
      (*state)++;

   return 0;
}
/*
 * WfaStaSndBK(): This function sends a AC_BK packet
 *                after the time specified by sleep_period
 *                and advances to the next state for the given test case
 */
int WfaStaSndBK(char psave,int sleep_period,int *state)
{
   int r;
   static int en=1;

   PRINTF("\r\nEnterring WfastasndBK %d",en++);
   if ((r=sender(psave,sleep_period,TG_WMM_AC_BK))>=0)
      (*state)++;

   return 0;
}

/*
 * WfaStaSndVOCyclic(): The function is the traffic generator for the L.1 test
 *                      caseThis function sends 3000 AC_VO packet
 *                      after the time specified by sleep_period (20ms)
 */
int WfaStaSndVOCyclic(char psave,int sleep_period,int *state)
{
   int i;
   static int en=1;

   for(i=0;i<3000;i++)
   {
       PRINTF("\r\nEnterring WfastasndVOCyclic %d",en++);
       sender(psave,sleep_period,TG_WMM_AC_VO);
       if(!(i%50))
       {
         PRINTF(".");
         fflush(stdout);
       }
   }

   (*state)++;

   return 0;
}

/*
 * WfaStaWaitStop(): This function sends the stop packet on behalf of the
 *                   station, the idea is to keep sending the Stop
 *                   till a stop is recd from the console,the functions
 *                   quits after stop threshold reaches.
 */
int WfaStaWaitStop(char psave,int sleep_period,int *state)
{
   int r;
   int myid=wmmps_info.ps_thread;
   PRINTF("\n Entering Sendwait");
   usleep(sleep_period);
   if(!num_stops)
   {
       wfaSetDUTPwrMgmt(psave);
       wfaTGSetPrio(psSockfd, TG_WMM_AC_BE);
   }

   num_stops++;
   create_apts_msg(APTS_STOP, psTxMsg,wmmps_info.my_sta_id);
   r = sendto(psSockfd, psTxMsg, msgsize, 0, (struct sockaddr *)&wmmps_info.psToAddr, sizeof(struct sockaddr));
   mpx("STA msg",psTxMsg,64);

   wmm_thr[myid].stop_flag = 1;
   pthread_mutex_lock(&wmm_thr[myid].thr_stop_mutex);
   pthread_cond_signal(&wmm_thr[myid].thr_stop_cond);
   pthread_mutex_unlock(&wmm_thr[myid].thr_stop_mutex);

   if(num_stops > MAX_STOPS)
   {
       DPRINT_ERR(WFA_ERR, "Too many stops sent\n");
       gtgWmmPS = 0;
       close(psSockfd);
       psSockfd = -1;
       signal(SIGALRM, SIG_IGN);
       //wfaSetDUTPwrMgmt(PS_OFF);
   }

   return 0;
}

#endif 
#ifdef _WINDOWS
DWORD WINAPI wfa_wmm_sleep_thread(void *thr_param)
{
	int sleep_prd = *(int *)thr_param;
	Sleep(sleep_prd);
    tmout_stop_send(0);
	return 0;
}
#endif
#ifdef _WINDOWS
DWORD WINAPI wfa_wmm_thread(void *thr_param)
#else
void * wfa_wmm_thread(void *thr_param)
#endif
{
    int myId = ((tgThrData_t *)thr_param)->tid;
    tgThrData_t *tdata =(tgThrData_t *) thr_param;
    tgWMM_t *my_wmm = &wmm_thr[myId];
    tgStream_t *myStream = NULL;
    int myStreamId;
    int mySock, status, respLen;
    tgProfile_t *myProfile;
    BYTE respBuf[WFA_BUFF_1K];
	int rdirect = DIRECT_SEND;
int resendcnt = 0;
#ifdef WFA_WMM_PS_EXT
    StationProcStatetbl_t  curr_state;
#endif
#ifndef _WINDOWS
      pthread_attr_t tattr;
    
      pthread_attr_init(&tattr);
      pthread_attr_setschedpolicy(&tattr, SCHED_RR);
#else
	DWORD thr_id;
	int timer_dur;

    int iOptVal;
    int iOptLen = sizeof(int);

#endif

    while(1)
    {
#ifdef _WINDOWS
		printf("Prestart %d ...\n",myId);
	    fflush(stdout);
		while(!my_wmm->thr_flag)
		{
		    WaitForSingleObject( my_wmm->thr_flag_mutex, INFINITE );
//			printf("pass the lock\n");
//            ReleaseMutex(my_wmm->thr_flag_mutex);
			Sleep(20);
		}
//            WaitForSingleObject( my_wmm->thr_flag_mutex,500);
	
        
        ReleaseMutex(my_wmm->thr_flag_mutex);		
        myStreamId = my_wmm->thr_flag;
        my_wmm->thr_flag = 0;
		printf("lock met %d ...\n",myId);
#else


#endif

       /* use the flag as a stream id to file the profile */ 
       myStream = findStreamProfile(myStreamId); 
       myProfile = &myStream->profile;

       if(myProfile == NULL)
       {
          status = STATUS_INVALID;
          wfaEncodeTLV(WFA_TRAFFIC_AGENT_SEND_RESP_TLV, 4, (BYTE *)&status, respBuf);
          respLen = WFA_TLV_HDR_LEN+4;
          /*
           * send it back to control agent.
           */
           continue;
       }

       printf("start ...\n");
       switch(myProfile->direction)
       {
           case DIRECT_SEND:
           printf("creating socket with port %d ...\n",myProfile->sport);
           mySock = wfaCreateUDPSock(myProfile->sipaddr, myProfile->sport);
		   if(mySock < 0)
				break;
           wfaConnectUDPPeer(mySock, myProfile->dipaddr, myProfile->dport);
           /*
            * Set packet/socket priority TOS field
            */
           wfaTGSetPrio(mySock, myProfile->trafficClass);

           /*
            * set a proper priority 
            */
#ifdef resolve
		   wfaSetThreadPrio(myId, myProfile->trafficClass);
#endif
           /* if delay is too long, it must be something wrong */
           if(myProfile->startdelay > 0 && myProfile->startdelay<100)
           {
#ifdef _WINDOWS
               Sleep(1000 * myProfile->startdelay);
#else
               sleep(myProfile->startdelay);
#endif
           }

           /*
            * set timer fire up
            */
#ifdef WFA_WMM_EXT
#ifndef _WINDOWS
           signal(SIGALRM, tmout_stop_send);
           alarm(myProfile->duration);
#else
		   
		   printf("\r\n Thread %d Setting timer for %d ms\n",myId,1000*myProfile->duration);
		   timer_dur = 1000*myProfile->duration;
		   CreateThread(NULL, 0, wfa_wmm_sleep_thread, (PVOID)&timer_dur, 0,
		   &thr_id);
		   //my_wmm->timerid = SetTimer(0, 0, 1000*myProfile->duration, (TIMERPROC)tmout_stop_send);
		   //printf("SetTimer returned %d\n",my_wmm->timerid);
#endif

#endif

           if (getsockopt(mySock, SOL_SOCKET, SO_SNDBUF, (char*)&iOptVal, &iOptLen) != SOCKET_ERROR) 
	       {
               printf("SO_SNDBUF Value: %ld\n", iOptVal);
           }

	       iOptVal = iOptVal*40;
		   //iOptVal = 262120;

	       setsockopt(mySock, SOL_SOCKET, SO_SNDBUF, (char*)&iOptVal, &iOptLen);

           if (getsockopt(mySock, SOL_SOCKET, SO_SNDBUF, (char*)&iOptVal, &iOptLen) != SOCKET_ERROR) 
		   {
               printf("SO_SNDBUF Value: %ld\n", iOptVal);
           }

           wfaSendLongFile(mySock, myStreamId, respBuf, &respLen);
		   //KillTimer(NULL,my_wmm->timerid);
           memset(respBuf, 0, WFA_BUFF_1K);
#ifdef _WINDOWS
		   printf("\r\n Closing socket for thread %d",myId);
		   closesocket(mySock);
           Sleep(1000);
#else
           sleep(1);
#endif

	       /*
            * uses thread 0 to pack the items and ships it to CA.
            */
           //SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	       if(myId == mainSendThread) {
		       wfaSentStatsResp(gxcSockfd, respBuf);
			   newCmdOn = 0;
	       
               resendcnt = 0;
		       while(newCmdOn == 0 && resendcnt < 2)
		       {
		           Sleep(3000);
                   wfaSentStatsResp(gxcSockfd, respBuf);
			       resendcnt++;
		       }
			   printf("finish resendsn %i", resendcnt);
		   }
           SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
		   
#if 0
		   if(newCmdOn == 1)
              CreateThread(NULL, 0, wfa_traffic_resend_results_thread, (PVOID)&rdirect, 0, &thr_id);
#endif
           break;

	   case DIRECT_RECV:
#ifdef WFA_WMM_PS_EXT
           /*
            * Test WMM-PS
            */
           if(myProfile->profile == PROF_UAPSD)
           {

		
                wmmps_info.sta_test = B_D;
                wmmps_info.ps_thread = myId;
                wmmps_info.rcv_state = 0;
                wmmps_info.tdata = tdata;
  		wmmps_info.dscp = wfaTGSetPrio(psSockfd, TG_WMM_AC_BE);
            	tdata->state_num=0;
               /*
                * default timer value
                */


               while(gtgWmmPS>0)
               {
                        if(resetsnd)
                        {
                          tdata->state_num = 0;
                          resetsnd = 0;
                        }
            		tdata->state =  stationProcStatetbl[wmmps_info.sta_test];
      			curr_state = tdata->state[tdata->state_num];
      			curr_state.statefunc(curr_state.pw_offon,curr_state.sleep_period,&(tdata->state_num));
               }
           }
           
#endif /* WFA_WMM_PS_EXT */

		   if(myProfile->profile == PROF_IPTV || myProfile->profile == PROF_FILE_TX || myProfile->profile == PROF_MCAST)
		   {
			    int nbytes = 0;
                char recvBuf[3072];

				mySock = wfaCreateUDPSock(myProfile->sipaddr, myProfile->sport);
                tgSockfds[myStream->tblidx] = mySock;

                if (getsockopt(mySock, SOL_SOCKET, SO_RCVBUF, (char*)&iOptVal, &iOptLen) != SOCKET_ERROR) 
	            {
                    printf("SO_RCVBUF Value: %ld\n", iOptVal);
                }

	            iOptVal = iOptVal*40;
		        //iOptVal = 262120;

	            setsockopt(mySock, SOL_SOCKET, SO_RCVBUF, (char*)&iOptVal, &iOptLen);

                if (getsockopt(mySock, SOL_SOCKET, SO_RCVBUF, (char*)&iOptVal, &iOptLen) != SOCKET_ERROR) 
		        {
                    printf("SO_RCVBUF Value: %ld\n", iOptVal);
                }

                for (;;)
		        {
                    nbytes = wfaRecvFile(mySock, myStreamId, (char  *)recvBuf);

                    //<<<----------------------------------------------------------------------------------------
                    //jira issue: SIG-465
                    //To enusre the loop exit if the nbytes becomes 0 due to the graceful closure of remote socket
                    if (nbytes==0)
                    {
                      DPRINT_ERR(WFA_OUT, "The connection has been gracefully closed, the return value is zero");
                      break;
                    }
                    //---------------------------------------------------------------------------------------->>>
			        if(nbytes== -1)
			        {
				        printf("Error recving\n");
#if 0
						if(mySock
						close(mySock);
						mySock = -1;
                        tgSockfds[myStream->tblidx] = -1;
#endif
						break;
			        }
		        }


		   }

           break;
           default:
              DPRINT_ERR(WFA_ERR, "Unknown covered case\n");
       }

    }
}

#endif