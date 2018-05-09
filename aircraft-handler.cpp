#
/*
 *    Copyright (C) 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the qt-1090
 *
 *    qt-1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    qt-1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with qt-1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *      qt-1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 */
#include	"aircraft-handler.h"
#include	"adsb-constants.h"
#include	"message-handling.h"
#include	"qt-1090.h"

/* Always positive MOD operation, used for CPR decoding. */
static inline
int cprModFunction (int a, int b) {
int res = a % b;
	return res < 0 ? res + b : res;
}

/* The NL function uses the precomputed table from 1090-WP-9-14 */
static
int	cprNLFunction(double lat) {
	if (lat < 0) lat = -lat; /* Table is symmetric about the equator. */
	if (lat < 10.47047130) return 59;
	if (lat < 14.82817437) return 58;
	if (lat < 18.18626357) return 57;
	if (lat < 21.02939493) return 56;
	if (lat < 23.54504487) return 55;
	if (lat < 25.82924707) return 54;
	if (lat < 27.93898710) return 53;
	if (lat < 29.91135686) return 52;
	if (lat < 31.77209708) return 51;
	if (lat < 33.53993436) return 50;
	if (lat < 35.22899598) return 49;
	if (lat < 36.85025108) return 48;
	if (lat < 38.41241892) return 47;
	if (lat < 39.92256684) return 46;
	if (lat < 41.38651832) return 45;
	if (lat < 42.80914012) return 44;
	if (lat < 44.19454951) return 43;
	if (lat < 45.54626723) return 42;
	if (lat < 46.86733252) return 41;
	if (lat < 48.16039128) return 40;
	if (lat < 49.42776439) return 39;
	if (lat < 50.67150166) return 38;
	if (lat < 51.89342469) return 37;
	if (lat < 53.09516153) return 36;
	if (lat < 54.27817472) return 35;
	if (lat < 55.44378444) return 34;
	if (lat < 56.59318756) return 33;
	if (lat < 57.72747354) return 32;
	if (lat < 58.84763776) return 31;
	if (lat < 59.95459277) return 30;
	if (lat < 61.04917774) return 29;
	if (lat < 62.13216659) return 28;
	if (lat < 63.20427479) return 27;
	if (lat < 64.26616523) return 26;
	if (lat < 65.31845310) return 25;
	if (lat < 66.36171008) return 24;
	if (lat < 67.39646774) return 23;
	if (lat < 68.42322022) return 22;
	if (lat < 69.44242631) return 21;
	if (lat < 70.45451075) return 20;
	if (lat < 71.45986473) return 19;
	if (lat < 72.45884545) return 18;
	if (lat < 73.45177442) return 17;
	if (lat < 74.43893416) return 16;
	if (lat < 75.42056257) return 15;
	if (lat < 76.39684391) return 14;
	if (lat < 77.36789461) return 13;
	if (lat < 78.33374083) return 12;
	if (lat < 79.29428225) return 11;
	if (lat < 80.24923213) return 10;
	if (lat < 81.19801349) return 9;
	if (lat < 82.13956981) return 8;
	if (lat < 83.07199445) return 7;
	if (lat < 83.99173563) return 6;
	if (lat < 84.89166191) return 5;
	if (lat < 85.75541621) return 4;
	if (lat < 86.53536998) return 3;
	if (lat < 87.00000000) return 2;
	else
	   return 1;
}

static
int cprNFunction (double lat, int isodd) {
int nl = cprNLFunction (lat) - isodd;
    return (nl < 1) ? 1 : nl;
}

static
double cprDlonFunction (double lat, int isodd) {
    return 360.0 / cprNFunction (lat, isodd);
}

/* This algorithm comes from:
 * http://www.lll.lu/~edward/edward/adsb/DecodingADSBposition.html.
 *
 *
 * A few remarks:
 * 1) 131072 is 2^17 since CPR latitude and longitude are encoded in 17 bits.
 * 2) We assume that we always received the odd packet as last packet for
 *    simplicity. This may provide a position that is less fresh of a few
 *    seconds.
 */
void	decodeCPR (aircraft *a) {
const double AirDlat0 = 360.0 / 60;
const double AirDlat1 = 360.0 / 59;
double lat0 = a -> even_cprlat;
double lat1 = a -> odd_cprlat;
double lon0 = a -> even_cprlon;
double lon1 = a -> odd_cprlon;

/* Compute the Latitude Index "j" */
	int j = floor(((59*lat0 - 60*lat1) / 131072) + 0.5);
	double rlat0 = AirDlat0 * (cprModFunction(j,60) + lat0 / 131072);
	double rlat1 = AirDlat1 * (cprModFunction(j,59) + lat1 / 131072);

	if (rlat0 >= 270)
	   rlat0 -= 360;
	if (rlat1 >= 270)
	   rlat1 -= 360;

/* Check that both are in the same latitude zone, or abort. */
	if (cprNLFunction(rlat0) != cprNLFunction(rlat1)) return;

/* Compute ni and the longitude index m */
	if (a -> even_cprtime > a -> odd_cprtime) {
/* Use even packet. */
	   int ni = cprNFunction (rlat0,0);
	   int m = floor((((lon0 * (cprNLFunction(rlat0)-1)) -
                        (lon1 * cprNLFunction(rlat0))) / 131072) + 0.5);
	   a -> lon = cprDlonFunction(rlat0,0) *
	                      (cprModFunction(m,ni)+lon0/131072);
	   a -> lat = rlat0;
	}
	else {		/* Use odd packet. */
	   int ni = cprNFunction(rlat1,1);
	   int m = floor((((lon0 * (cprNLFunction(rlat1)-1)) -
	                   (lon1 * cprNLFunction(rlat1))) / 131072.0) + 0.5);
	   a -> lon = cprDlonFunction (rlat1,1) *
	                           (cprModFunction (m, ni) + lon1 / 131072);
	   a -> lat = rlat1;
	}

	if (a -> lon > 180)
	   a->lon -= 360;
}

/*
 *	Return a new aircraft structure for the interactive mode linked list
 *	of aircrafts.
 */
	aircraft::aircraft (uint32_t addr) {
	this -> addr	= addr;
	snprintf (hexaddr, sizeof (hexaddr),"%06x",(int)addr);
	flight [0]	= '\0';
	altitude	= 0;
	speed		= 0;
	track		= 0;
	odd_cprlat 	= 0;
	odd_cprlon 	= 0;
	odd_cprtime 	= 0;
	even_cprlat 	= 0;
	even_cprlon 	= 0;
	even_cprtime 	= 0;
	lat		= 0;
	lon		= 0;
	seen		= time (NULL);
	messages	= 0;
	next		= NULL;
}

	aircraft::~aircraft (void) {}

/*
 *	Return the aircraft with the specified address, or NULL if no aircraft
 *	exists with this address.
 */
aircraft *findAircraft (aircraft *a, uint32_t addr) {
	while (a != NULL) {
	   if (a -> addr == addr)
	      return a;
	   a = a -> next;
	}
	return NULL;
}

/* Receive new messages and populate the interactive mode with more info. */
aircraft *interactiveReceiveData (qt1090 *st, message *mm) {
uint32_t addr;
aircraft *a, *aux;

	if (st -> check_crc && mm -> is_crcok () == 0)
	   return NULL;
	addr	= mm -> getAddr ();

//	Lookup our aircraft or create a new one. */
	a = findAircraft (st -> aircrafts, addr);
	if (a == NULL) {
	   a = new aircraft (addr);
	   a -> next = st -> aircrafts;
	   st -> aircrafts = a;
	} else {
        /* If it is an already known aircraft, move it on head
         * so we keep aircrafts ordered by received message time.
         * However move it on head only if at least one second elapsed
         * since the aircraft that is currently on head sent a message,
         * othewise with multiple aircrafts at the same time we have an
         * useless shuffle of positions on the screen. */
	   if (0 && st -> aircrafts != a && (time(NULL) - a -> seen) >= 1) {
	      aux = st -> aircrafts;
	      while (aux -> next != a)
	         aux = aux -> next;
//	Now we are a node before the aircraft to remove. */
	      aux -> next = aux -> next -> next; /* removed. */
//	Add on head */
	      a -> next = st -> aircrafts;
	      st -> aircrafts = a;
	   }
	}

	a -> fillData (mm);
	return a;
}

void	aircraft::fillData (message *mm) {
	this -> seen = time(NULL);
	this ->  messages++;

	if (mm -> msgtype == 0 ||
	    mm -> msgtype == 4 || mm -> msgtype == 20) {
	   this -> altitude = mm -> altitude;
	}
	else
	if (mm -> msgtype == 17) {
	   if (mm -> metype >= 1 && mm -> metype <= 4) {
	      memcpy (this -> flight, mm -> flight, sizeof (this -> flight));
	   }
	   else
	   if (mm -> metype >= 9 && mm -> metype <= 18) {
	      this -> altitude = mm -> altitude;
	      if (mm -> fflag) {
	         this -> odd_cprlat = mm -> raw_latitude;
	         this -> odd_cprlon = mm -> raw_longitude;
	         this -> odd_cprtime = mstime ();
	      }
	      else {
	         this -> even_cprlat = mm -> raw_latitude;
	         this -> even_cprlon = mm -> raw_longitude;
	         this -> even_cprtime = mstime ();
	      }
//	If the two data is less than 10 seconds apart, compute
//	the position.
	      if (abs (this -> even_cprtime - this -> odd_cprtime) <= 10000) {
	         decodeCPR (this);
	      }
	   }	
	   else
	   if (mm -> metype == 19) {
	      if (mm -> mesub == 1 || mm -> mesub == 2) {
	         this -> speed = mm -> velocity;
	         this -> track = mm -> heading;
	      }
	   }
	}
}

/* When in interactive mode If we don't receive new nessages within
 * MODES_INTERACTIVE_TTL seconds we remove the aircraft from the list. */
void	interactiveRemoveStaleAircrafts (qt1090 *st) {
aircraft *a = st -> aircrafts;
aircraft *prev = NULL;
time_t now = time(NULL);

	while (a != NULL) {
	   if ((now - a -> seen) > st -> interactive_ttl) {
            aircraft *next = a->next;
//	Remove the element from the linked list, be careful
//	when removing the first element. */
            delete a;
            if (!prev)
                st -> aircrafts = next;
            else
                prev -> next = next;
            a = next;
        } else {
            prev = a;
            a = a -> next;
        }
    }
}

void	showPlanes (aircraft *aircrafts, int rows, bool metric) {
int amount	= rows;
time_t now	= time (NULL);
char progress [4];
int count = 0;

	memset (progress, ' ', 3);
	progress [time (NULL)%3] = '.';
	progress [3] = '\0';

	printf ("\x1b[H\x1b[2J");    /* Clear the screen */
	printf (
	  "Hex    Flight   Altitude  Speed   Lat       Lon       Track  Messages Seen %s\n"
"--------------------------------------------------------------------------------\n",
        progress);

	while (aircrafts != NULL && count < amount) {
	   aircraft *a = aircrafts;
	   int altitude	= a -> altitude;
	   int speed	= a -> speed;

//	Convert units to metric if --metric was specified. */
           if (metric) {
              altitude /= 3.2828;
              speed *= 1.852;
	   }

	   printf ("%-6s %-8s %-9d %-7d %-7.03f   %-7.03f   %-3d   %-9ld %d sec\n",
	           a -> hexaddr, a -> flight, altitude, speed,
	           a -> lat, a -> lon, a -> track, a -> messages,
	           (int)(now - a -> seen));
	   aircrafts = aircrafts -> next;
	   count++;
	}
}
