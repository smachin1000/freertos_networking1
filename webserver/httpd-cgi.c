/**
 * \addtogroup httpd
 * @{
 */

/**
 * \file
 *         Web server script interface
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Copyright (c) 2001-2006, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: httpd-cgi.c,v 1.2 2006/06/11 21:46:37 adam Exp $
 *
 */

#include "uip.h"
#include "psock.h"
#include "httpd.h"
#include "httpd-cgi.h"
#include "httpd-fs.h"
#include "../drivers/mss_ace/mss_ace.h"
#include "../CMSIS/a2fxxxm3.h"

#include <stdio.h>
#include <string.h>

HTTPD_CGI_CALL(file, "file-stats", file_stats);
HTTPD_CGI_CALL(tcp, "tcp-connections", tcp_stats);
HTTPD_CGI_CALL(net, "net-stats", net_stats);
HTTPD_CGI_CALL(mul, "mul-stats", mul_stats);
HTTPD_CGI_CALL(realtime_data, "realtime-data", realtime_data1);

HTTPD_CGI_CALL(oled_data, "oled-data", oled_data1);

float                     real_voltage_value;
float                     real_current_value;
float                     real_temperature_value_tc;
float                     real_temperature_value_tk;
float                     real_temperature_value_tf;

static const struct httpd_cgi_call *calls[] = { &file, &tcp, &net, &mul, &realtime_data, &oled_data, NULL };
/*--------------------------------------------------------------------------------------------------------------*/
//ACE FUNCTIONS
/*--------------------------------------------------------------------------------------------------------------*/
/****************************************************************/
// voltage test
/****************************************************************/

/*---------------------------------------------------------------------------*/
static
PT_THREAD(nullfunction(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_cgifunction
httpd_cgi(char *name)
{
  const struct httpd_cgi_call **f;

  /* Find the matching name in the table, return the function. */
  for(f = calls; *f != NULL; ++f) {
    if(strncmp((*f)->name, name, strlen((*f)->name)) == 0) {
      return (*f)->function;
    }
  }
  return nullfunction;
}
/*---------------------------------------------------------------------------*/
static unsigned short
generate_file_stats(void *arg)
{
  char *f = (char *)arg;
  return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE, "%5u", httpd_fs_count(f));
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(file_stats(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

  PSOCK_GENERATOR_SEND(&s->sout, generate_file_stats, strchr(ptr, ' ') + 1);
  
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static const char closed[] =   /*  "CLOSED",*/
{0x43, 0x4c, 0x4f, 0x53, 0x45, 0x44, 0};
static const char syn_rcvd[] = /*  "SYN-RCVD",*/
{0x53, 0x59, 0x4e, 0x2d, 0x52, 0x43, 0x56,
 0x44,  0};
static const char syn_sent[] = /*  "SYN-SENT",*/
{0x53, 0x59, 0x4e, 0x2d, 0x53, 0x45, 0x4e,
 0x54,  0};
static const char established[] = /*  "ESTABLISHED",*/
{0x45, 0x53, 0x54, 0x41, 0x42, 0x4c, 0x49, 0x53, 0x48,
 0x45, 0x44, 0};
static const char fin_wait_1[] = /*  "FIN-WAIT-1",*/
{0x46, 0x49, 0x4e, 0x2d, 0x57, 0x41, 0x49,
 0x54, 0x2d, 0x31, 0};
static const char fin_wait_2[] = /*  "FIN-WAIT-2",*/
{0x46, 0x49, 0x4e, 0x2d, 0x57, 0x41, 0x49,
 0x54, 0x2d, 0x32, 0};
static const char closing[] = /*  "CLOSING",*/
{0x43, 0x4c, 0x4f, 0x53, 0x49,
 0x4e, 0x47, 0};
static const char time_wait[] = /*  "TIME-WAIT,"*/
{0x54, 0x49, 0x4d, 0x45, 0x2d, 0x57, 0x41,
 0x49, 0x54, 0};
static const char last_ack[] = /*  "LAST-ACK"*/
{0x4c, 0x41, 0x53, 0x54, 0x2d, 0x41, 0x43,
 0x4b, 0};

static const char *states[] = {
  closed,
  syn_rcvd,
  syn_sent,
  established,
  fin_wait_1,
  fin_wait_2,
  closing,
  time_wait,
  last_ack};
  

static unsigned short
generate_tcp_stats(void *arg)
{
  struct uip_conn *conn;
  struct httpd_state *s = (struct httpd_state *)arg;
    
  conn = &uip_conns[s->count];
  return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE,
		 "<tr><td>%d</td><td>%u.%u.%u.%u:%u</td><td>%s</td><td>%u</td><td>%u</td><td>%c %c</td></tr>\r\n",
		 htons(conn->lport),
		 htons(conn->ripaddr[0]) >> 8,
		 htons(conn->ripaddr[0]) & 0xff,
		 htons(conn->ripaddr[1]) >> 8,
		 htons(conn->ripaddr[1]) & 0xff,
		 htons(conn->rport),
		 states[conn->tcpstateflags & UIP_TS_MASK],
		 conn->nrtx,
		 conn->timer,
		 (uip_outstanding(conn))? '*':' ',
		 (uip_stopped(conn))? '!':' ');
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(tcp_stats(struct httpd_state *s, char *ptr))
{
  
  PSOCK_BEGIN(&s->sout);

  for(s->count = 0; s->count < UIP_CONNS; ++s->count) {
    if((uip_conns[s->count].tcpstateflags & UIP_TS_MASK) != UIP_CLOSED) {
      PSOCK_GENERATOR_SEND(&s->sout, generate_tcp_stats, s);
    }
  }

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static unsigned short
generate_net_stats(void *arg)
{
  struct httpd_state *s = (struct httpd_state *)arg;
  return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE,
		  "%5u\n", ((uip_stats_t *)&uip_stat)[s->count]);
}

static
PT_THREAD(net_stats(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

#if UIP_STATISTICS

  for(s->count = 0; s->count < sizeof(uip_stat) / sizeof(uip_stats_t);
      ++s->count) {
    PSOCK_GENERATOR_SEND(&s->sout, generate_net_stats, s);
  }
  
#endif /* UIP_STATISTICS */
  
  PSOCK_END(&s->sout);
}

extern int dataFromMain;

extern float dataFromMainf1;
extern float dataFromMainf2;
extern int dataFromMaind1;
extern int dataFromMaind2;
extern int my_ip[4];
/*---------------------------------------------------------------------------*/
static unsigned short
generate_mul_stats(void *arg)
{
	
		return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE,    


				"<html><head><title>Smartfusion Embedded Webserver</title></head>\n"
		"<table width=\"800%\" cellpadding=\"1\" cellspacing=\"1\" align=\"center\" class=\"tbl_text\">"
		"<tr><td colspan=\"2\" align=\"center\" <img src=\"/b?\">"
		"</td></tr>"
		"<tr><td colspan=\"2\" align=\"center\"><b>Multimeter Mode</b>"
		"</td></tr></table>"
		"<table width=\"400%\" border=\"1px\" align=\"center\" class=\"tbl_text\">"
		"<tr><td width=\"50%\" align=\"center\"><font size=\"-1\"><b>Channel</b></font></td>"
		"<td align=\"center\"><font size=\"-1\"><b>Value</b></font></td>"  
		"</td></tr>"         
		"<tr><td>Potentiometer Voltage</td><td align=\"center\">"
		" %.2f V" 
		"</td></tr>"
		"<tr><td>Potentiometer Current </td><td align=\"center\">"
		" %.2f uA"
		"</td></tr>" 	 
				  "<TR>"
				   " <TD>External Temperature in Celcius</TD>"
				    "<TD align=middle>"
				    "%.2f °C"
					  "</TD></TR>"
				  "<TR>"
				   " <TD>External Temperature in Fahrenheit</TD>"
				    "<TD align=middle>"
				    "%.2f F"
				  "</TD></TR>"	
				  "<TR>"
				   " <TD>External Temperature in Kelvin</TD>"
				   "<TD align=middle>"
				   "%.2f K"
		"</td></tr></table>"
		"<table width=\"40%\" cellpadding=\"1\" cellspacing=\"1\" align=\"center\">"
		"<form>"
				"<input type = \"Button\" value = \"Home\" onclick = \"window.location.href='index.html'\">"
				"</form>"
		"</table></form>\n" 
		"</body></html>\n", real_voltage_value, real_current_value, real_temperature_value_tc,real_temperature_value_tf,real_temperature_value_tk);

			
			
}



static
PT_THREAD(mul_stats(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

//  for(s->count = 0; s->count < sizeof(uip_stat) / sizeof(uip_stats_t);
//      ++s->count) {
    PSOCK_GENERATOR_SEND(&s->sout, generate_mul_stats, s);
//  }
  
  PSOCK_END(&s->sout);
}



static unsigned short
generate_realtime_data(void *arg)
{	
return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE,    
#if 1		

		"<HTML><HEAD><TITLE>Actel SmartFusion Webserver</TITLE>"
"<META http-equiv=Content-Type content=\"text/html; charset=windows-1252\">"
"<META http-equiv=Refresh content=2>"
"<BODY>"
"<FORM action=realdata.shtm method=get>"
"<TABLE class=tbl_text cellSpacing=1 cellPadding=1 width=\"800%\" align=center>"
"<TBODY>"
"  <TR>"
"    <TD align=middle colSpan=2 td <></TD>"
"  <TR>"
"    <TD align=middle colSpan=2><B>Real Time Data"
"  Display<B></B></B></TD></TR></TBODY></TABLE>"
"<TABLE class=tbl_text width=\"400%\" align=center border=1>"
  "<TBODY>"
  "<TR>"
    "<TD align=middle width=\"70%\"><FONT"
    "size=-1><B>Channel/Quantity</B></FONT></TD>"
    "<TD align=middle><FONT size=-1><B>Value</B></FONT></TD></TD></TR>"
  "<TR>"
   " <TD>Potentiometer Voltage</TD>"
    "<TD align=middle>"
		"%.2f V"
		"</TD></TR>"
  "<TR>"
   " <TD>Potentiometer Current</TD>"
    "<TD align=middle>"
		"%.2f uA"
		"</TD></TR>"
  "<TR>"
   " <TD>External Temperature in Celcius</TD>"
    "<TD align=middle>"
    "%.2f °C"
	  "</TD></TR>"
  "<TR>"
   " <TD>External Temperature in Fahrenheit</TD>"
    "<TD align=middle>"
    "%.2f F"
  "</TD></TR>"	
  "<TR>"
   " <TD>External Temperature in Kelvin</TD>"
   "<TD align=middle>"
   "%.2f K"
   "</TD></TR></TBODY></TABLE>"
"<TABLE align=center>"
 " <TBODY>"
"<form>"
"<input type = \"Button\" value = \"Home\" onclick = \"window.location.href='index.html'\">"
"</form>"
"</table></form>\n" 
"</body></html>\n", real_voltage_value, real_current_value, real_temperature_value_tc,real_temperature_value_tf,real_temperature_value_tk);
#endif  
  
  
}

static
PT_THREAD(realtime_data1(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

    PSOCK_GENERATOR_SEND(&s->sout, generate_realtime_data, s);
  
  PSOCK_END(&s->sout);
}



static unsigned short
generate_oled_data(void *arg)
{
return snprintf((char *)uip_appdata, UIP_APPDATA_SIZE, 
		
"<META content=\"MSHTML 6.00.2900.5726\" name=GENERATOR></HEAD>"
"<BODY>"
"<FORM action=oled.shtml method=get>"
"<TABLE class=tbl_text height=\"32%\" cellSpacing=1 cellPadding=1 width=\"40%\""
"align=center>"
  "<TBODY>"
"<form name=\"input\" method=\"get\">"
"String to Display on OLED:"
"<input type=\"text\" maxlength=19 name=\" INPUTSTRING \" />"
"<input type=\"submit\" value=\"Submit\" />" 
"</form>"
"<form>"
"<input type = \"Button\" value = \"Home\" onclick = \"window.location.href='index.html'\">"
"</form>"  
"</TBODY></TABLE></FORM></BODY></HTML> \n"
);

}

static
PT_THREAD(oled_data1(struct httpd_state *s, char *ptr))
{
  PSOCK_BEGIN(&s->sout);

//  for(s->count = 0; s->count < sizeof(uip_stat) / sizeof(uip_stats_t);
//      ++s->count) {
    PSOCK_GENERATOR_SEND(&s->sout, generate_oled_data, s);
//  }

  
  PSOCK_END(&s->sout);
}


/*---------------------------------------------------------------------------*/
/** @} */
