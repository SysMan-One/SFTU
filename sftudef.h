/********************************************************************************************************************************/
/* Created: 29-Jun-2016 16:57:20 by OpenVMS SDL EV2-3      */
/* Source:  22-JUN-2016 16:14:28 DISK$SYSMAN:[LAISHEV.WORK.SFTU]SFTUDEF.SDL;36 */
/********************************************************************************************************************************/
/*** MODULE SFTUDEF IDENT SFTUDEF-1-X ***/
#ifndef __SFTUDEF_LOADED
#define __SFTUDEF_LOADED 1
 
#pragma __nostandard			 /* This file uses non-ANSI-Standard features */
#pragma __member_alignment __save
#pragma __nomember_alignment
#ifdef __INITIAL_POINTER_SIZE			 /* Defined whenever ptr size pragmas supported */
#pragma __required_pointer_size __save		 /* Save the previously-defined required ptr size */
#pragma __required_pointer_size __short		 /* And set ptr size default to 32-bit pointers */
#endif
 
#ifdef __cplusplus
    extern "C" {
#define __unknown_params ...
#define __optional_params ...
#else
#define __unknown_params
#define __optional_params ...
#endif
 
#ifndef __struct
#if !defined(__VAXC)
#define __struct struct
#else
#define __struct variant_struct
#endif
#endif
 
#ifndef __union
#if !defined(__VAXC)
#define __union union
#else
#define __union variant_union
#endif
#endif
 
/*++                                                                        */
/* Facility:                                                                */
/*      StarLet File Transfer Utility                                       */
/*                                                                          */
/* Abstract:                                                                */
/*      This is an interface module contains data structures definitions,   */
/*      constants, functions/procedures prototypes.                         */
/*                                                                          */
/* Author:                                                                  */
/*      Ruslan R. Laishev                                                   */
/*                                                                          */
/* Creation Date: 2-JUN-2016                                                */
/*                                                                          */
/* Modification History:                                                    */
/*                                                                          */
/*--                                                                        */
#define SFTU$K_VBLOCKSZ 512
#define SFTU$K_BUCKETSZ 8192
/*                                                                          */
	
#ifdef __NEW_STARLET
typedef struct _asc {
    unsigned char asc_b_len;
    char asc_t_sts [255];
    } ASC;
#else	    /* __OLD_STARLET */
struct asc {
    unsigned char asc_b_len;
    char asc_t_sts [255];
    } ;
#endif	    /* #ifdef __NEW_STARLET */
/*                                                                          */
#define SFTU$CTL_LOGIN 1
#define SFTU$CTL_LOGOUT 2
#define SFTU$CTL_QUIT 4
#define SFTU$CTL_LS 5
#define SFTU$CTL_CD 6
#define SFTU$CTL_GET 7
#define SFTU$CTL_PUT 8
#define SFTU$CTL_DATA 9
#define SFTU$CTL_ACK 10
/*                                                                          */
#define PUT$K_SZ 262
	
#ifdef __NEW_STARLET
typedef struct _sftu_put {
    unsigned int put$l_ebk;             /* XAB$L_EBK                        */
    unsigned short int put$w_ffb;       /* XAB$W_FFB                        */
    ASC put$r_fname;
    } SFTU_PUT;
#else	    /* __OLD_STARLET */
struct sftu_put {
    unsigned int put$l_ebk;             /* XAB$L_EBK                        */
    unsigned short int put$w_ffb;       /* XAB$W_FFB                        */
    struct asc put$r_fname;
    } ;
#endif	    /* #ifdef __NEW_STARLET */
/*                                                                          */
#define DATA$K_SZ 2
	
#ifdef __NEW_STARLET
typedef struct _sftu_data {
    unsigned short int data$w_len;
    unsigned char data$b_data [0];
    } SFTU_DATA;
#else	    /* __OLD_STARLET */
struct sftu_data {
    unsigned short int data$w_len;
    unsigned char data$b_data [0];
    } ;
#endif	    /* #ifdef __NEW_STARLET */
/*                                                                          */
#define ACK$K_SZ 8
	
#ifdef __NEW_STARLET
typedef struct _sftu_ack {
    unsigned int ack$l_vbn;
    unsigned int ack$l_status;
    } SFTU_ACK;
#else	    /* __OLD_STARLET */
struct sftu_ack {
    unsigned int ack$l_vbn;
    unsigned int ack$l_status;
    } ;
#endif	    /* #ifdef __NEW_STARLET */
/*                                                                          */
#define PDU$K_SZ 4
	
#ifdef __NEW_STARLET
typedef struct _sftu_pdu {
    unsigned int pdu$l_req;
    __union  {
        SFTU_PUT pdu$r_put;
        SFTU_DATA pdu$r_data;
        SFTU_ACK pdu$r_ack;
        } pdu$r__u;
    } SFTU_PDU;
 
#if !defined(__VAXC)
#define pdu$r_put pdu$r__u.pdu$r_put
#define pdu$r_data pdu$r__u.pdu$r_data
#define pdu$r_ack pdu$r__u.pdu$r_ack
#endif		/* #if !defined(__VAXC) */
 
#else	    /* __OLD_STARLET */
struct sftu_pdu {
    unsigned int pdu$l_req;
    __union  {
        struct sftu_put pdu$r_put;
        struct sftu_data pdu$r_data;
        struct sftu_ack pdu$r_ack;
        } pdu$r__u;
    } ;
 
#if !defined(__VAXC)
#define pdu$r_put pdu$r__u.pdu$r_put
#define pdu$r_data pdu$r__u.pdu$r_data
#define pdu$r_ack pdu$r__u.pdu$r_ack
#endif		/* #if !defined(__VAXC) */
 
#endif	    /* #ifdef __NEW_STARLET */
/*                                                                          */
	
#ifdef __NEW_STARLET
typedef struct _sftu_frame {
    unsigned short int frame$w_len;
    unsigned int frame$l_crc32;
    SFTU_PDU frame$r_pdu;
    } SFTU_FRAME;
#else	    /* __OLD_STARLET */
struct sftu_frame {
    unsigned short int frame$w_len;
    unsigned int frame$l_crc32;
    struct sftu_pdu frame$r_pdu;
    } ;
#endif	    /* #ifdef __NEW_STARLET */
 
#pragma __member_alignment __restore
#ifdef __INITIAL_POINTER_SIZE			 /* Defined whenever ptr size pragmas supported */
#pragma __required_pointer_size __restore		 /* Restore the previously-defined required ptr size */
#endif
#ifdef __cplusplus
    }
#endif
#pragma __standard
 
#endif /* __SFTUDEF_LOADED */
 
