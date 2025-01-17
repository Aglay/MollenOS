/**
 * MollenOS
 *
 * Copyright 2019, Philip Meulengracht
 *
 * This program is free software : you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ? , either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Internet Address Support Definitions & Structures
 * - This header describes the base inetaddr-structures, prototypes
 *   and functionality, refer to the individual things for descriptions
 */

#ifndef __INET_INTERNET_H__
#define __INET_INTERNET_H__

#include <inet/socket.h>

#ifndef	_IN_PORT_T_DEFINED_
#define	_IN_PORT_T_DEFINED_
typedef	uint16_t in_port_t;
#endif

#ifndef	_IN_ADDR_T_DEFINED_
#define	_IN_ADDR_T_DEFINED_
typedef	uint32_t in_addr_t;
#endif

// Values that are available for sockets
#define IPPROTO_IP            0    /* Dummy protocol for TCP */
#define IPPROTO_HOPOPTS       0    /* IPv6 Hop-by-Hop options.  */
#define IPPROTO_ICMP          1    /* Internet Control Message Protocol */
#define IPPROTO_IGMP          2    /* Internet Group Management Protocol */
#define IPPROTO_IPIP          4    /* IPIP tunnels (older KA9Q tunnels use 94) */
#define IPPROTO_TCP           6    /* Transmission Control Protocol */
#define IPPROTO_EGP           8    /* Exterior Gateway Protocol */
#define IPPROTO_PUP           12   /* PUP protocol */
#define IPPROTO_UDP           17   /* User Datagram Protocol */
#define IPPROTO_IDP           22   /* XNS IDP protocol */
#define IPPROTO_TP            29   /* SO Transport Protocol Class 4.  */
#define IPPROTO_DCCP          33   /* Datagram Congestion Control Protocol */
#define IPPROTO_IPV6          41   /* IPv6-in-IPv4 tunnelling */
#define IPPROTO_ROUTING       43   /* IPv6 routing header. */
#define IPPROTO_FRAGMENT      44   /* IPv6 fragmentation header.  */
#define IPPROTO_RSVP          46   /* Reservation Protocol. */
#define IPPROTO_GRE           47   /* General Routing Encapsulation. */
#define IPPROTO_ESP           50   /* Encapsulation Security Payload protocol */
#define IPPROTO_AH            51   /* Authentication Header protocol */
#define IPPROTO_ICMPV6        58   /* ICMPv6 */
#define IPPROTO_NONE          59   /* IPv6 no next header. */
#define IPPROTO_DSTOPTS       60   /* IPv6 destination options. */
#define IPPROTO_MTP           92   /* Multicast Transport Protocol.  */
#define IPPROTO_ENCAP         98   /* Encapsulation Header. */
#define IPPROTO_BEETPH        94   /* IP option pseudo header for BEET */
#define IPPROTO_PIM           103  /* Protocol Independent Multicast */
#define IPPROTO_COMP          108  /* Compression Header protocol */
#define IPPROTO_SCTP          132  /* Stream Control Transport Protocol */
#define IPPROTO_UDPLITE       136  /* UDP-Lite (RFC 3828) */
#define IPPROTO_RAW           255  /* Raw IP packets */

// Definitions of the bits in an Internet address integer.
// On subnets, host and network parts are found according to
// the subnet mask, not these masks.
#define IN_CLASSA(a)       ((((in_addr_t)(a)) & 0x80000000) == 0)
#define IN_CLASSA_NET      0xff000000
#define IN_CLASSA_NSHIFT   24
#define IN_CLASSA_HOST     (0xffffffff & ~IN_CLASSA_NET)
#define IN_CLASSA_MAX      128
#define IN_CLASSB(a)       ((((in_addr_t)(a)) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET      0xffff0000
#define IN_CLASSB_NSHIFT   16
#define IN_CLASSB_HOST     (0xffffffff & ~IN_CLASSB_NET)
#define IN_CLASSB_MAX      65536
#define IN_CLASSC(a)       ((((in_addr_t)(a)) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET      0xffffff00
#define IN_CLASSC_NSHIFT   8
#define IN_CLASSC_HOST     (0xffffffff & ~IN_CLASSC_NET)
#define IN_CLASSD(a)       ((((in_addr_t)(a)) & 0xf0000000) == 0xe0000000)
#define IN_MULTICAST(a)    IN_CLASSD(a)
#define IN_EXPERIMENTAL(a) ((((in_addr_t)(a)) & 0xe0000000) == 0xe0000000)
#define IN_BADCLASS(a)     ((((in_addr_t)(a)) & 0xf0000000) == 0xf0000000)

#define INADDR_ANY ((in_addr_t) 0x00000000)        /* Address to accept any incoming messages.  */
#define INADDR_BROADCAST ((in_addr_t) 0xffffffff)  /* Address to send to all hosts.  */
#define INADDR_NONE ((in_addr_t) 0xffffffff)       /* Address indicating an error return.  */
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK ((in_addr_t) 0x7f000001)   /* Address for loopback to local host, inet 127.0.0.1.  */
#endif

#define IN6ADDR_ANY_INIT      {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}}
#define IN6ADDR_LOOPBACK_INIT {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}}}

/* Network number for local host loopback.  */
#define IN_LOOPBACKNET 127

/* Defines for Multicast INADDR.  */
#define INADDR_UNSPEC_GROUP    ((in_addr_t) 0xe0000000) /* 224.0.0.0 */
#define INADDR_ALLHOSTS_GROUP  ((in_addr_t) 0xe0000001) /* 224.0.0.1 */
#define INADDR_ALLRTRS_GROUP   ((in_addr_t) 0xe0000002) /* 224.0.0.2 */
#define INADDR_MAX_LOCAL_GROUP ((in_addr_t) 0xe00000ff) /* 224.0.0.255 */

// ip4 address
struct in_addr {
    in_addr_t s_addr; 
};

// ip6 address
struct in6_addr
{
    union {
        uint8_t  __u6_addr8[16];
        uint16_t __u6_addr16[8];
        uint32_t __u6_addr32[4];
    } __in6_u;
#define s6_addr    __in6_u.__u6_addr8
#define s6_addr16  __in6_u.__u6_addr16
#define s6_addr32  __in6_u.__u6_addr32
};

struct sockaddr_in {
	uint8_t          sin_len;
	sa_family_t      sin_family;   // e.g. AF_INET, AF_INET6
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};

struct sockaddr_in6 {
	uint8_t         sin6_len;
    uint16_t        sin6_family;   // address family, AF_INET6
    uint16_t        sin6_port;     // port number, Network Byte Order
    uint32_t        sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // IPv6 address
    uint32_t        sin6_scope_id; // Scope ID
};

#endif //!__INET_INTERNET_H__
