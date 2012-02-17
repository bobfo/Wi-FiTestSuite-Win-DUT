/****************************************************************************
 *  (c) Copyright 2007 Wi-Fi Alliance.  All Rights Reserved
 *
 * 
 *  LICENSE
 *
 *  License is granted only to Wi-Fi Alliance members and designated 
 *  contractors (“Authorized Licensees”).  Authorized Licensees are granted 
 *  the non-exclusive, worldwide, limited right to use, copy, import, export 
 *  and distribute this software: 
 *  (i) solely for noncommercial applications and solely for testing Wi-Fi 
 *  equipment; and 
 *  (ii) solely for the purpose of embedding the software into Authorized 
 *  Licensee’s proprietary equipment and software products for distribution to 
 *  its customers under a license with at least the same restrictions as 
 *  contained in this License, including, without limitation, the disclaimer of 
 *  warranty and limitation of liability, below.  The distribution rights 
 *  granted in clause 
 *  (ii), above, include distribution to third party companies who will 
 *  redistribute the Authorized Licensee’s product to their customers with or 
 *  without such third party’s private label. Other than expressly granted 
 *  herein, this License is not transferable or sublicensable, and it does not 
 *  extend to and may not be used with non-Wi-Fi applications.  Wi-Fi Alliance 
 *  reserves all rights not expressly granted herein. 
 * 
 *  Except as specifically set forth above, commercial derivative works of 
 *  this software or applications that use the Wi-Fi scripts generated by this 
 *  software are NOT AUTHORIZED without specific prior written permission from 
 *  Wi-Fi Alliance.
 * 
 *  Non-Commercial derivative works of this software for internal use are 
 *  authorized and are limited by the same restrictions; provided, however, 
 *  that the Authorized Licensee shall provide Wi-Fi Alliance with a copy of 
 *  such derivative works under a perpetual, payment-free license to use, 
 *  modify, and distribute such derivative works for purposes of testing Wi-Fi 
 *  equipment.
 * 
 *  Neither the name of the author nor "Wi-Fi Alliance" may be used to endorse 
 *  or promote products that are derived from or that use this software without 
 *  specific prior written permission from Wi-Fi Alliance.
 *
 *  THIS SOFTWARE IS PROVIDED BY WI-FI ALLIANCE "AS IS" AND ANY EXPRESS OR 
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 *  OF MERCHANTABILITY, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE, 
 *  ARE DISCLAIMED. IN NO EVENT SHALL WI-FI ALLIANCE BE LIABLE FOR ANY DIRECT, 
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 *  (INCLUDING, BUT NOT LIMITED TO, THE COST OF PROCUREMENT OF SUBSTITUTE 
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 *  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE) ARISING IN ANY WAY OUT OF 
 *  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 */

/*    Revision History:
 *    2007/07/25  -- Initial created by qhu
 *    2007/08/15  -- 02.10 WMM-Power Save Released by qhu
 *    2007/10/10 --  02.20 Voice SOHO beta -- qhu
 *    2007/11/07 --  02.30 Voice HSO -- qhu
 *    2007/12/10 --  02.32 no change
 *    2008/02/07 --  02.40 Upgrade the new WMM-PS method.
 *    2008/03/20 --  02.41 Bug #18, the hello counter limit could fails other
 *                         tests. Reset num_hello in wfaRcvProc(). 
 *                         Not Bug, print message too long and frequent. Remove
 *                         trace messages for L1.
 *    2009/09/30 -- WINv03.00.00 - Simga 3.0 Release, supports TGn Program including WMM and WPA2 [QH]
 *
 */
#include "stdafx.h"
#ifdef WFA_WMM_EXT
#ifdef WFA_WMM_PS_EXT
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "wfa_sock.h"
#include "wfa_types.h"
#include "wfa_tg.h"
#include "wfa_ca.h"
#include "wfa_wmmps.h"
#include "wfa_main.h"
#include "wfa_debug.h"

extern int psSockfd;
extern int num_stops;
extern int num_hello;
extern tgWMM_t wmm_thr[];

extern unsigned int psTxMsg[512];
extern unsigned int psRxMsg[512];
extern char gCmdStr[WFA_CMD_STR_SZ];
extern int msgsize;

int resetsnd=0;
int reset_recd=0;
int resetrcv=0;
int num_retry=0;
int gtgPsPktRecvd = 0;                    // need to reset
struct timeval time_ap;
struct timeval time_ul;

extern wfaWmmPS_t wmmps_info;
extern int gtgWmmPS;

extern int wfaTGSetPrio(int sockfd, int tgClass);

void wmmps_wait_state_proc();
/* APTS messages*/
struct apts_msg apts_msgs[] ={
	{0, -1},
	{"B.D", B_D},	
	{"B.H", B_H},
        {"B.B", B_B},
	{"B.M", B_M},	
	{"M.D", M_D},	
	{"B.Z", B_Z},	
	{"M.Y", M_Y},	
	{"L.1", L_1},	
	{"A.Y", A_Y},	
	{"B.W", B_W},	
	{"A.J", A_J},	
	{"M.V", M_V},	
	{"M.U", M_U},	
        {"A.U", A_U},	
	{"M.L", M_L},	
	{"B.K", B_K},	
	{"M.B", M_B},	
	{"M.K", M_K},	
	{"M.W", M_W},	
#ifdef WFA_WMM_AC
	{"ATC1", ATC1},	
	{"ATC2", ATC2},	
	{"ATC3", ATC3},	
	{"ATC4", ATC4},	
	{"ATC5", ATC5},	
	{"ATC6", ATC6},	
	{"ATC7", ATC7},	
	{"ATC8", ATC8},	
	{"ATC9", ATC9},	
	{"ATC10", ATC10},	
	{"ATC11", ATC11},	
	{"STC1", STC1},	
	{"STC2", STC2},	
	{"STC3", STC3},	
#endif
	{"APTS TX         ", APTS_DEFAULT },
	{"APTS Hello      ", APTS_HELLO },
	{"APTS Broadcast  ", APTS_BCST },
	{"APTS Confirm    ", APTS_CONFIRM},
	{"APTS STOP       ", APTS_STOP},
	{"APTS CK BE      ", APTS_CK_BE },
	{"APTS CK BK      ", APTS_CK_BK },
	{"APTS CK VI      ", APTS_CK_VI },
	{"APTS CK VO      ", APTS_CK_VO },
	{"APTS RESET      ", APTS_RESET },
	{"APTS RESET RESP ", APTS_RESET_RESP },
	{"APTS RESET STOP ", APTS_RESET_STOP },
	{0, 0 }		// APTS_LAST
};
/* The DUT recv table for each of the test cases*/
StationRecvProcStatetbl_t stationRecvProcStatetbl[LAST_TEST+1][6] = 
{
         {WfaRcvStop,0,0,0,0},
 /*B.D*/ {WfaRcvProc,WfaRcvVO,WfaRcvStop,0,0,0},
 /*B.H*/ {WfaRcvProc,WfaRcvVO,WfaRcvVO,WfaRcvStop,0,0},
 /*B.B*/ {WfaRcvProc,WfaRcvStop,0,0,0,0},
 /*B.M*/ {WfaRcvProc,WfaRcvStop,0,0,0,0},
 /*M.D*/ {WfaRcvProc,WfaRcvBE,WfaRcvBK,WfaRcvVI,WfaRcvVO,WfaRcvStop},
 /*B.Z*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvStop,0,0},
 /*M.Y*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvBE,WfaRcvStop,0},
 /*L.1*/ {WfaRcvProc,WfaRcvVOCyclic,0,0,0,0},
 /*A.Y*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvBE,WfaRcvStop,0},
 /*B.W*/ {WfaRcvProc,WfaRcvBE,WfaRcvVI,WfaRcvBE,WfaRcvVI,WfaRcvStop},
 /*A.J*/ {WfaRcvProc,WfaRcvVO,WfaRcvVI,WfaRcvBE,WfaRcvBK,WfaRcvStop},
 /*M.V*/ {WfaRcvProc,WfaRcvBE,WfaRcvVI,WfaRcvStop,0,0},
 /*M.U*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvVO,WfaRcvVO,WfaRcvStop},
 /*A.U*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvVO,WfaRcvStop,0},
 /*M.L*/ {WfaRcvProc,WfaRcvBE,WfaRcvStop,0,0,0},
 /*B.K*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvStop,0,0},
 /*M.B*/ {WfaRcvProc,WfaRcvStop,0,0,0,0},
 /*M.K*/ {WfaRcvProc,WfaRcvBE,WfaRcvVI,WfaRcvStop,0,0},
 /*M.W*/ {WfaRcvProc,WfaRcvBE,WfaRcvBE,WfaRcvBE,WfaRcvVI,WfaRcvStop}
#ifdef WFA_WMM_AC
 /*ATC1*/ ,{WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvStop},
 /*ATC2*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvBE,WfaRcvStop},
 /*ATC3*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvBE,WfaRcvStop},
 /*ATC4*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvVO,WfaRcvVO,WfaRcvStop},
 /*ATC5*/ {WfaRcvProc,WfaRcvVI,WfaRcvVO,WfaRcvStop},
 /*ATC6*/ {WfaRcvProc,WfaRcvVO,WfaRcvVI,WfaRcvVI,WfaRcvStop},
 /*ATC7*/ {WfaRcvProc,WfaRcvVO,WfaRcvVI,WfaRcvBE,WfaRcvBK,WfaRcvStop},
 /*ATC8*/ {WfaRcvProc,WfaRcvVO,WfaRcvVI,WfaRcvStop},
 /*ATC9*/ {WfaRcvProc,WfaRcvVO,WfaRcvVI,WfaRcvBE,WfaRcvBK,WfaRcvStop},
 /*ATC10*/{WfaRcvProc,WfaRcvVO,WfaRcvVI,WfaRcvBE,WfaRcvBK,WfaRcvStop},
 /*ATC11*/{WfaRcvProc,WfaRcvVO,WfaRcvVO,WfaRcvVI,WfaRcvVO,WfaRcvStop},
 /*STC1*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvStop},
 /*STC2*/ {WfaRcvProc,WfaRcvBE,WfaRcvVI,WfaRcvStop},
 /*STC3*/ {WfaRcvProc,WfaRcvBE,WfaRcvBK,WfaRcvVI,WfaRcvVO,WfaRcvStop}
#endif
};
/* The DUT send table for each of the test cases*/

StationProcStatetbl_t stationProcStatetbl[LAST_TEST+1][11] = {
/* Dummy*/{{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}},
/* B.D*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
  {WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}        ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* B.H*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}        ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* B.B*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVI,P_ON,LII / 2}        ,{WfaStaSndBE,P_ON,LII / 2}     ,{WfaStaSndBK,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0}},

/* B.M*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,30000000},{WfaStaWaitStop,P_ON,LII / 2}     ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},	

/* M.D*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndVI,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}     ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0}},

/* B.Z*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
  {WfaStaSndVO,P_ON,LII / 2 }  ,{WfaStaWaitStop,P_ON,LII / 2}     ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.Y*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
  {WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}        ,{WfaStaSndBE,P_ON,LII / 2}     ,{WfaStaSndBE,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0}},

/* L.1*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
  {WfaStaSndVOCyclic,P_ON,20000},{WfaStaWaitStop,P_ON,LII / 2 }},

/* A.Y*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
  {WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}        ,{WfaStaSndBE,P_ON,LII / 2}     ,{WfaStaSndBE,P_OFF,LII / 2}    ,{WfaStaSndBE,P_ON,LII / 2}   ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0}}, 

/* B.W*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndVI,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* A.J*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVO,P_OFF,LII / 2},{WfaStaWaitStop	,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.V*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndBE,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop	,P_ON,LII / 2} ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.U*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
  {WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndBE,P_ON,LII / 2}        ,{WfaStaSnd2VO,P_ON,LII / 2}   ,{WfaStaWaitStop	,P_ON,LII / 2} ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* A.U*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,LII / 2}  ,{WfaStaSndBE,P_OFF,LII / 2}      ,{WfaStaSndBE,P_ON,LII / 2}    ,{WfaStaSndBE,P_OFF,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVO,P_OFF,LII / 2} ,{WfaStaWaitStop ,P_ON,LII / 2},{0,0,0}},

/* M.L*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndBE,P_ON,LII / 2}   ,{WfaStaWaitStop,P_ON,LII / 2}     ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* B.K*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndBE,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.B*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVI,P_ON,LII / 2}        ,{WfaStaSndBE,P_ON,LII / 2}     ,{WfaStaSndBK,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2} ,{0,0,0},{0,0,0},{0,0,0}},

/* M.K*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndBE,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.W*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndBE,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}}
#ifdef WFA_WMM_AC
/* ATC1*/  ,{{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVO,P_ON,1000000}   ,{WfaStaWaitStop,P_ON,LII / 2}     ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
/* ATC2*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,1000000}   ,{WfaStaSndVO,P_ON,1000000}        ,{WfaStaSndBE,P_ON,1000000}    ,{WfaStaSndBE,P_ON,1000000}  ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0}},
/* ATC3*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,becon_int}   ,{WfaStaSndVO,P_ON,1000000}      ,{WfaStaSndBE,P_ON,1000000}    ,{WfaStaSndBE,P_ON,1000000}  ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0}},
/* ATC4*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,lis_int}   ,{WfaStaSndBE,P_ON,1000000}        ,{WfaStaSndVO,P_ON,1000000}    ,{WfaStaSndVO,P_ON,1}        ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0}},
/* ATC5*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, 1},
{WfaStaSndVI,P_ON,lis_int}   ,{WfaStaSndVO,P_ON,lis_int+2*becon_int}        ,{WfaStaSndVO,P_ON,becon_int}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
/* ATC6*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,becon_int}   ,{WfaStaSndVO,P_ON,lis_int}        ,{WfaStaSndVI,P_ON,becon_int}  ,{WfaStaSndVO,P_ON,becon_int}  ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0}},
/* ATC7*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,becon_int}   ,{WfaStaSndVO,P_ON,lis_int}        ,{WfaStaSndVI,P_ON,becon_int}  ,{WfaStaSndVO,P_ON,becon_int}  ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0}},
/* ATC8*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVO,P_ON,lis_int}   ,{WfaStaSndVI,P_ON,lis_int+2*becon_int}        ,{WfaStaSndVI,P_ON,becon_int}  ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
/* ATC9*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndBE,P_ON,becon_int}   ,{WfaStaSndVO,P_ON,lis_int}        ,{WfaStaSndBE,P_ON,becon_int}  ,{WfaStaSndVI,P_ON,becon_int}  ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0}},
/* ATC10*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,becon_int}   ,{WfaStaSndBE,P_ON,lis_int}        ,{WfaStaSndVI,P_ON,becon_int}  ,{WfaStaSndBE,P_ON,becon_int}  ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0}},
/* ATC11*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndBE,P_ON,becon_int}   ,{WfaStaSndVI,P_ON,lis_int}        ,{WfaStaSndBE,P_ON,becon_int}  ,{WfaStaSndVO,P_ON,lis_int}  ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0}},
/* STC1*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,becon_int}   ,{WfaStaSndBE,P_ON,1000000}        ,{WfaStaSndVI,P_ON,1000000}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
/* STC2*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,becon_int}   ,{WfaStaSndBE,P_ON,1000000}        ,{WfaStaSndVO,P_ON,1000000}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
/* STC3*/  {{WfaStaSndHello,P_OFF, 1000000},{WfaStaSndConfirm,P_ON, LII / 2},
{WfaStaSndVI,P_ON,becon_int}   ,{WfaStaSndVI,P_ON,becon_int}        ,{WfaStaSndVI,P_ON,becon_int}    ,{WfaStaSndVI,P_ON,becon_int},{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
#endif

};

#if 0
StationRecvProcStatetbl_t stationRecvProcStatetbl[LAST_TEST+1][6] = 
{
         {WfaRcvStop,0,0,0,0,0},
 /*B.D*/ {WfaRcvProc,WfaRcvVO,WfaRcvStop,0,0,0},
 /*B.H*/ {WfaRcvProc,WfaRcvVO,WfaRcvVO,WfaRcvStop,0,0},
 /*B.B*/ {WfaRcvProc,WfaRcvStop,0,0,0,0},
 /*B.M*/ {WfaRcvProc,WfaRcvStop,0,0,0,0},
 /*M.D*/ {WfaRcvProc,WfaRcvBE,WfaRcvBK,WfaRcvVI,WfaRcvVO,WfaRcvStop},
 /*B.Z*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvStop,0,0},
 /*M.Y*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvBE,WfaRcvStop,0},
 /*L.1*/ {WfaRcvProc,WfaRcvVOCyclic,0,0,0,0},
 /*A.Y*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvBE,WfaRcvStop,0},
 /*B.W*/ {WfaRcvProc,WfaRcvBE,WfaRcvVI,WfaRcvBE,WfaRcvVI,WfaRcvStop},
 /*A.J*/ {WfaRcvProc,WfaRcvVO,WfaRcvVI,WfaRcvBE,WfaRcvBK,WfaRcvStop},
 /*M.V*/ {WfaRcvProc,WfaRcvBE,WfaRcvVI,WfaRcvStop,0,0},
 /*M.U*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvVO,WfaRcvVO,WfaRcvStop},
 /*A.U*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvVO,WfaRcvStop,0},
 /*M.L*/ {WfaRcvProc,WfaRcvBE,WfaRcvStop,0,0,0},
 /*B.K*/ {WfaRcvProc,WfaRcvVI,WfaRcvBE,WfaRcvStop,0,0},
 /*M.B*/ {WfaRcvProc,WfaRcvStop,0,0,0,0},
 /*M.K*/ {WfaRcvProc,WfaRcvBE,WfaRcvVI,WfaRcvStop,0,0},
 /*M.W*/ {WfaRcvProc,WfaRcvBE,WfaRcvBE,WfaRcvBE,WfaRcvVI,WfaRcvStop}
};
#endif

int ac_seq[APTS_LAST][6] ={
   {0,      0,      0,      0,      0},
   {0},
   {0},
   {0},
   {0},
   {0},
   {0},
   {0},
   {0},
   {0},
   {0}, // APTS_TESTS
   {0}, // B.D
   {0}, // B.2
   {0}, // B.H
   {0}, // B.4
   {0}, // B_5
   {0, 0, 0, 0, 0}, // B_6
   {TG_WMM_AC_VO, TG_WMM_AC_VI, TG_WMM_AC_BE, TG_WMM_AC_BK, 0}, // B.B B_B - 4 exchanges: 1 uplink, 0 downlink
   {0}, // B.E
   {0}, // B.G
   {0}, // B.I
   {0}, // M.D
   {0}, // M.G
   {0}, // M.I
   {0}, // B.Z  1, 1, 1, 0},	// 1 special exchange for Broadcast testing
   {TG_WMM_AC_VI, TG_WMM_AC_VO, TG_WMM_AC_BE, TG_WMM_AC_BE, 0}, //  M.Y  M_Y 2 special exchange for Broadcast testing
   {0}, // L.1
   {0}, // DLOAD
   {0}, // ULOAD
   {0}, // "APTS PASS"
   {0}, // "APTS FAIL"
   //{TOS_VI, TOS_VO, TOS_BE, TOS_BE, 0}, //  A.Y A_Y special exchange for Broadcast testing
   {TG_WMM_AC_VI, TG_WMM_AC_VO, TG_WMM_AC_BE, TG_WMM_AC_BE, TG_WMM_AC_BE}, //  A.Y A_Y special exchange for Broadcast testing
   {0}, //  B.W  2 special exchange for Broadcast testing
   {0}, //  A.J
   {TG_WMM_AC_VI, TG_WMM_AC_BE, TG_WMM_AC_VI, TG_WMM_AC_VI, TG_WMM_AC_VI}, //  M.V M_V
   {TG_WMM_AC_VI, TG_WMM_AC_BE, TG_WMM_AC_VO, TG_WMM_AC_VO, TG_WMM_AC_VO}, //  M.U M_U
   {TG_WMM_AC_VI, TG_WMM_AC_BE, TG_WMM_AC_BE, TG_WMM_AC_BE, TG_WMM_AC_VO, TG_WMM_AC_VO},  //  A.U A_U
   {0}, //  M.L M_L
   {TG_WMM_AC_VI, TG_WMM_AC_BE, TG_WMM_AC_VI, TG_WMM_AC_VI, 0}, // B.K B_K
   {TG_WMM_AC_VO, TG_WMM_AC_VI, TG_WMM_AC_BE, TG_WMM_AC_BK, 0}, // M.B M_B - 4 exchanges: 1 uplink, 0 downlink
   {TG_WMM_AC_VI, TG_WMM_AC_BE, TG_WMM_AC_VI, TG_WMM_AC_VI, 0}, // M.K M_K
   {TG_WMM_AC_VI, TG_WMM_AC_BE, TG_WMM_AC_VI, TG_WMM_AC_VI, 0} //  M.W M_W   special exchange for Broadcast testing
};
/* Generic function to create a meassage, it also fills in the AC as part of
** the payload
** */
void create_apts_msg(int msg, unsigned int txbuf[],int id)
{
	struct apts_msg *t;

	t = &apts_msgs[msg];
	txbuf[ 0] = wmmps_info.my_cookie;
	txbuf[ 1] = wmmps_info.dscp;
	txbuf[ 2] = 0;
	txbuf[ 3] = 0;
	txbuf[ 4] = 0;
	txbuf[ 5] = 0; 
	//txbuf[ 6] = t->param0; 
	//txbuf[ 7] = t->param1; 
	//txbuf[ 8] = t->param2; 
	txbuf[ 9] = id;
	txbuf[ 10] = t->cmd; 
	strcpy((char *)&txbuf[11], t->name);
	 PRINTF("create_apts_msg (%s) %d\n", t->name,t->cmd);
}

void print_hex_string(char* buf, int len)
{
    int i;

    if (len==0) { printf("<empty string>"); return; }

    for (i = 0; i < len; i++) {
        printf("%02x ", *((unsigned char *)buf + i));
	if ((i&0xf)==15) printf("\n   ");
    }
    if ((i&0xf))
	printf("\n");
}
/* trace print*/
void mpx(char *m, void *buf_v, int len)
{
    char *buf = buf_v;

    printf("%s   MSG: %s\n   ", m, &buf[44] );
    print_hex_string(buf, len);
}
/* function to validate the AC of the payload recd to ensure the correct
** message sequence*/
int receiver(unsigned int *rmsg,int length,int tos,unsigned int type)
{
  int r=1;
#ifndef WFA_WMM_AC
  int new_dscp=rmsg[1];

  if((new_dscp != tos)||(rmsg[10] != type))
  {
	  PRINTF("\r\n dscp recd is %d msg type is %d\n",new_dscp,rmsg[10]);
    r=-6;
  }
#else
  if(rmsg[10] != type)
  {
	  PRINTF("\r\n dscp recd is %d msg type is %d\n",new_dscp,rmsg[10]);
    r=-6;
  }
#endif

  return r;
}
/* WfaRcvProc: This function receives the test case name
** after sending the initial hello packet, on receiving a
** valid test case it advances onto the next receive state
*/
int WfaRcvProc(unsigned int *rmsg,int length,int *state)
{

    int sta_test;
    int usedThread = wmmps_info.ps_thread;
    num_hello=0;
    sta_test = wmmps_info.sta_test = rmsg[10];
    mpx("STA recv\n", rmsg, 64);
    if(!((sta_test >=B_D)&&(sta_test <= LAST_TEST)))
	  return -1;
    wmmps_info.my_sta_id = rmsg[9];
    pthread_mutex_lock(&wmm_thr[usedThread].thr_flag_mutex);
    wmm_thr[usedThread].thr_flag = wmmps_info.streamid;
    pthread_mutex_unlock(&wmm_thr[usedThread].thr_flag_mutex);
    (*state)++;

    return 0;
}
/* WfaStaResetAll: This function resets the whole communication with
** the console (in the event of a wrong message received for the test case)
** resulting into resending of all the packets from the scratch, there is an
** upper bound for the resets a max of three*/
void WfaStaResetAll()
{
    int r;
    PRINTF("Entering Reset\n");
    num_retry++;
    if(num_retry > MAXRETRY)
    {
        create_apts_msg(APTS_RESET_STOP, psTxMsg,wmmps_info.my_sta_id);
        wfaTGSetPrio(psSockfd, TG_WMM_AC_BE);
        r = sendto(psSockfd, psTxMsg, msgsize, 0, (struct sockaddr *)&wmmps_info.psToAddr, sizeof(struct sockaddr));
        mpx("STA msg",psTxMsg,64);
        printf("Too many retries\n");
	exit(-8);
    }
    if(!reset_recd)
    {
        create_apts_msg(APTS_RESET, psTxMsg,wmmps_info.my_sta_id);
        wfaTGSetPrio(psSockfd, TG_WMM_AC_BE);
        psTxMsg[1] = TOS_BE;
        r = sendto(psSockfd, psTxMsg, msgsize, 0, (struct sockaddr *)&wmmps_info.psToAddr, sizeof(struct sockaddr));
        mpx("STA msg",psTxMsg,64);
    }
    else
    {
        create_apts_msg(APTS_RESET_RESP, psTxMsg,wmmps_info.my_sta_id);
        wfaTGSetPrio(psSockfd, TG_WMM_AC_BE);
        r = sendto(psSockfd, psTxMsg, msgsize, 0, (struct sockaddr *)&wmmps_info.psToAddr, sizeof(struct sockaddr));
        mpx("STA msg",psTxMsg,64);
        reset_recd=0;
    }

    resetsnd=1;
    resetrcv=1;
}
/* WfaRcvVO: A function expected to receive a AC_VO packet from
** the console, if does not reeive a valid VO resets the communication wit
** h the console*/

int WfaRcvVO(unsigned int *rmsg,int length,int *state)
{

    int r;

    if ((r=receiver(rmsg,length,TOS_VO,APTS_DEFAULT))>=0)
        (*state)++;
    else
    {
        PRINTF("\nBAD REC in VO%d\n",r);
        WfaStaResetAll();
    }

    return 0;
}
/* WfaRcvVI: A function expected to receive a AC_VI packet from
** the console, if does not reeive a valid VI resets the communication wit
** h the console*/

int WfaRcvVI(unsigned int *rmsg,int length,int *state)
{

    int r;

    if ((r=receiver(rmsg,length,TOS_VI,APTS_DEFAULT))>=0)
        (*state)++;
    else
        PRINTF("\nBAD REC in VI%d\n",r);

    return 0;
}

/* WfaRcvBE: A function expected to receive a AC_BE packet from
** the console, if does not reeive a valid BE resets the communication wit
** h the console*/

int WfaRcvBE(unsigned int *rmsg,int length,int *state)
{

    int r;
    if ((r=receiver(rmsg,length,TOS_BE,APTS_DEFAULT))>=0)
        (*state)++;
    else
    {
        PRINTF("\nBAD REC in BE%d\n",r);
    }

    return 0;
}

/* WfaRcvBK: A function expected to receive a AC_BK packet from
** the console, if does not reeive a valid BK resets the communication wit
** h the console*/

int WfaRcvBK(unsigned int *rmsg,int length,int *state)
{

    int r;

    if ((r=receiver(rmsg,length,TOS_BK,APTS_DEFAULT))>=0)
        (*state)++;
    else
        PRINTF("\nBAD REC in BK%d\n",r);

    return 0;
}
/* WfaRcvVOCyclic: This is meant for the L1 test case. The function
** receives the VO packets from the console */
int WfaRcvVOCyclic(unsigned int *rmsg,int length,int *state)
{

    int r;
    tgWMM_t *my_wmm = &wmm_thr[wmmps_info.ps_thread];

    if(rmsg[10] != APTS_STOP)
    {
        if ((r=receiver(rmsg,length,TOS_VO,APTS_DEFAULT))>=0)
            ;
        else
            PRINTF("\nBAD REC in VO%d\n",r);
    }
    else
    {

        pthread_mutex_lock(&my_wmm->thr_stop_mutex);
        while(!my_wmm->stop_flag)
        {
            pthread_cond_wait(&my_wmm->thr_stop_cond, &my_wmm->thr_stop_mutex);
        }
        pthread_mutex_unlock(&my_wmm->thr_stop_mutex);
        my_wmm->stop_flag = 0;
        gtgWmmPS = 0;
        close(psSockfd);
        psSockfd = -1;
        signal(SIGALRM, SIG_IGN);
        //wfaSetDUTPwrMgmt(PS_OFF);
        sleep(1);
    }

    return 0;
}
/* WfaRcvStop: This function receives the stop message from the
** console, it waits for the sending thread to have sent the stop before
** quitting*/
int WfaRcvStop(unsigned int *rmsg,int length,int *state)
{

  tgWMM_t *my_wmm = &wmm_thr[wmmps_info.ps_thread];

    my_wmm->stop_flag = 0;
  PRINTF("\r\nEnterring Wfarcvstop\n");
  if(rmsg[10] != APTS_STOP)
  {
		PRINTF("\nBAD REC in rcvstop%d\n",r);
		//WfaStaResetAll();
  }
  else
  {
    pthread_mutex_lock(&my_wmm->thr_stop_mutex);
    while(!my_wmm->stop_flag)
    {
        pthread_cond_wait(&my_wmm->thr_stop_cond, &my_wmm->thr_stop_mutex);
    }
    num_stops=0;
    pthread_mutex_unlock(&my_wmm->thr_stop_mutex);
    my_wmm->stop_flag = 0;
    gtgWmmPS = 0;
    close(psSockfd);
    psSockfd = -1;
    signal(SIGALRM, SIG_IGN);
    //wfaSetDUTPwrMgmt(PS_OFF);
    sleep(1);

  }
  return 0;
}
void BUILD_APTS_MSG(int msg, unsigned long *txbuf)
{   
    struct apts_msg *t;

    t = &apts_msgs[msg];
    txbuf[0] = wmmps_info.msgno++;
    txbuf[1] = 0;
    txbuf[2] = 0;
    txbuf[3] = 0;
    txbuf[4] = 0;
    txbuf[5] = 0;
    txbuf[6] = t->param0;
    txbuf[7] = t->param1;
    txbuf[8] = t->param2;
    txbuf[9] = t->param3;
    txbuf[10] = t->cmd;
    strcpy((char *)&txbuf[11], t->name); 
}

void send_txmsg(int new_prio_class)
{
    int r;
    int new_dscp = 0;

    if(new_prio_class > -1)
       new_dscp = wfaTGSetPrio(psSockfd, new_prio_class);

    psTxMsg[0] = wmmps_info.msgno++;
    psTxMsg[1] = new_dscp;
    psTxMsg[2] = wmmps_info.my_group_cookie;
    psTxMsg[3] = wmmps_info.my_cookie;
    psTxMsg[4] = wmmps_info.my_sta_id;

    if(psTxMsg[10] == APTS_DEFAULT)
    {
        psTxMsg[13] = (wmmps_info.msgno%10) + 0x20202030; 
    }

    r = wfaTrafficSendTo(psSockfd, (char *)psTxMsg, 200+(wmmps_info.msgno%200), (struct sockaddr *) &wmmps_info.psToAddr);

    wmmps_info.nsent++;
}

/*
 * This needs to adopt to the specific platform you port to.
 */
void wfaSetDUTPwrMgmt(int mode)
{
   static int curr_mode = -1;
   char iface[32];

   strncpy(iface,WFA_STAUT_IF,31);

   if(curr_mode == mode)
   {
       return;
   }

#if !defined(_CYGWIN) && !defined(_WINDOWS)
   if(mode == PS_OFF)
   {
       sprintf(gCmdStr, "iwpriv %s set PSMode=CAM", iface);
       if( system(gCmdStr) < 0)
       {
          DPRINT_ERR(WFA_ERR, "Cant Set PS OFF\n");
       }
       else
         printf("\r\n STA PS OFF \n");
   }
   else
   {
       sprintf(gCmdStr, "iwpriv %s set PSMode=MAX_PSP", iface);
       if( system(gCmdStr) < 0)
       {
          DPRINT_ERR(WFA_ERR, "Cant Set PS ON\n");
       }
       else
         printf("\r\n STA PS ON \n");
   }

   curr_mode = mode;
#endif
}

int wfaWmmPowerSaveProcess(int sockfd)
{
   int rbytes = 0;
   int sta_test;
   struct sockaddr from;
   int len;
   StationRecvProcStatetbl_t  *rcvstatarray;
   StationRecvProcStatetbl_t  func;
   int *rcv_state;
   len=sizeof(from);

   rbytes = recvfrom(sockfd, (char *)psRxMsg, MAX_UDP_LEN, 0, &from, (socklen_t *)&len);
   if(rbytes < 0)
   {
      perror("receive error");
      return rbytes;
   }
        sta_test = wmmps_info.sta_test;
        if(sta_test != L_1)
	  mpx("RX msg",psRxMsg,64);
        if(psRxMsg[10] == APTS_STOP)
          PRINTF("\r\n stop recd\n");
	if(psRxMsg[10] == APTS_RESET)
	   {
		reset_recd=1;
		WfaStaResetAll();
		return 0;
	   }
   //If reset signal is there for the receiving thread and station has sent the
   //reset message (i.e. !reset_recd) then ignore all the messages till an
   //APTS_RESET_RESP has been received.

   if(resetrcv)
	   {
      wmmps_info.rcv_state = 0;
      if((!reset_recd)&&(psRxMsg[10] != APTS_RESET_RESP))
        return 0;
      else
	   {
        resetrcv = 0;
        reset_recd = 0;
       }
   }
   sta_test = wmmps_info.sta_test;
       wmmps_info.my_cookie = psRxMsg[0];
   rcv_state = &(wmmps_info.rcv_state);
   rcvstatarray = stationRecvProcStatetbl[sta_test];
   func = rcvstatarray[*(rcv_state)];
   func.statefunc(psRxMsg,rbytes,rcv_state);


   return TRUE;
}

#endif /* WFA_WMM_PS_EXT */
#endif /* WFA_WMM_EXT    */
