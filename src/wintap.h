#ifndef __WINTAP_H
#define __WINTAP_H

#define TAP_REGISTRY_DATA_SIZE 256

// Network-Endian (= Big-Endian)
typedef struct TapConfigTun
{
	struct in_addr Address;
	struct in_addr  Network;
	struct in_addr  Mask;
} TapConfigTun_t, *PTapConfigTun_t;

// Network-Endian (= Big-Endian), except LeaseDuration
typedef struct TapConfigDhcp
{
	struct in_addr  Address;
	struct in_addr  Mask;
	struct in_addr  DhcpServer;
	uint32_t  LeaseDuration; // Host-Endian (=Little-Endian). Anything else is Big-Endian
} TapConfigDhcp_t, *PTapConfigDhcp_t;

typedef struct TapDriverVersion
{
	uint32_t Major;
	uint32_t Minor;
	uint32_t Build;
	uint32_t Revision;
} TapDriverVersion_t, *PTapDriverVersion_t;

// Network-Endian (= Big-Endian)
typedef struct IpPacket {
	uint8_t	 ip_hl : 4,		/* header length */
			 ip_v : 4;			/* version */
	uint8_t	 ip_tos;			/* type of service */
	int16_t	 ip_len;			/* total length */
	uint16_t ip_id;			/* identification */
	int16_t	 ip_off;			/* fragment offset field */
	uint8_t	 ip_ttl;			/* time to live */
	uint8_t	 ip_p;			/* protocol */
	uint16_t ip_sum;			/* checksum */
	uint32_t ip_src, ip_dst;	/* source and dest address */
	uint8_t  payload[0];
} IpPacket_t, *PIpPacket_t;

void startTap(char* const argument);

#endif //__WINTAP_H


