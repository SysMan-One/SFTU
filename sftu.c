#pragma	module	SFTU    "SFTU"
#define	__MODULE__	"SFTU"

/*
**++
** Facility:
**      StarLet File Transfer Utility
**
** Abstract:
**      This is an utility to transfering VMS's files over the TCP/IP network to a remote  non-VMS hosts, 
**	a like the FTP or SFTP but with additional features: resuming broken transfers and generating FDL file
**	to help restoring RMS's attributes on VMS hosts.
**	
**
** Author:
**      Ruslan R. Laishev
**
** Creation Date: 2-JUN-2016
**
** Modification History:
**
**--
*/


/*
**
**  INCLUDE FILES
**
*/
#include	<starlet.h>
#include	<ssdef.h>
#include	<iodef.h>
#include	<netdb.h>
#include	<socket.h>
#include	<in.h>
#include	<inet.h>
#include	<tcpip$inetdef.h>
#include	<efndef.h>
#include	<descrip.h>
#include	<builtins.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ots$routines.h>
#include	<stdarg.h>
#include	<cli$routines.h>
#include	<clidef.h>
#include	<climsgdef.h>

#include	<fdl$routines.h>
#include	<fdldef.h>
#include	<rms.h>
#include	<xabfhcdef.h>

#define	__NEW_STARLET	1
#include	<iledef.h>
#include	<iosbdef.h>

#include	"macros.h"

#include	"sftudef.h"
#include	"sftu_msg.h"



/*
**
*/
$DESCRIPTOR (tmo_dsc, 	"0 00:03:45");
const	int	one = 1;
int	tmo[2], trace_flag = 1, exit_flag = 0;

/* Comand Line Interface stuff .... */
extern  void    *SFTU_CLD;

$DESCRIPTOR(p_p1,	"P1");
$DESCRIPTOR(p_p2,	"P2");
$DESCRIPTOR(p_p3,	"P3");
$DESCRIPTOR(q_ack,	"ACK");
$DESCRIPTOR(q_trace,	"TRACE");

$DESCRIPTOR(dsc_ucxdev, 	"UCX$DEVICE");
$DESCRIPTOR(dsc_tcpipdev, 	"TCPIP$DEVICE");

struct	FAB	fab, *fabp = &fab;
struct	RAB	rab, *rabp = &rab;
struct	XABFHC	xabfhc;
struct	NAM	nam;


static	void	_trace      (
                unsigned char * module, 
		unsigned char * func, 
                unsigned        line, 
                unsigned char * fao, 
                        ...
                        )
{
int     status, idx = 0;
va_list ap;
char	buf[512] = {"!%T [!AZ\\!AZ\\!UL] "}, out[4096];
struct  dsc$descriptor  buf_dsc, fao_dsc;
int     argc = 0, argl[32] = {0};

        /*
        ** Form FAO string
        */
        strcat(buf, fao);
        status = strnlen(buf, sizeof(buf));
        INIT_SDESC(fao_dsc, status, buf);
        
        /*
        ** Reorganize parameters list for $FAOL
        */
        va_start(ap, fao);
        argl[idx++] = 0;
        argl[idx++] = module;
	argl[idx++] = func;
        argl[idx++] = line;

        for (va_count(argc); idx < argc; idx++)
                argl[idx] = va_arg(ap, unsigned);

        va_end((void *) ap);

        /*
        ** Format a message, put it to SYS$OUTPUT
        */
        INIT_SDESC(buf_dsc, sizeof(out), out);
        if ( !(1 & (status = sys$faol(&fao_dsc, &buf_dsc.dsc$w_length, &buf_dsc, argl))) )
                lib$signal(status);

        lib$put_output(&buf_dsc);
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Put message to standard output device (SYS$OUTPUT).
**
**  FORMAL PARAMETERS:
**
**      msgid:
**              VMS condition code
**              variable agriments list
**
**  RETURN VALUE:
**
**      VMS condition code
**
**
**--
*/
int	_log		(
			int	msgid, 
			...
			)
{
long	status, retvalue = msgid;
va_list	args;
char	buf[1024] = {"!%D "}, msg_buf[1024], outadr[4];
struct	dsc$descriptor	opr_dsc, buf_dsc, fao_dsc;
int	argc, argl[32] = {0}, idx, flag = 15, lvl;

	/*
	** Get a message text with given msgid
	*/
	INIT_SDESC(fao_dsc, sizeof(buf) -4, &buf[4]);
	if ( !(1 & (status = sys$getmsg( msgid, &fao_dsc.dsc$w_length, &fao_dsc, flag, &outadr))) )
		lib$signal(status);
	
	/*
	** Reorganize parameters list for $FAOL
	*/
	va_start(args, msgid);

	for (idx = 1, va_count(argc); idx < argc; idx++)
                argl[idx] = va_arg(args, unsigned);

        va_end( args);

	/*
	** Format a message, put it to SYS$OUTPUT
	*/
	fao_dsc.dsc$a_pointer -=4;
	fao_dsc.dsc$w_length  +=4;
	INIT_SDESC(buf_dsc, sizeof(msg_buf), msg_buf);

	if ( !(1 & (status = sys$faol( &fao_dsc, &buf_dsc.dsc$w_length, &buf_dsc, argl))) )
		lib$signal(status);

	lib$put_output(&buf_dsc);

	return	retvalue;
}


/*	CRC32-C	*/
const static unsigned int crc32c_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106, 
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

unsigned int	crc32c (unsigned int crc, const void *buf, size_t buflen)
{
const unsigned char  *p = buf;

	crc = crc ^ ~0U;

	while (buflen--)
		crc = crc32c_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Canceling all queued and processed request due of expiration timer.
**
**  FORMAL PARAMETERS:
**
**      reqidt:		pointer to a network channel
**
**  RETURN VALUE:
**
**      None
**
**  SIDE EFFECTS:
**
**      Cancel _all_ request for the given I/O channel.
**--
*/
static	void	timer_ast	(
			short	*reqidt
			)
{
	sys$cantim(reqidt, 0);
	sys$cancel(*reqidt);
}



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**	Open a TCP-connection to given address and port.
**
**  FORMAL PARAMETERS:
**
**      chan:	channel number, longword, by reference
**	host:	sockaddr_in structure contained address and port, structure, by reference
**
**  RETURN VALUE:
**
**      VMS condition code
**
**--
*/
static	int	netio_conn	(
			short	*chan, 
	struct sockaddr_in	*host
			)
{
int	status;
static	struct 	{
	short	proto;
	char	type;
	char	domain;
	} sck_parm = {INET$C_TCP, INET_PROTYP$C_STREAM, INET$C_AF_INET};
IOSB	iosb;
struct sockaddr_in home = {0, 0, 0, 0, 0, 0};
ILE2	rhst_adrs = {sizeof(struct sockaddr_in), 0, host}, 
	lhst_adrs = {sizeof(struct sockaddr_in), 0, &home}, 
	sock_opt [] = {{sizeof(one), TCPIP$C_KEEPALIVE, &one}}, 
	options = {sizeof(sock_opt), TCPIP$C_SOCKOPT, &sock_opt};


	*chan = 0;
	if ( !host->sin_addr.s_addr && !host->sin_port )
		return	SS$_INSFARG;

	/*
	** Assign an I/O channel to BG device
	*/
	if ( !(1 & (status = sys$assign( &dsc_ucxdev, chan, 0, 0))) )
		{
		if ( status != SS$_NOSUCHDEV )
			return	status;
		if ( !(1 & (status = sys$assign( &dsc_tcpipdev, chan, 0, 0))) )
			return	status;
		}

	/*
	**
	*/
	home.sin_family = INET$C_AF_INET;

/*
	if ( !(1 & (status = sys$qiow (EFN$C_ENF, *chan, IO$_SETMODE, &iosb, 
		0, 0, &sck_parm, 0, &lhst_adrs, 0, &options, 0))) )
	if ( !(1 & (status = sys$qiow (EFN$C_ENF, *chan, IO$_SETMODE, &iosb, 
		0, 0, &sck_parm, 0, 0, 0, 0, 0))) )
*/

	if ( !(1 & (status = sys$qiow (EFN$C_ENF, *chan, IO$_SETMODE, &iosb, 
		0, 0, &sck_parm, 0, 0, 0, &options, 0))) )
		return	status;
	if ( !(iosb.iosb$w_status & 1) )
		return	iosb.iosb$w_status;

	host->sin_family = INET$C_AF_INET;
	if ( !(1 & (status = sys$qiow (EFN$C_ENF, *chan, IO$_ACCESS, &iosb, 0, 0, 0, 0, &rhst_adrs, 0, 0, 0))) )

	if ( !(iosb.iosb$w_status & 1) )
		return  iosb.iosb$w_status;

	if ( !(1 & (status = sys$qiow (EFN$C_ENF, *chan, IO$_SETMODE, &iosb, 
		0, 0, 0, 0, 0, 0, &options, 0))) )
		return	status;

	if ( !(iosb.iosb$w_status & 1) )
		return  iosb.iosb$w_status;

	return	status;


}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**	Send a data over TCP-connection.
**
**  FORMAL PARAMETERS:
**
**      chan:	a channel number, given from netio_conn, word, by value
**	buf:	a buffer to receiving data, byte array, by reference
**	buflen:	the length of a data in the buffer, word, by value
**
**  RETURN VALUE:
**
**      VMS condition code
**
**--
*/
static	int	netio_writepdu	(
			short	 chan, 
			int	 req,
			char	*buf, 
			short	 buflen
			)
{
unsigned	status, crc = 0;
IOSB	iosb;
unsigned short wlen = htons(buflen);
struct {unsigned l; void *a;} bvec[] = { sizeof(wlen), &wlen, sizeof(crc), &crc, buflen, buf};
struct dsc$descriptor buf_d = { sizeof(bvec), DSC$K_CLASS_S, DSC$K_DTYPE_T, &bvec} ;
SFTU_PDU *ppdu = buf;

	ppdu->pdu$l_req = htonl(req);

	crc = crc32c ( 0, buf, buflen);

	TRACE	" PDU --> req = !UL, crc = %x!ZL, data length = !UW octets", req, crc, buflen);

	crc = htonl(crc);

	/*
	** Enqueue a I/O request
	*/
	status = sys$qiow (EFN$C_ENF, chan, IO$_WRITEVBLK, &iosb, 0, 0, 0, 0, 0, 0, &buf_d, 0);

	return	(1 & status) ? iosb.iosb$w_status : status;
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**	Read a data from the specified network channel, reading for the
**	speficied amount of data with a timeout processing.
**
**  FORMAL PARAMETERS:
**
**      chan:	a channel number, given from netio_conn, word, by value
**	buf:	a buffer to receiving data, a byte array, by reference
**	buflen:	the size of the buffer, word, by value
**	sz:	a length of a received data in the buffer, word, by reference
**
**  RETURN VALUE:
**
**      VMS condition code
**
**--
*/
static	int	netio_readpdu	(
			short	 chan, 
			int	*req,
			char	*buf, 
			short	 buflen, 
			short	*sz
			)
{
unsigned status, crc = 0;
IOSB	iosb;
unsigned short wlen = 0;
struct {unsigned l; void *a;} bvec[] = { sizeof(wlen), &wlen, sizeof(crc), &crc};
struct dsc$descriptor buf_d = { sizeof(bvec), DSC$K_CLASS_S, DSC$K_DTYPE_T, &bvec} ;
SFTU_PDU *ppdu = buf;

	*sz = 0;

	/* Setup timer request */
	if ( !(1 & (status = sys$setimr (EFN$C_ENF, tmo, timer_ast, &chan, 0))) )
		return	status;

	/*
	** Read a word length and CRC fields of the PDU
	*/
	status	= sys$qiow (EFN$C_ENF, chan, IO$_READVBLK | IO$M_LOCKBUF, &iosb, 0, 0, 0, 0, 0, 0, 0, &buf_d);
	if ( !(1 & status) || !(1 & iosb.iosb$w_status) )
		return  (1 & status)?iosb.iosb$w_status:status;

	if ( !(wlen = ntohs(wlen)) )
		{
		sys$cantim(&chan, 0);
		return	SS$_NODATA;
		}

	if ( wlen > buflen )
		{
		sys$cantim(&chan, 0);
		return	SS$_INSFMEM;
		}

	crc	= ntohl(crc);

	/*
	** Reading the specified number of bytes
	*/
	status	= sys$qiow (EFN$C_ENF, chan, IO$_READVBLK | IO$M_LOCKBUF, &iosb, 0, 0, buf, wlen, 0, 0, 0, 0);

	/* Cancel a timer request */
	sys$cantim(&chan, 0);

	if ( !(1 & status) || !(1 & iosb.iosb$w_status) )
		return  (1 & status)?iosb.iosb$w_status:status;

	/*
	** Performs a check sum calculation and compare with received
	*/
	if ( crc != crc32c(0, buf, iosb.iosb$w_bcnt) )
		return	SS$_BADCHECKSUM;

	*sz	= iosb.iosb$w_bcnt;
       	*req	= ntohl(ppdu->pdu$l_req);


	TRACE	" PDU <-- req = !UL, crc = %x!ZL, data length = !UW octets", *req, crc, *sz);


	return	(1 & status)?iosb.iosb$w_status:status;
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Close (deassign) a given channel to a network device.
**
**  FORMAL PARAMETERS:
**
**      chan:	channel number, longword, by value
**
**  RETURN VALUE:
**
**      VMS condition code
**
**--
*/
static	int	netio_disc	(
			short	chan
			)
{
	sys$cancel (chan);
	return	sys$dassgn (chan);
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Open a passive socket and listening for incomming TCP-connection request.
**
**  FORMAL PARAMETERS:
**
**	netchan:	A returned network channel
**	port:		A local port number, 0 - assume default port number
**
**  RETURN VALUE:
**
**	VMS condition code
**
**--
*/
int	netio_open	(
		unsigned *	netchan, 
		unsigned short	port
			)
{
int	status;
const struct  {
        short   proto;
        char    type;
        char    domain;
        } sck_parm = {TCPIP$C_TCP, TCPIP$C_STREAM, TCPIP$C_AF_INET};
IOSB	iosb;
struct sockaddr_in home;
const ILE2	lhst_adrs = {sizeof(struct sockaddr_in), TCPIP$C_SOCK_NAME, &home}, 
		sock_opt[]={{sizeof(one), TCPIP$C_REUSEADDR, &one}, 
		{sizeof(one), TCPIP$C_KEEPALIVE, &one}}, 
		options = {sizeof(sock_opt), TCPIP$C_SOCKOPT, &sock_opt};

	/*
	** Open a network device
	*/
	if ( !(1 & (status = sys$assign(&dsc_ucxdev, netchan, 0, 0))) )
		{
		if ( status != SS$_NOSUCHDEV )
			return	status;
		if ( !(1 & (status = sys$assign( &dsc_tcpipdev, netchan, 0, 0))) )
			return	status;
		}

	/*
	** Create connection socket
	*/
	if ( !(1 & (status = sys$qiow(EFN$C_ENF, *netchan, IO$_SETMODE, &iosb, 0, 0, 
		       &sck_parm, 0, 0, 0, &options, 0))) || !(iosb.iosb$w_status & 1) )
		{
		sys$dassgn (*netchan);
		return	(1 & status) ? iosb.iosb$w_status : status;
		}

	/*
	** Initialize structure for creating main listening sockets
	*/
	memset(&home, 0, sizeof(struct sockaddr_in));

	home.sin_port		= htons(port);
	home.sin_family		= INET$C_AF_INET;
	home.sin_addr.s_addr	= TCPIP$C_INADDR_ANY;

	/*
	** Start waiting incomming connection request
	*/
	if ( !(1 & (status = sys$qiow(EFN$C_ENF, *netchan, IO$_SETMODE, &iosb, 0, 0, 
		       0, 0, &lhst_adrs, 5, 0, 0))) || !(iosb.iosb$w_status & 1) )
		return	(1 & status) ? iosb.iosb$w_status : status;

	return	status;
}

int	netio_accept	(
		unsigned	netchan, 
		unsigned *	chan
			)
{
unsigned status;
unsigned short	sz = 0;
IOSB	iosb;
struct sockaddr_in rhost;
ILE3	rhost_item = {sizeof(struct sockaddr_in), 0, &rhost, &sz};

	memset(&rhost, 0, sizeof(struct sockaddr_in));

	/* Create a socket for new connection */
	if ( !(1 & (status = sys$assign(&dsc_ucxdev, chan, 0, 0))) )
		{
		if ( status != SS$_NOSUCHDEV )
			return	status;
		if ( !(1 & (status = sys$assign(&dsc_tcpipdev, chan, 0, 0))) )
			return	status;
		}

	/* Accept client connection */
	if ( !(1 & (status = sys$qiow(EFN$C_ENF, netchan, IO$_ACCESS | IO$M_ACCEPT, &iosb, 0, 0, 
		       0, 0, &rhost_item, chan, 0, 0))) || !(iosb.iosb$w_status & 1) )
		{
		sys$dassgn ( *chan);
		return	(1 & status)?iosb.iosb$w_status:status;
		}


	_log(SFTU_ACCEPT, inet_ntoa(rhost.sin_addr), ntohs(rhost.sin_port));

	return status;
}


int	_do_put_file	(
		int		chan, 
	struct dsc$descriptor	*fdl_dsc, 
	struct dsc$descriptor	*fname_dsc,
			int	ack
			)
{
unsigned status, vbn = 0, len = 0, req = 0;
char	buf[SFTU$K_BUCKETSZ + 256];
SFTU_PDU *ppdu = (SFTU_PDU *) &buf;
	
	/* Form and send PUT request */
	ppdu->pdu$r_put.put$l_ebk = htonl(xabfhc.xab$l_ebk);
	ppdu->pdu$r_put.put$w_ffb = htons(xabfhc.xab$w_ffb);

	ppdu->pdu$r_put.put$r_fname.asc_b_len =  fname_dsc->dsc$w_length;
	memcpy(ppdu->pdu$r_put.put$r_fname.asc_t_sts, fname_dsc->dsc$a_pointer, ppdu->pdu$r_put.put$r_fname.asc_b_len);
	
	/* Send PUT request */
	len = PDU$K_SZ + PUT$K_SZ;
	
	TRACE	"PUT --> : !UW  octets --> !AC, EBK = !UL, FFB = !UL",
		len , &ppdu->pdu$r_put.put$r_fname, xabfhc.xab$l_ebk, xabfhc.xab$w_ffb);


	_log(SFTU_SNDPUT, fname_dsc, xabfhc.xab$l_ebk,  xabfhc.xab$w_ffb, ack ? "" : "NO" );


	if ( !(1 & (status = netio_writepdu (chan, SFTU$CTL_PUT, buf, len))) )
		return	status;

	if ( !(1 & (status = netio_readpdu  (chan, &req, buf, sizeof(buf), &len))) )
		return	status;

	/* We expect ACK with a good 'status' and starting VBN of file */
	if ( req != SFTU$CTL_ACK)
		return	SS$_ILLIOFUNC;

	vbn = ntohl(ppdu->pdu$r_ack.ack$l_vbn);
	status = ntohl(ppdu->pdu$r_ack.ack$l_status);

	TRACE	"ACK <-- : !UW  octets <-- VBN = !UL/%x!XL, status = %x!XL", len , vbn, vbn, status);

	_log(SFTU_RCVACK, status, vbn, vbn);


	if ( !(1 & status) )
		return	status;

	if ( vbn == 0xFFFFFFFF )
		return _log(SFTU_ALREADY);

	rab.rab$l_bkt = vbn;
	rab.rab$l_ubf = &ppdu->pdu$r_data.data$b_data;
	rab.rab$w_usz = SFTU$K_BUCKETSZ;
	

	_log(SFTU_SENDING, fname_dsc, vbn, ack ? "" : "NO" );

	while ( !exit_flag )
		{
		/*
		** Read block from file
		*/
		if ( !(1 & (status = sys$read(&rab))) && (status != RMS$_EOF) )
			return	status;

		if ( status == RMS$_EOF )
			break;

		rab.rab$l_bkt = 0;
		ppdu->pdu$r_data.data$w_len = htons(rab.rab$w_rsz);

		TRACE	"sys$read() -> status = !XL, VBN=!UL, !UW octets", status, vbn, rab.rab$w_rsz);
		vbn += rab.rab$w_rsz/SFTU$K_VBLOCKSZ;

		/*
		** Send block over network with DATA reqest
		*/
		len = PDU$K_SZ + DATA$K_SZ + rab.rab$w_rsz;
	
		TRACE	"DATA --> : !UW  octets", len);

		if ( !(1 & (status = netio_writepdu (chan, SFTU$CTL_DATA, buf, len))) )
			return	status;

		if ( ack )
			{
			/*
			** Expect ACK request
			*/
			if ( !(1 & (status = netio_readpdu  (chan, &req, buf, sizeof(buf), &len))) )
				return	status;

			/*
			** We expect ACK with a good 'status' and starting VBN of file
			*/
			if ( req != SFTU$CTL_ACK)
				return	SS$_ILLIOFUNC;

			status = ntohl(ppdu->pdu$r_ack.ack$l_status);
			TRACE	"ACK <-- : !UW  octets, status = !XL", len, status); 

			if ( !(1 & status) )
				return	status;
			}
		}


	return	_log(SFTU_SENT, fname_dsc, status);

}


int	_do_fal	(int chan)
{
unsigned	status, ebk, vbn, req = 0;
unsigned short	ffb, len = 0;
char	buf[SFTU$K_BUCKETSZ + 256];
SFTU_PDU *ppdu = (SFTU_PDU *) &buf;

	/* Read PDU of request */
	if ( !(1 & (status = netio_readpdu  (chan, &req, buf, sizeof(buf), &len))) )
		return	status;

	TRACE	"req = !UL/!XL", req, req);

	/* We expect the PUT requests */
	if ( req != SFTU$CTL_PUT )
		return	SS$_ILLIOFUNC;


	ebk = ntohl(ppdu->pdu$r_put.put$l_ebk);
	ffb = ntohs(ppdu->pdu$r_put.put$w_ffb);

	TRACE	"PUT <-- : req = !UL/!XL, EBK = !UL, FFB = !UW", req, req, ebk, ffb);

	/*
	** RMS's stuff initialization, open/create file ...
	*/
	fab = cc$rms_fab;
	xabfhc = cc$rms_xabfhc; 

	fab.fab$l_xab = &xabfhc;

	fab.fab$b_fac = FAB$M_PUT | FAB$M_BIO | FAB$M_UPD;
	fab.fab$l_fop = FAB$M_CIF;

	fab.fab$l_fna = ppdu->pdu$r_put.put$r_fname.asc_t_sts;
	fab.fab$b_fns = ppdu->pdu$r_put.put$r_fname.asc_b_len;

	fab.fab$b_org = FAB$C_SEQ;
	fab.fab$b_rfm = FAB$C_FIX;
	fab.fab$w_mrs = 512;

	if ( !(1 & (status = sys$create(&fab))) )
                lib$signal(status, fab.fab$l_stv);

	TRACE	"sys$create(!AC) -> %x!XL, EBK = !UL, FFB = !UW", &ppdu->pdu$r_put.put$r_fname,
			status, xabfhc.xab$l_ebk, xabfhc.xab$w_ffb);

	vbn = xabfhc.xab$l_ebk;

	/*
	** Do we get the whole file already ?!
	*/
	if ( (ntohl(ppdu->pdu$r_put.put$l_ebk) == xabfhc.xab$l_ebk)
		&& (ntohs(ppdu->pdu$r_put.put$w_ffb) == xabfhc.xab$w_ffb) )
		{
		if ( !(1 & (status = sys$close (&fab, 0, 0))) )
			lib$signal(status, fab.fab$l_stv);

		/* Form and send ACK request */
		ppdu->pdu$r_ack.ack$l_vbn = 0xFFFFFFFF;
		ppdu->pdu$r_ack.ack$l_status = htonl(SS$_NORMAL);

		TRACE	"ACK --> : !UW  octets, vbn = !UL, status = !XL", len, SS$_NORMAL, 0xFFFFFFFF); 

		return	netio_writepdu (chan, SFTU$CTL_ACK, buf, PDU$K_SZ + ACK$K_SZ);
		}

	/*
	** Send a start VBN to send
	*/
	ppdu->pdu$r_ack.ack$l_vbn = htonl(vbn);
	ppdu->pdu$r_ack.ack$l_status = htonl(SS$_NORMAL);

	status = netio_writepdu (chan, SFTU$CTL_ACK, buf, PDU$K_SZ + ACK$K_SZ);

	TRACE	"ACK --> : !UW  octets, vbn = !UL, status = !XL", len, SS$_NORMAL, vbn); 

	/*
	** Start receiving file bucket-by-bucket in loop ...
	*/
	rab = cc$rms_rab;
        rab.rab$l_fab = &fab;

	if ( !(1 & (status = sys$connect(&rab))) )
                lib$signal(status, rab.rab$l_stv);

	while ( !exit_flag )
		{
		if ( !(1 & (status = netio_readpdu  (chan, &req, buf, sizeof(buf), &len))) 
			|| (req == SFTU$CTL_QUIT) )
			break;
		else if ( req != SFTU$CTL_DATA )
			{
			lib$signal(status = SS$_ABORT);
			break;
			}

		rab.rab$l_bkt = vbn;
		rab.rab$l_rbf = &ppdu->pdu$r_data.data$b_data;
		rab.rab$w_rsz = ntohs(ppdu->pdu$r_data.data$w_len);

		TRACE	"sys$write (VBN=!UL, !UW octets)", vbn, rab.rab$w_rsz);

		if ( !(1 & (status = sys$write(&rab))) )
			break;
		vbn += rab.rab$w_rsz/SFTU$K_VBLOCKSZ;

		/*
		** Send a start VBN to send
		*/
		ppdu->pdu$r_ack.ack$l_vbn = htonl(vbn);
		ppdu->pdu$r_ack.ack$l_status = htonl(SS$_NORMAL);

		status = netio_writepdu (chan, SFTU$CTL_ACK, buf, PDU$K_SZ + ACK$K_SZ);

		TRACE	"ACK --> : !UW  octets, vbn = !UL, status = !XL", PDU$K_SZ + ACK$K_SZ, vbn, SS$_NORMAL); 
		}

	if ( !(1 & (status = sys$close (&fab, 0, 0))) )
		lib$signal(status, fab.fab$l_stv);

	return	status;
}

int	_sftu_fal	(void)
{
int	status, mainchan, chan, sz = 0;
unsigned short port = 8888;
char	buf[SFTU$K_BUCKETSZ + 256];
struct dsc$descriptor junk;

	trace_flag = CLI$_PRESENT == (status = cli$present (&q_trace));

	/* P1 - Port */
	INIT_SDESC(junk, sizeof(buf), buf);
	if ( (1 & (status = cli$get_value(&p_p1, &junk, &sz))) )
		if ( !lib$cvt_dtb(sz, buf, &port) )
			lib$signal( status );

	/* Open channel to TCP device, is used for network requests */
	if ( !(1 & (status = netio_open (&mainchan, port))) )
		return	status;


	_log(SFTU_LISTEN, mainchan, port);


	while ( !exit_flag )
		{
		/* Start waiting for new connection request */
		if ( !(1 & (status = netio_accept (mainchan, &chan))) )
			return	status;

		/*
		**
		*/
		status = _do_fal(chan);

		netio_disc(chan);
		}

	netio_disc(mainchan);

	return	status;
}


int	_sftu_put	(void)
{
int	status, flags = FDL$M_FDL_STRING, sz = 0, chan = 0, ack = 0;
char	fdl_string [SFTU$K_BUCKETSZ], resspec[NAM$C_MAXRSS], expspec[NAM$C_MAXRSS], fname[NAM$C_MAXRSS],
	buf[1024];
struct dsc$descriptor	fdl_dsc = { sizeof(fdl_string), DSC$K_DTYPE_T, DSC$K_CLASS_VS, fdl_string }, 
			fname_dsc = { sizeof(fname), DSC$K_DTYPE_T, DSC$K_CLASS_VS, fname },
			junk;
struct	sockaddr_in rsock = {0};
unsigned short port = 8888;

	trace_flag = CLI$_PRESENT == (status = cli$present (&q_trace));
	ack = CLI$_PRESENT == (status = cli$present (&q_ack));


	/* P1 - File */
	INIT_SDESC(junk, sizeof(fname), fname);
	if ( !(1 & (status = cli$get_value(&p_p1, &junk, &fname_dsc.dsc$w_length))) )
		return	status;

	/* P3 - Port */
	INIT_SDESC(junk, sizeof(buf), buf);
	if ( (1 & (status = cli$get_value(&p_p3, &junk, &sz))) )
		if ( !lib$cvt_dtb(sz, buf, &port) )
			lib$signal( status );

	/* P2 - Target Host IP */
	INIT_SDESC(junk, sizeof(buf), buf);
	if ( !(1 & (status = cli$get_value(&p_p2, &junk, &sz))) )
		return	status;

	/*
	** Initialize socket structure
	*/
	buf[sz] = '\0';

	rsock.sin_family = AF_INET;
	rsock.sin_port   = htons(port);
	rsock.sin_addr.s_addr = inet_addr( buf );


	/*
	** RMS's stuff initialization, open file ...
	*/
	fab = cc$rms_fab;
	nam = cc$rms_nam;
	xabfhc = cc$rms_xabfhc; 

	fab.fab$l_nam = &nam;
	fab.fab$l_xab = &xabfhc;

	nam.nam$b_rss = NAM$C_MAXRSS;	/* Max. resultant size */
	nam.nam$l_rsa = resspec;	/* Resultant address */
	nam.nam$b_ess = NAM$C_MAXRSS;	/* Max. expanded size */
	nam.nam$l_esa = expspec;	/* Expanded address */

        fab.fab$b_shr = FAB$M_SHRGET;
	fab.fab$b_fac = FAB$M_GET | FAB$M_BIO;

	fab.fab$l_fna = fname;
	fab.fab$b_fns = fname_dsc.dsc$w_length;;


	if ( !(1 & (status = sys$parse(&fab))) )
                lib$signal(status, fab.fab$l_stv);

	INIT_SDESC (fname_dsc, nam.nam$b_name + nam.nam$b_type, nam.nam$l_name);

	TRACE	 "'!AD', '!AS'", nam.nam$b_name + nam.nam$b_type, nam.nam$l_name, &fname_dsc);
	

	if ( !(1 & (status = sys$open(&fab))) )
                lib$signal(status, fab.fab$l_stv);

	rab = cc$rms_rab;
        rab.rab$l_fab = &fab;

	if ( !(1 & (status = sys$connect(&rab))) )
                lib$signal(status, rab.rab$l_stv);

	/*
	** Generate the File Definition Language string ...
	*/
	if ( !(1 & (status = fdl$generate(&flags, &fabp , &rabp, 0, 0, &fdl_dsc, 0, &sz))) )
		lib$signal(status);

	fdl_dsc.dsc$w_length = sz;

	TRACE	"!AS", &fdl_dsc);


	/*
	** All preparation steps has been done, now we are ready to send the file over the network ...
	*/
	if ( 1 & (status = netio_conn (&chan, &rsock)) )
		status = _do_put_file(chan, &fdl_dsc, &fname_dsc, ack);

	/*
	**
	*/
	netio_disc(chan);
	sys$close(&fab);

	return	status;
}


int	_sftu_fdl	(void)
{
int	status, sz = 0, chan = 0, flags = 0; // FDL$M_FDL_STRING;
char	fdl_string[NAM$C_MAXRSS], resspec[NAM$C_MAXRSS], expspec[NAM$C_MAXRSS], fname[NAM$C_MAXRSS],
	buf[1024];
struct dsc$descriptor	fdl_dsc,
			fname_dsc = { sizeof(fname), DSC$K_DTYPE_T, DSC$K_CLASS_VS, fname },
			junk;

	trace_flag = CLI$_PRESENT == (status = cli$present (&q_trace));


	/* P1 - File */
	INIT_SDESC(junk, sizeof(fname), fname);
	if ( !(1 & (status = cli$get_value(&p_p1, &junk, &fname_dsc.dsc$w_length))) )
		return	status;

	/*
	** RMS's stuff initialization, open file ...
	*/
	fab = cc$rms_fab;
	nam = cc$rms_nam;
	xabfhc = cc$rms_xabfhc; 

	fab.fab$l_nam = &nam;
	fab.fab$l_xab = &xabfhc;

	nam.nam$b_rss = NAM$C_MAXRSS;	/* Max. resultant size */
	nam.nam$l_rsa = resspec;	/* Resultant address */
	nam.nam$b_ess = NAM$C_MAXRSS;	/* Max. expanded size */
	nam.nam$l_esa = expspec;	/* Expanded address */

        fab.fab$b_shr = FAB$M_SHRGET;
	fab.fab$b_fac = FAB$M_GET | FAB$M_BIO;

	fab.fab$l_fna = fname;
	fab.fab$b_fns = fname_dsc.dsc$w_length;;

	if ( !(1 & (status = sys$parse(&fab))) )
                lib$signal(status, fab.fab$l_stv);

	INIT_SDESC (fname_dsc, nam.nam$b_name + nam.nam$b_type, nam.nam$l_name);

	TRACE	 "'!AD', '!AS'", nam.nam$b_name + nam.nam$b_type, nam.nam$l_name, &fname_dsc);
	

	if ( !(1 & (status = sys$open(&fab))) )
                lib$signal(status, fab.fab$l_stv);

	rab = cc$rms_rab;
        rab.rab$l_fab = &fab;

	if ( !(1 & (status = sys$connect(&rab))) )
                lib$signal(status, rab.rab$l_stv);


	/* P2 - FDL File Name to create */
	INIT_SDESC(fdl_dsc, sizeof(fdl_string), fdl_string);
	if ( 1 & (status = cli$get_value(&p_p2, &fdl_dsc, &sz)) )
		fdl_dsc.dsc$w_length = sz;
	else	{
		INIT_SDESC(fdl_dsc, nam.nam$b_name, nam.nam$l_name);
		}



	/*
	** Generate the File Definition Language string ...
	*/
	if ( !(1 & (status = fdl$generate(&flags, &fabp , &rabp, &fdl_dsc, 0, &fdl_dsc, 0, &sz))) )
		lib$signal(status);

	fdl_dsc.dsc$w_length = sz;

	TRACE	"!AS", &fdl_dsc);

	/*
	**
	*/
	sys$close(&fab);

	return	status;
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      A main procedure performs reading input, parsing and dispatch a command processing.
**
**  FORMAL PARAMETERS:
**
**      None
**
**  RETURN VALUE:
**
**      VMS condition code
**
**--
*/
int     main    (int    argc, char **argv)
{
int     status, flag = 0;
char    buf[128];
struct dsc$descriptor cmd_dsc;

	if ( !(1 & (status = sys$bintim (&tmo_dsc, &tmo))) )
		lib$signal(status);



        /*
        ** Check the presence of the comand line agruments
        */
        INIT_SDESC(cmd_dsc, sizeof(buf), buf);
        if ( !(1 & (status = lib$get_foreign(&cmd_dsc,0, &cmd_dsc.dsc$w_length, &flag))) )
                return status;

        if ( cmd_dsc.dsc$w_length )
                {
                if ( CLI$_NORMAL == (status = cli$dcl_parse (&cmd_dsc, &SFTU_CLD,0,0,0)) )
                        return  cli$dispatch();
                else    return  status;
                }

        /*
        ** Run interactively ...
        */
#if	0
        while ( RMS$_EOF != ( status=cli$dcl_parse (0,&SFTU_CLD, lib$get_input, lib$get_input, &prompt)) )
                {
                if ( status !=  CLI$_NORMAL )
                        continue;

		if ( !(1 & (status = cli$dispatch())) )
			return	status;
                }
#endif


	return	status;
}
