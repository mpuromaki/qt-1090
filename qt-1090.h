#
/*
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the qt-1090 program
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
 *      qt-1090 is based on and contains source code from the dump1090 program 
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 */

#ifndef	__QT_1090__
#define	__QT_1090__

#include	"adsb-constants.h"
#include	<QMainWindow>
#include	<QObject>
#include	<QSettings>
#include	<QMessageBox>
#include	<QCloseEvent>
#include	<QTableWidget>
#include	"message-handling.h"
#include	"ui_qt-1090.h"

class	deviceHandler;
class	syncViewer;

/* Structure used to describe a networking client. */
struct client {
        int fd;                 /* File descriptor. */
        int service;            /* TCP port the client is connected to. */
        char buf [CLIENT_BUF_SIZE+1];    /* Read buffer. */
        int buflen;             /* Amount of data on buffer. */
};


class	qt1090: public QMainWindow, private Ui_mainwindow {
Q_OBJECT
public:
	qt1090		(QSettings *, deviceHandler *);
	~qt1090		(void);
	char		*aircraftsToJson (int *len);
private:
	void		finalize	(void);
	void		closeEvent	(QCloseEvent *event);
	int		decodeBits	(uint8_t *bits, uint16_t *m);
	void		detectModeS	(uint16_t *m, uint32_t mlen);
	void		useModesMessage (message *mm);
	void		serverInit	(void);
	void		AcceptClients	(void);
	void		FreeClient	(int fd);
	void		ReadFromClient	(struct client *c,
                                       char *sep,
                                       int(*handler)(void *, struct client *));
	void		ReadFromClients (void);
	void		update_view (uint16_t *m, bool);
	void		update_table (int16_t, int);
	int		table [32];
public slots:
	void		processData (void);
private:
	pthread_t	reader_thread;
	deviceHandler	*device;
	QSettings	*dumpSettings;
	uint16_t	*magnitudeVector;
	uint32_t	data_len;	/* Buffer length. */

	syncViewer	*viewer;
//	Networking */
	char		aneterr		[ANET_ERR_LEN];
	struct client	*clients [MODES_NET_MAX_FD]; /* Our clients. */
	int		maxfd;		/* Greatest fd currently active. */
	int		sbsos;          /* SBS output listening socket. */
        int		ros;            /* Raw output listening socket. */
        int		ris;            /* Raw input listening socket. */
        int		https;          /* HTTP listening socket. */

//	Configuration */
	bool		net;		/* Enable networking. */
	bool		interactive;	/* Interactive mode */
	int		interactive_rows;  /* Interactive mode: max rows. */
	bool		stats;		/* Print stats at exit in --ifile mode. */
	bool		metric;		/* Use metric units. */
	double		avg_corr;
	int		correlationCounter;
	QTableWidget	*tableWidget;
	bool		show_preambles;
public:
	icaoCache	*icao_cache;
	bool		check_crc;	/* Only display messages with good CRC. */
	int		handle_errors;	/* Level of error correction	*/
	int		interactive_ttl;   /* Interactive mode: TTL before deletion. */
	int		debug;		/* Debugging mode. */

//	Interactive mode */
	aircraft	*aircrafts;
	long long interactive_last_update;  /* Last screen update in milliseconds */

//	Statistics */
public:
	long long stat_valid_preamble;
	long long stat_demodulated;
	long long stat_goodcrc;
	long long stat_badcrc;
	long long stat_fixed;
	long long stat_single_bit_fix;
	long long stat_two_bits_fix;
	long long stat_http_requests;
	long long stat_sbs_connections;
	long long stat_out_of_phase;
	long long stat_phase_corrected;
private slots:
	void	handle_interactiveButton (void);
	void	handle_errorhandlingCombo	(const QString &);
	void	handle_httpButton	(void);
	void	set_ttl			(int);
	void	handle_show_preamblesButton (void);
	void	handle_metricButton	(void);
};

#endif

