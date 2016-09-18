#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#if !defined(_WIN32)
#include <sys/socket.h>
#endif

#include "output.h"
#include "crypto.h"
#include "endian.h"
#include "kms.h"
#include "shared_globals.h"
#include "helpers.h"

#define FRIENDLY_NAME_WINDOWS "Windows"
#define FRIENDLY_NAME_OFFICE2010 "Office 2010"
#define FRIENDLY_NAME_OFFICE2013 "Office 2013+"

#ifndef IS_LIBRARY

#ifdef NO_LOG
#define LOGTEXT(x) ""
#else //!NO_LOG
#define LOGTEXT(x) x
#endif // !NO_LOG

// Do not change the order of this list. Append items as necessary
const KmsIdList ProductList[] = {
	/* 000 */ { { 0xe85af946, 0x2e25, 0x47b7, { 0x83, 0xe1, 0xbe, 0xbc, 0xeb, 0xea, 0xc6, 0x11, } } /*e85af946-2e25-47b7-83e1-bebcebeac611*/, LOGTEXT("Office 2010"),                                        EPID_OFFICE2010, 4,  5 },
	/* 001 */ { { 0xe6a6f1bf, 0x9d40, 0x40c3, { 0xaa, 0x9f, 0xc7, 0x7b, 0xa2, 0x15, 0x78, 0xc0, } } /*e6a6f1bf-9d40-40c3-aa9f-c77ba21578c0*/, LOGTEXT("Office 2013"),                                        EPID_OFFICE2013, 5,  5 },
	/* 002 */ { { 0xaa4c7968, 0xb9da, 0x4680, { 0x92, 0xb6, 0xac, 0xb2, 0x5e, 0x2f, 0x86, 0x6c, } } /*aa4c7968-b9da-4680-92b6-acb25e2f866c*/, LOGTEXT("Office 2013 (Pre-Release)"),                          EPID_OFFICE2013, 5,  5 },
	/* 003 */ { { 0x85b5f61b, 0x320b, 0x4be3, { 0x81, 0x4a, 0xb7, 0x6b, 0x2b, 0xfa, 0xfc, 0x82, } } /*85b5f61b-320b-4be3-814a-b76b2bfafc82*/, LOGTEXT("Office 2016"),                                        EPID_OFFICE2016, 6,  5 },
#	ifndef NO_BASIC_PRODUCT_LIST
	/* 004 */ { { 0x212a64dc, 0x43b1, 0x4d3d, { 0xa3, 0x0c, 0x2f, 0xc6, 0x9d, 0x20, 0x95, 0xc6, } } /*212a64dc-43b1-4d3d-a30c-2fc69d2095c6*/, LOGTEXT("Windows Vista"),                                      EPID_WINDOWS,    4, 25 },
	/* 005 */ { { 0x7fde5219, 0xfbfa, 0x484a, { 0x82, 0xc9, 0x34, 0xd1, 0xad, 0x53, 0xe8, 0x56, } } /*7fde5219-fbfa-484a-82c9-34d1ad53e856*/, LOGTEXT("Windows 7"),                                          EPID_WINDOWS,    4, 25 },
	/* 006 */ { { 0x3c40b358, 0x5948, 0x45af, { 0x92, 0x3b, 0x53, 0xd2, 0x1f, 0xcc, 0x7e, 0x79, } } /*3c40b358-5948-45af-923b-53d21fcc7e79*/, LOGTEXT("Windows 8 (Volume)"),                                 EPID_WINDOWS,    5, 25 },
	/* 007 */ { { 0x5f94a0bb, 0xd5a0, 0x4081, { 0xa6, 0x85, 0x58, 0x19, 0x41, 0x8b, 0x2f, 0xe0, } } /*5f94a0bb-d5a0-4081-a685-5819418b2fe0*/, LOGTEXT("Windows Preview"),                                    EPID_WINDOWS,    5, 25 },
	/* 008 */ { { 0xbbb97b3b, 0x8ca4, 0x4a28, { 0x97, 0x17, 0x89, 0xfa, 0xbd, 0x42, 0xc4, 0xac, } } /*bbb97b3b-8ca4-4a28-9717-89fabd42c4ac*/, LOGTEXT("Windows 8 (Retail)"),                                 EPID_WINDOWS,    5, 25 },
	/* 009 */ { { 0xcb8fc780, 0x2c05, 0x495a, { 0x97, 0x10, 0x85, 0xaf, 0xff, 0xc9, 0x04, 0xd7, } } /*cb8fc780-2c05-495a-9710-85afffc904d7*/, LOGTEXT("Windows 8.1 (Volume)"),                               EPID_WINDOWS,    6, 25 },
	/* 010 */ { { 0x6d646890, 0x3606, 0x461a, { 0x86, 0xab, 0x59, 0x8b, 0xb8, 0x4a, 0xce, 0x82, } } /*6d646890-3606-461a-86ab-598bb84ace82*/, LOGTEXT("Windows 8.1 (Retail)"),                               EPID_WINDOWS,    6, 25 },
	/* 011 */ { { 0x33e156e4, 0xb76f, 0x4a52, { 0x9f, 0x91, 0xf6, 0x41, 0xdd, 0x95, 0xac, 0x48, } } /*33e156e4-b76f-4a52-9f91-f641dd95ac48*/, LOGTEXT("Windows Server 2008 A (Web and HPC)"),                EPID_WINDOWS,    4,  5 },
	/* 012 */ { { 0x8fe53387, 0x3087, 0x4447, { 0x89, 0x85, 0xf7, 0x51, 0x32, 0x21, 0x5a, 0xc9, } } /*8fe53387-3087-4447-8985-f75132215ac9*/, LOGTEXT("Windows Server 2008 B (Standard and Enterprise)"),    EPID_WINDOWS,    4,  5 },
	/* 013 */ { { 0x8a21fdf3, 0xcbc5, 0x44eb, { 0x83, 0xf3, 0xfe, 0x28, 0x4e, 0x66, 0x80, 0xa7, } } /*8a21fdf3-cbc5-44eb-83f3-fe284e6680a7*/, LOGTEXT("Windows Server 2008 C (Datacenter)"),                 EPID_WINDOWS,    4,  5 },
	/* 014 */ { { 0x0fc6ccaf, 0xff0e, 0x4fae, { 0x9d, 0x08, 0x43, 0x70, 0x78, 0x5b, 0xf7, 0xed, } } /*0fc6ccaf-ff0e-4fae-9d08-4370785bf7ed*/, LOGTEXT("Windows Server 2008 R2 A (Web and HPC)"),             EPID_WINDOWS,    4,  5 },
	/* 015 */ { { 0xca87f5b6, 0xcd46, 0x40c0, { 0xb0, 0x6d, 0x8e, 0xcd, 0x57, 0xa4, 0x37, 0x3f, } } /*ca87f5b6-cd46-40c0-b06d-8ecd57a4373f*/, LOGTEXT("Windows Server 2008 R2 B (Standard and Enterprise)"), EPID_WINDOWS,    4,  5 },
	/* 016 */ { { 0xb2ca2689, 0xa9a8, 0x42d7, { 0x93, 0x8d, 0xcf, 0x8e, 0x9f, 0x20, 0x19, 0x58, } } /*b2ca2689-a9a8-42d7-938d-cf8e9f201958*/, LOGTEXT("Windows Server 2008 R2 C (Datacenter)"),              EPID_WINDOWS,    4,  5 },
	/* 017 */ { { 0x8665cb71, 0x468c, 0x4aa3, { 0xa3, 0x37, 0xcb, 0x9b, 0xc9, 0xd5, 0xea, 0xac, } } /*8665cb71-468c-4aa3-a337-cb9bc9d5eaac*/, LOGTEXT("Windows Server 2012"),                                EPID_WINDOWS,    5,  5 },
	/* 018 */ { { 0x8456efd3, 0x0c04, 0x4089, { 0x87, 0x40, 0x5b, 0x72, 0x38, 0x53, 0x5a, 0x65, } } /*8456efd3-0c04-4089-8740-5b7238535a65*/, LOGTEXT("Windows Server 2012 R2"),                             EPID_WINDOWS,    6,  5 },
	/* 019 */ { { 0x6d5f5270, 0x31ac, 0x433e, { 0xb9, 0x0a, 0x39, 0x89, 0x29, 0x23, 0xc6, 0x57, } } /*6d5f5270-31ac-433e-b90a-39892923c657*/, LOGTEXT("Windows Server Preview"),                             EPID_WINDOWS,    6,  5 },
	/* 020 */ { { 0x58e2134f, 0x8e11, 0x4d17, { 0x9c, 0xb2, 0x91, 0x06, 0x9c, 0x15, 0x11, 0x48, } } /*58e2134f-8e11-4d17-9cb2-91069c151148*/, LOGTEXT("Windows 10 2015 (Volume)"),                           EPID_WINDOWS,    6, 25 },
	/* 021 */ { { 0xe1c51358, 0xfe3e, 0x4203, { 0xa4, 0xa2, 0x3b, 0x6b, 0x20, 0xc9, 0x73, 0x4e, } } /*e1c51358-fe3e-4203-a4a2-3b6b20c9734e*/, LOGTEXT("Windows 10 (Retail)"),                                EPID_WINDOWS,    6, 25 },
	/* 022 */ { { 0x6e9fc069, 0x257d, 0x4bc4, { 0xb4, 0xa7, 0x75, 0x05, 0x14, 0xd3, 0x27, 0x43, } } /*6e9fc069-257d-4bc4-b4a7-750514d32743*/, LOGTEXT("Windows Server 2016"),                                EPID_WINDOWS,    6,  5 },
	/* 023 */ { { 0x969fe3c0, 0xa3ec, 0x491a, { 0x9f, 0x25, 0x42, 0x36, 0x05, 0xde, 0xb3, 0x65, } } /*969fe3c0-a3ec-491a-9f25-423605deb365*/, LOGTEXT("Windows 10 2016 (Volume)"),                           EPID_WINDOWS,    6, 25 },
#	endif // NO_BASIC_PRODUCT_LIST
	/* 024 */ { { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, } }, NULL, NULL, 0, 0 }
};

#ifndef NO_LOG
// Application ID is used by KMS server to count KeyManagementServiceCurrentCount
// Do not change the order of this list. Append items as necessary
const KmsIdList AppList[] = {
	/* 000 */ { { 0x55c92734, 0xd682, 0x4d71, { 0x98, 0x3e, 0xd6, 0xec, 0x3f, 0x16, 0x05, 0x9f } } /*"55C92734-D682-4D71-983E-D6EC3F16059F"*/, LOGTEXT(FRIENDLY_NAME_WINDOWS),    EPID_WINDOWS, 	0,	0},
	/* 001 */ { { 0x59A52881, 0xa989, 0x479d, { 0xaf, 0x46, 0xf2, 0x75, 0xc6, 0x37, 0x06, 0x63 } } /*"59A52881-A989-479D-AF46-F275C6370663"*/, LOGTEXT(FRIENDLY_NAME_OFFICE2010), EPID_OFFICE2010,	0,	0},
	/* 002 */ { { 0x0FF1CE15, 0xA989, 0x479D, { 0xaf, 0x46, 0xf2, 0x75, 0xc6, 0x37, 0x06, 0x63 } } /*"0FF1CE15-A989-479D-AF46-F275C6370663"*/, LOGTEXT(FRIENDLY_NAME_OFFICE2013), EPID_OFFICE2013,	0,	0},
	/* 003 */ { { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, NULL, NULL, 0, 0 }
};
#endif // NO_LOG

#ifndef NO_EXTENDED_PRODUCT_LIST
const KmsIdList ExtendedProductList [] = {
		// Windows 10 (Retail)
		{ { 0x58e97c99, 0xf377, 0x4ef1, { 0x81, 0xd5, 0x4a, 0xd5, 0x52, 0x2b, 0x5f, 0xd8, } } /*58e97c99-f377-4ef1-81d5-4ad5522b5fd8*/, LOGTEXT("Windows 10 Home"),                                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_RETAIL },
		{ { 0xa9107544, 0xf4a0, 0x4053, { 0xa9, 0x6a, 0x14, 0x79, 0xab, 0xde, 0xf9, 0x12, } } /*a9107544-f4a0-4053-a96a-1479abdef912*/, LOGTEXT("Windows 10 Home Country Specific"),               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_RETAIL },
		{ { 0x7b9e1751, 0xa8da, 0x4f75, { 0x95, 0x60, 0x5f, 0xad, 0xfe, 0x3d, 0x8e, 0x38, } } /*7b9e1751-a8da-4f75-9560-5fadfe3d8e38*/, LOGTEXT("Windows 10 Home N"),                              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_RETAIL },
		{ { 0xcd918a57, 0xa41b, 0x4c82, { 0x8d, 0xce, 0x1a, 0x53, 0x8e, 0x22, 0x1a, 0x83, } } /*cd918a57-a41b-4c82-8dce-1a538e221a83*/, LOGTEXT("Windows 10 Home Single Language"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_RETAIL },

		// Windows 10 2015 (Volume)
		{ { 0xe0c42288, 0x980c, 0x4788, { 0xa0, 0x14, 0xc0, 0x80, 0xd2, 0xe1, 0x92, 0x6e, } } /*e0c42288-980c-4788-a014-c080d2e1926e*/, LOGTEXT("Windows 10 Education"),                           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0x3c102355, 0xd027, 0x42c6, { 0xad, 0x23, 0x2e, 0x7e, 0xf8, 0xa0, 0x25, 0x85, } } /*3c102355-d027-42c6-ad23-2e7ef8a02585*/, LOGTEXT("Windows 10 Education N"),                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0x73111121, 0x5638, 0x40f6, { 0xbc, 0x11, 0xf1, 0xd7, 0xb0, 0xd6, 0x43, 0x00, } } /*73111121-5638-40f6-bc11-f1d7b0d64300*/, LOGTEXT("Windows 10 Enterprise"),                          EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0x7b51a46c, 0x0c04, 0x4e8f, { 0x9a, 0xf4, 0x84, 0x96, 0xcc, 0xa9, 0x0d, 0x5e, } } /*7b51a46c-0c04-4e8f-9af4-8496cca90d5e*/, LOGTEXT("Windows 10 Enterprise 2015 LTSB"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0x87b838b7, 0x41b6, 0x4590, { 0x83, 0x18, 0x57, 0x97, 0x95, 0x1d, 0x85, 0x29, } } /*87b838b7-41b6-4590-8318-5797951d8529*/, LOGTEXT("Windows 10 Enterprise 2015 LTSB N"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0xe272e3e2, 0x732f, 0x4c65, { 0xa8, 0xf0, 0x48, 0x47, 0x47, 0xd0, 0xd9, 0x47, } } /*e272e3e2-732f-4c65-a8f0-484747d0d947*/, LOGTEXT("Windows 10 Enterprise N"),                        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0x43f2ab05, 0x7c87, 0x4d56, { 0xb2, 0x7c, 0x44, 0xd0, 0xf9, 0xa3, 0xda, 0xbd, } } /*43f2ab05-7c87-4d56-b27c-44d0f9a3dabd*/, LOGTEXT("Windows 10 Enterprise Preview"),                  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0x2de67392, 0xb7a7, 0x462a, { 0xb1, 0xca, 0x10, 0x8d, 0xd1, 0x89, 0xf5, 0x88, } } /*2de67392-b7a7-462a-b1ca-108dd189f588*/, LOGTEXT("Windows 10 Professional"),                        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0x3f1afc82, 0xf8ac, 0x4f6c, { 0x80, 0x05, 0x1d, 0x23, 0x3e, 0x60, 0x6e, 0xee, } } /*3f1afc82-f8ac-4f6c-8005-1d233e606eee*/, LOGTEXT("Windows 10 Professional Education"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0x5300b18c, 0x2e33, 0x4dc2, { 0x82, 0x91, 0x47, 0xff, 0xce, 0xc7, 0x46, 0xdd, } } /*5300b18c-2e33-4dc2-8291-47ffcec746dd*/, LOGTEXT("Windows 10 Professional Education N"),            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0xa80b5abf, 0x76ad, 0x428b, { 0xb0, 0x5d, 0xa4, 0x7d, 0x2d, 0xff, 0xee, 0xbf, } } /*a80b5abf-76ad-428b-b05d-a47d2dffeebf*/, LOGTEXT("Windows 10 Professional N"),                      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
		{ { 0xff808201, 0xfec6, 0x4fd4, { 0xae, 0x16, 0xab, 0xbd, 0xda, 0xde, 0x57, 0x06, } } /*ff808201-fec6-4fd4-ae16-abbddade5706*/, LOGTEXT("Windows 10 Professional Preview"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },

		// Windows 10 2016 (Volume)
		{ { 0x2d5a5a60, 0x3040, 0x48bf, { 0xbe, 0xb0, 0xfc, 0xd7, 0x70, 0xc2, 0x0c, 0xe0, } } /*2d5a5a60-3040-48bf-beb0-fcd770c20ce0*/, LOGTEXT("Windows 10 Enterprise 2016 LTSB"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_LTSB2016 },
		{ { 0x9f776d83, 0x7156, 0x45b2, { 0x8a, 0x5c, 0x35, 0x9b, 0x9c, 0x9f, 0x22, 0xa3, } } /*9f776d83-7156-45b2-8a5c-359b9c9f22a3*/, LOGTEXT("Windows 10 Enterprise 2016 LTSB N"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_LTSB2016 },

		// Windows 10 Unknown (Volume)

		// Windows 7
		{ { 0xdb537896, 0x376f, 0x48ae, { 0xa4, 0x92, 0x53, 0xd0, 0x54, 0x77, 0x73, 0xd0, } } /*db537896-376f-48ae-a492-53d0547773d0*/, LOGTEXT("Windows 7 Embedded POSReady"),                    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
		{ { 0xe1a8296a, 0xdb37, 0x44d1, { 0x8c, 0xce, 0x7b, 0xc9, 0x61, 0xd5, 0x9c, 0x54, } } /*e1a8296a-db37-44d1-8cce-7bc961d59c54*/, LOGTEXT("Windows 7 Embedded Standard"),                    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
		{ { 0xae2ee509, 0x1b34, 0x41c0, { 0xac, 0xb7, 0x6d, 0x46, 0x50, 0x16, 0x89, 0x15, } } /*ae2ee509-1b34-41c0-acb7-6d4650168915*/, LOGTEXT("Windows 7 Enterprise"),                           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
		{ { 0x46bbed08, 0x9c7b, 0x48fc, { 0xa6, 0x14, 0x95, 0x25, 0x05, 0x73, 0xf4, 0xea, } } /*46bbed08-9c7b-48fc-a614-95250573f4ea*/, LOGTEXT("Windows 7 Enterprise E"),                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
		{ { 0x1cb6d605, 0x11b3, 0x4e14, { 0xbb, 0x30, 0xda, 0x91, 0xc8, 0xe3, 0x98, 0x3a, } } /*1cb6d605-11b3-4e14-bb30-da91c8e3983a*/, LOGTEXT("Windows 7 Enterprise N"),                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
		{ { 0xb92e9980, 0xb9d5, 0x4821, { 0x9c, 0x94, 0x14, 0x0f, 0x63, 0x2f, 0x63, 0x12, } } /*b92e9980-b9d5-4821-9c94-140f632f6312*/, LOGTEXT("Windows 7 Professional"),                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
		{ { 0x5a041529, 0xfef8, 0x4d07, { 0xb0, 0x6f, 0xb5, 0x9b, 0x57, 0x3b, 0x32, 0xd2, } } /*5a041529-fef8-4d07-b06f-b59b573b32d2*/, LOGTEXT("Windows 7 Professional E"),                       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
		{ { 0x54a09a0d, 0xd57b, 0x4c10, { 0x8b, 0x69, 0xa8, 0x42, 0xd6, 0x59, 0x0a, 0xd5, } } /*54a09a0d-d57b-4c10-8b69-a842d6590ad5*/, LOGTEXT("Windows 7 Professional N"),                       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
		{ { 0xaa6dd3aa, 0xc2b4, 0x40e2, { 0xa5, 0x44, 0xa6, 0xbb, 0xb3, 0xf5, 0xc3, 0x95, } } /*aa6dd3aa-c2b4-40e2-a544-a6bbb3f5c395*/, LOGTEXT("Windows 7 ThinPC"),                               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },

		// Windows 8 (Retail)
		{ { 0xc04ed6bf, 0x55c8, 0x4b47, { 0x9f, 0x8e, 0x5a, 0x1f, 0x31, 0xce, 0xee, 0x60, } } /*c04ed6bf-55c8-4b47-9f8e-5a1f31ceee60*/, LOGTEXT("Windows 8 Core"),                                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },
		{ { 0x9d5584a2, 0x2d85, 0x419a, { 0x98, 0x2c, 0xa0, 0x08, 0x88, 0xbb, 0x9d, 0xdf, } } /*9d5584a2-2d85-419a-982c-a00888bb9ddf*/, LOGTEXT("Windows 8 Core Country Specific"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },
		{ { 0x197390a0, 0x65f6, 0x4a95, { 0xbd, 0xc4, 0x55, 0xd5, 0x8a, 0x3b, 0x02, 0x53, } } /*197390a0-65f6-4a95-bdc4-55d58a3b0253*/, LOGTEXT("Windows 8 Core N"),                               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },
		{ { 0x8860fcd4, 0xa77b, 0x4a20, { 0x90, 0x45, 0xa1, 0x50, 0xff, 0x11, 0xd6, 0x09, } } /*8860fcd4-a77b-4a20-9045-a150ff11d609*/, LOGTEXT("Windows 8 Core Single Language"),                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },
		{ { 0xa00018a3, 0xf20f, 0x4632, { 0xbf, 0x7c, 0x8d, 0xaa, 0x53, 0x51, 0xc9, 0x14, } } /*a00018a3-f20f-4632-bf7c-8daa5351c914*/, LOGTEXT("Windows 8 Professional WMC"),                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },

		// Windows 8 (Volume)
		{ { 0x18db1848, 0x12e0, 0x4167, { 0xb9, 0xd7, 0xda, 0x7f, 0xcd, 0xa5, 0x07, 0xdb, } } /*18db1848-12e0-4167-b9d7-da7fcda507db*/, LOGTEXT("Windows 8 Embedded Industry Enterprise"),         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
		{ { 0x10018baf, 0xce21, 0x4060, { 0x80, 0xbd, 0x47, 0xfe, 0x74, 0xed, 0x4d, 0xab, } } /*10018baf-ce21-4060-80bd-47fe74ed4dab*/, LOGTEXT("Windows 8 Embedded Industry Professional"),       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
		{ { 0x458e1bec, 0x837a, 0x45f6, { 0xb9, 0xd5, 0x92, 0x5e, 0xd5, 0xd2, 0x99, 0xde, } } /*458e1bec-837a-45f6-b9d5-925ed5d299de*/, LOGTEXT("Windows 8 Enterprise"),                           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
		{ { 0xe14997e7, 0x800a, 0x4cf7, { 0xad, 0x10, 0xde, 0x4b, 0x45, 0xb5, 0x78, 0xdb, } } /*e14997e7-800a-4cf7-ad10-de4b45b578db*/, LOGTEXT("Windows 8 Enterprise N"),                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
		{ { 0xa98bcd6d, 0x5343, 0x4603, { 0x8a, 0xfe, 0x59, 0x08, 0xe4, 0x61, 0x11, 0x12, } } /*a98bcd6d-5343-4603-8afe-5908e4611112*/, LOGTEXT("Windows 8 Professional"),                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
		{ { 0xebf245c1, 0x29a8, 0x4daf, { 0x9c, 0xb1, 0x38, 0xdf, 0xc6, 0x08, 0xa8, 0xc8, } } /*ebf245c1-29a8-4daf-9cb1-38dfc608a8c8*/, LOGTEXT("Windows 8 Professional N"),                       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },

		// Windows 8.1 (Retail)
		{ { 0xfe1c3238, 0x432a, 0x43a1, { 0x8e, 0x25, 0x97, 0xe7, 0xd1, 0xef, 0x10, 0xf3, } } /*fe1c3238-432a-43a1-8e25-97e7d1ef10f3*/, LOGTEXT("Windows 8.1 Core"),                               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
		{ { 0xffee456a, 0xcd87, 0x4390, { 0x8e, 0x07, 0x16, 0x14, 0x6c, 0x67, 0x2f, 0xd0, } } /*ffee456a-cd87-4390-8e07-16146c672fd0*/, LOGTEXT("Windows 8.1 Core ARM"),                           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
		{ { 0xdb78b74f, 0xef1c, 0x4892, { 0xab, 0xfe, 0x1e, 0x66, 0xb8, 0x23, 0x1d, 0xf6, } } /*db78b74f-ef1c-4892-abfe-1e66b8231df6*/, LOGTEXT("Windows 8.1 Core Country Specific"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
		{ { 0x78558a64, 0xdc19, 0x43fe, { 0xa0, 0xd0, 0x80, 0x75, 0xb2, 0xa3, 0x70, 0xa3, } } /*78558a64-dc19-43fe-a0d0-8075b2a370a3*/, LOGTEXT("Windows 8.1 Core N"),                             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
		{ { 0xc72c6a1d, 0xf252, 0x4e7e, { 0xbd, 0xd1, 0x3f, 0xca, 0x34, 0x2a, 0xcb, 0x35, } } /*c72c6a1d-f252-4e7e-bdd1-3fca342acb35*/, LOGTEXT("Windows 8.1 Core Single Language"),               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
		{ { 0xe58d87b5, 0x8126, 0x4580, { 0x80, 0xfb, 0x86, 0x1b, 0x22, 0xf7, 0x92, 0x96, } } /*e58d87b5-8126-4580-80fb-861b22f79296*/, LOGTEXT("Windows 8.1 Professional Student"),               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
		{ { 0xcab491c7, 0xa918, 0x4f60, { 0xb5, 0x02, 0xda, 0xb7, 0x5e, 0x33, 0x4f, 0x40, } } /*cab491c7-a918-4f60-b502-dab75e334f40*/, LOGTEXT("Windows 8.1 Professional Student N"),             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
		{ { 0x096ce63d, 0x4fac, 0x48a9, { 0x82, 0xa9, 0x61, 0xae, 0x9e, 0x80, 0x0e, 0x5f, } } /*096ce63d-4fac-48a9-82a9-61ae9e800e5f*/, LOGTEXT("Windows 8.1 Professional WMC"),                   EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },

		// Windows 8.1 (Volume)
		{ { 0xe9942b32, 0x2e55, 0x4197, { 0xb0, 0xbd, 0x5f, 0xf5, 0x8c, 0xba, 0x88, 0x60, } } /*e9942b32-2e55-4197-b0bd-5ff58cba8860*/, LOGTEXT("Windows 8.1 Core Connected"),                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0xba998212, 0x460a, 0x44db, { 0xbf, 0xb5, 0x71, 0xbf, 0x09, 0xd1, 0xc6, 0x8b, } } /*ba998212-460a-44db-bfb5-71bf09d1c68b*/, LOGTEXT("Windows 8.1 Core Connected Country Specific"),    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0xc6ddecd6, 0x2354, 0x4c19, { 0x90, 0x9b, 0x30, 0x6a, 0x30, 0x58, 0x48, 0x4e, } } /*c6ddecd6-2354-4c19-909b-306a3058484e*/, LOGTEXT("Windows 8.1 Core Connected N"),                   EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0xb8f5e3a3, 0xed33, 0x4608, { 0x81, 0xe1, 0x37, 0xd6, 0xc9, 0xdc, 0xfd, 0x9c, } } /*b8f5e3a3-ed33-4608-81e1-37d6c9dcfd9c*/, LOGTEXT("Windows 8.1 Core Connected Single Language"),     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0xf7e88590, 0xdfc7, 0x4c78, { 0xbc, 0xcb, 0x6f, 0x38, 0x65, 0xb9, 0x9d, 0x1a, } } /*f7e88590-dfc7-4c78-bccb-6f3865b99d1a*/, LOGTEXT("Windows 8.1 Embedded Industry Automotive"),       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0xcd4e2d9f, 0x5059, 0x4a50, { 0xa9, 0x2d, 0x05, 0xd5, 0xbb, 0x12, 0x67, 0xc7, } } /*cd4e2d9f-5059-4a50-a92d-05d5bb1267c7*/, LOGTEXT("Windows 8.1 Embedded Industry Enterprise"),       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0x0ab82d54, 0x47f4, 0x4acb, { 0x81, 0x8c, 0xcc, 0x5b, 0xf0, 0xec, 0xb6, 0x49, } } /*0ab82d54-47f4-4acb-818c-cc5bf0ecb649*/, LOGTEXT("Windows 8.1 Embedded Industry Professional"),     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0x81671aaf, 0x79d1, 0x4eb1, { 0xb0, 0x04, 0x8c, 0xbb, 0xe1, 0x73, 0xaf, 0xea, } } /*81671aaf-79d1-4eb1-b004-8cbbe173afea*/, LOGTEXT("Windows 8.1 Enterprise"),                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0x113e705c, 0xfa49, 0x48a4, { 0xbe, 0xea, 0x7d, 0xd8, 0x79, 0xb4, 0x6b, 0x14, } } /*113e705c-fa49-48a4-beea-7dd879b46b14*/, LOGTEXT("Windows 8.1 Enterprise N"),                       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0xc06b6981, 0xd7fd, 0x4a35, { 0xb7, 0xb4, 0x05, 0x47, 0x42, 0xb7, 0xaf, 0x67, } } /*c06b6981-d7fd-4a35-b7b4-054742b7af67*/, LOGTEXT("Windows 8.1 Professional"),                       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
		{ { 0x7476d79f, 0x8e48, 0x49b4, { 0xab, 0x63, 0x4d, 0x0b, 0x81, 0x3a, 0x16, 0xe4, } } /*7476d79f-8e48-49b4-ab63-4d0b813a16e4*/, LOGTEXT("Windows 8.1 Professional N"),                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },

		// Windows Preview
	#	ifdef INCLUDE_BETAS
		{ { 0xcde952c7, 0x2f96, 0x4d9d, { 0x8f, 0x2b, 0x2d, 0x34, 0x9f, 0x64, 0xfc, 0x51, } } /*cde952c7-2f96-4d9d-8f2b-2d349f64fc51*/, LOGTEXT("Windows 10 Enterprise Preview"),                  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
		{ { 0xa4383e6b, 0xdada, 0x423d, { 0xa4, 0x3d, 0xf2, 0x56, 0x78, 0x42, 0x96, 0x76, } } /*a4383e6b-dada-423d-a43d-f25678429676*/, LOGTEXT("Windows 10 Professional Preview"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
		{ { 0xcf59a07b, 0x1a2a, 0x4be0, { 0xbf, 0xe0, 0x42, 0x3b, 0x58, 0x23, 0xe6, 0x63, } } /*cf59a07b-1a2a-4be0-bfe0-423b5823e663*/, LOGTEXT("Windows 10 Professional WMC Preview"),            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
		{ { 0x2b9c337f, 0x7a1d, 0x4271, { 0x90, 0xa3, 0xc6, 0x85, 0x5a, 0x2b, 0x8a, 0x1c, } } /*2b9c337f-7a1d-4271-90a3-c6855a2b8a1c*/, LOGTEXT("Windows 8.x Preview"),                            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
		{ { 0x631ead72, 0xa8ab, 0x4df8, { 0xbb, 0xdf, 0x37, 0x20, 0x29, 0x98, 0x9b, 0xdd, } } /*631ead72-a8ab-4df8-bbdf-372029989bdd*/, LOGTEXT("Windows 8.x Preview ARM"),                        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
	#	endif // INCLUDE_BETAS

		// Windows Server 2008 A (Web and HPC)
		{ { 0x7afb1156, 0x2c1d, 0x40fc, { 0xb2, 0x60, 0xaa, 0xb7, 0x44, 0x2b, 0x62, 0xfe, } } /*7afb1156-2c1d-40fc-b260-aab7442b62fe*/, LOGTEXT("Windows Server 2008 Compute Cluster"),            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008A },
		{ { 0xddfa9f7c, 0xf09e, 0x40b9, { 0x8c, 0x1a, 0xbe, 0x87, 0x7a, 0x9a, 0x7f, 0x4b, } } /*ddfa9f7c-f09e-40b9-8c1a-be877a9a7f4b*/, LOGTEXT("Windows Server 2008 Web"),                        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008A },

		// Windows Server 2008 B (Standard and Enterprise)
		{ { 0xc1af4d90, 0xd1bc, 0x44ca, { 0x85, 0xd4, 0x00, 0x3b, 0xa3, 0x3d, 0xb3, 0xb9, } } /*c1af4d90-d1bc-44ca-85d4-003ba33db3b9*/, LOGTEXT("Windows Server 2008 Enterprise"),                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008B },
		{ { 0x8198490a, 0xadd0, 0x47b2, { 0xb3, 0xba, 0x31, 0x6b, 0x12, 0xd6, 0x47, 0xb4, } } /*8198490a-add0-47b2-b3ba-316b12d647b4*/, LOGTEXT("Windows Server 2008 Enterprise without Hyper-V"), EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008B },
		{ { 0xad2542d4, 0x9154, 0x4c6d, { 0x8a, 0x44, 0x30, 0xf1, 0x1e, 0xe9, 0x69, 0x89, } } /*ad2542d4-9154-4c6d-8a44-30f11ee96989*/, LOGTEXT("Windows Server 2008 Standard"),                   EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008B },
		{ { 0x2401e3d0, 0xc50a, 0x4b58, { 0x87, 0xb2, 0x7e, 0x79, 0x4b, 0x7d, 0x26, 0x07, } } /*2401e3d0-c50a-4b58-87b2-7e794b7d2607*/, LOGTEXT("Windows Server 2008 Standard without Hyper-V"),   EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008B },

		// Windows Server 2008 C (Datacenter)
		{ { 0x68b6e220, 0xcf09, 0x466b, { 0x92, 0xd3, 0x45, 0xcd, 0x96, 0x4b, 0x95, 0x09, } } /*68b6e220-cf09-466b-92d3-45cd964b9509*/, LOGTEXT("Windows Server 2008 Datacenter"),                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008C },
		{ { 0xfd09ef77, 0x5647, 0x4eff, { 0x80, 0x9c, 0xaf, 0x2b, 0x64, 0x65, 0x9a, 0x45, } } /*fd09ef77-5647-4eff-809c-af2b64659a45*/, LOGTEXT("Windows Server 2008 Datacenter without Hyper-V"), EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008C },
		{ { 0x01ef176b, 0x3e0d, 0x422a, { 0xb4, 0xf8, 0x4e, 0xa8, 0x80, 0x03, 0x5e, 0x8f, } } /*01ef176b-3e0d-422a-b4f8-4ea880035e8f*/, LOGTEXT("Windows Server 2008 for Itanium"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008C },

		// Windows Server 2008 R2 A (Web and HPC)
		{ { 0xf772515c, 0x0e87, 0x48d5, { 0xa6, 0x76, 0xe6, 0x96, 0x2c, 0x3e, 0x11, 0x95, } } /*f772515c-0e87-48d5-a676-e6962c3e1195*/, LOGTEXT("Windows MultiPoint Server 2010"),                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2A },
		{ { 0xcda18cf3, 0xc196, 0x46ad, { 0xb2, 0x89, 0x60, 0xc0, 0x72, 0x86, 0x99, 0x94, } } /*cda18cf3-c196-46ad-b289-60c072869994*/, LOGTEXT("Windows Server 2008 R2 HPC Edition"),             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2A },
		{ { 0xa78b8bd9, 0x8017, 0x4df5, { 0xb8, 0x6a, 0x09, 0xf7, 0x56, 0xaf, 0xfa, 0x7c, } } /*a78b8bd9-8017-4df5-b86a-09f756affa7c*/, LOGTEXT("Windows Server 2008 R2 Web"),                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2A },

		// Windows Server 2008 R2 B (Standard and Enterprise)
		{ { 0x620e2b3d, 0x09e7, 0x42fd, { 0x80, 0x2a, 0x17, 0xa1, 0x36, 0x52, 0xfe, 0x7a, } } /*620e2b3d-09e7-42fd-802a-17a13652fe7a*/, LOGTEXT("Windows Server 2008 R2 Enterprise"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2B },
		{ { 0x68531fb9, 0x5511, 0x4989, { 0x97, 0xbe, 0xd1, 0x1a, 0x0f, 0x55, 0x63, 0x3f, } } /*68531fb9-5511-4989-97be-d11a0f55633f*/, LOGTEXT("Windows Server 2008 R2 Standard"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2B },

		// Windows Server 2008 R2 C (Datacenter)
		{ { 0x7482e61b, 0xc589, 0x4b7f, { 0x8e, 0xcc, 0x46, 0xd4, 0x55, 0xac, 0x3b, 0x87, } } /*7482e61b-c589-4b7f-8ecc-46d455ac3b87*/, LOGTEXT("Windows Server 2008 R2 Datacenter"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2C },
		{ { 0x8a26851c, 0x1c7e, 0x48d3, { 0xa6, 0x87, 0xfb, 0xca, 0x9b, 0x9a, 0xc1, 0x6b, } } /*8a26851c-1c7e-48d3-a687-fbca9b9ac16b*/, LOGTEXT("Windows Server 2008 R2 for Itanium Enterprise"),  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2C },

		// Windows Server 2012
		{ { 0xd3643d60, 0x0c42, 0x412d, { 0xa7, 0xd6, 0x52, 0xe6, 0x63, 0x53, 0x27, 0xf6, } } /*d3643d60-0c42-412d-a7d6-52e6635327f6*/, LOGTEXT("Windows Server 2012 Datacenter"),                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012 },
		{ { 0x95fd1c83, 0x7df5, 0x494a, { 0xbe, 0x8b, 0x13, 0x00, 0xe1, 0xc9, 0xd1, 0xcd, } } /*95fd1c83-7df5-494a-be8b-1300e1c9d1cd*/, LOGTEXT("Windows Server 2012 MultiPoint Premium"),         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012 },
		{ { 0x7d5486c7, 0xe120, 0x4771, { 0xb7, 0xf1, 0x7b, 0x56, 0xc6, 0xd3, 0x17, 0x0c, } } /*7d5486c7-e120-4771-b7f1-7b56c6d3170c*/, LOGTEXT("Windows Server 2012 MultiPoint Standard"),        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012 },
		{ { 0xf0f5ec41, 0x0d55, 0x4732, { 0xaf, 0x02, 0x44, 0x0a, 0x44, 0xa3, 0xcf, 0x0f, } } /*f0f5ec41-0d55-4732-af02-440a44a3cf0f*/, LOGTEXT("Windows Server 2012 Standard"),                   EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012 },

		// Windows Server 2012 R2
		{ { 0xb743a2be, 0x68d4, 0x4dd3, { 0xaf, 0x32, 0x92, 0x42, 0x5b, 0x7b, 0xb6, 0x23, } } /*b743a2be-68d4-4dd3-af32-92425b7bb623*/, LOGTEXT("Windows Server 2012 R2 Cloud Storage"),           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012R2 },
		{ { 0x00091344, 0x1ea4, 0x4f37, { 0xb7, 0x89, 0x01, 0x75, 0x0b, 0xa6, 0x98, 0x8c, } } /*00091344-1ea4-4f37-b789-01750ba6988c*/, LOGTEXT("Windows Server 2012 R2 Datacenter"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012R2 },
		{ { 0x21db6ba4, 0x9a7b, 0x4a14, { 0x9e, 0x29, 0x64, 0xa6, 0x0c, 0x59, 0x30, 0x1d, } } /*21db6ba4-9a7b-4a14-9e29-64a60c59301d*/, LOGTEXT("Windows Server 2012 R2 Essentials"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012R2 },
		{ { 0xb3ca044e, 0xa358, 0x4d68, { 0x98, 0x83, 0xaa, 0xa2, 0x94, 0x1a, 0xca, 0x99, } } /*b3ca044e-a358-4d68-9883-aaa2941aca99*/, LOGTEXT("Windows Server 2012 R2 Standard"),                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012R2 },

		// Windows Server 2016
		{ { 0x7b4433f4, 0xb1e7, 0x4788, { 0x89, 0x5a, 0xc4, 0x53, 0x78, 0xd3, 0x82, 0x53, } } /*7b4433f4-b1e7-4788-895a-c45378d38253*/, LOGTEXT("Windows Server 2016 Azure Core"),                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2016 },
		{ { 0x3dbf341b, 0x5f6c, 0x4fa7, { 0xb9, 0x36, 0x69, 0x9d, 0xce, 0x9e, 0x26, 0x3f, } } /*3dbf341b-5f6c-4fa7-b936-699dce9e263f*/, LOGTEXT("Windows Server 2016 Cloud Storage"),              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2016 },
		{ { 0x21c56779, 0xb449, 0x4d20, { 0xad, 0xfc, 0xee, 0xce, 0x0e, 0x1a, 0xd7, 0x4b, } } /*21c56779-b449-4d20-adfc-eece0e1ad74b*/, LOGTEXT("Windows Server 2016 Datacenter"),                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2016 },
		{ { 0x2b5a1b0f, 0xa5ab, 0x4c54, { 0xac, 0x2f, 0xa6, 0xd9, 0x48, 0x24, 0xa2, 0x83, } } /*2b5a1b0f-a5ab-4c54-ac2f-a6d94824a283*/, LOGTEXT("Windows Server 2016 Essentials"),                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2016 },
		{ { 0x8c1c5410, 0x9f39, 0x4805, { 0x8c, 0x9d, 0x63, 0xa0, 0x77, 0x06, 0x35, 0x8f, } } /*8c1c5410-9f39-4805-8c9d-63a07706358f*/, LOGTEXT("Windows Server 2016 Standard"),                   EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2016 },

		// Windows Server Preview
	#	ifdef INCLUDE_BETAS
		{ { 0xba947c44, 0xd19d, 0x4786, { 0xb6, 0xae, 0x22, 0x77, 0x0b, 0xc9, 0x4c, 0x54, } } /*ba947c44-d19d-4786-b6ae-22770bc94c54*/, LOGTEXT("Windows Server 2016 Datacenter Preview"),         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_SRV_BETA },
	#	endif // INCLUDE_BETAS

		// Windows Vista
		{ { 0x4f3d1606, 0x3fea, 0x4c01, { 0xbe, 0x3c, 0x8d, 0x67, 0x1c, 0x40, 0x1e, 0x3b, } } /*4f3d1606-3fea-4c01-be3c-8d671c401e3b*/, LOGTEXT("Windows Vista Business"),                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_VISTA },
		{ { 0x2c682dc2, 0x8b68, 0x4f63, { 0xa1, 0x65, 0xae, 0x29, 0x1d, 0x4c, 0xf1, 0x38, } } /*2c682dc2-8b68-4f63-a165-ae291d4cf138*/, LOGTEXT("Windows Vista Business N"),                       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_VISTA },
		{ { 0xcfd8ff08, 0xc0d7, 0x452b, { 0x9f, 0x60, 0xef, 0x5c, 0x70, 0xc3, 0x20, 0x94, } } /*cfd8ff08-c0d7-452b-9f60-ef5c70c32094*/, LOGTEXT("Windows Vista Enterprise"),                       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_VISTA },
		{ { 0xd4f54950, 0x26f2, 0x4fb4, { 0xba, 0x21, 0xff, 0xab, 0x16, 0xaf, 0xca, 0xde, } } /*d4f54950-26f2-4fb4-ba21-ffab16afcade*/, LOGTEXT("Windows Vista Enterprise N"),                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_VISTA },

		// Office 2010
		{ { 0x8ce7e872, 0x188c, 0x4b98, { 0x9d, 0x90, 0xf8, 0xf9, 0x0b, 0x7a, 0xad, 0x02, } } /*8ce7e872-188c-4b98-9d90-f8f90b7aad02*/, LOGTEXT("Office Access 2010"),                             EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xcee5d470, 0x6e3b, 0x4fcc, { 0x8c, 0x2b, 0xd1, 0x74, 0x28, 0x56, 0x8a, 0x9f, } } /*cee5d470-6e3b-4fcc-8c2b-d17428568a9f*/, LOGTEXT("Office Excel 2010"),                              EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x8947d0b8, 0xc33b, 0x43e1, { 0x8c, 0x56, 0x9b, 0x67, 0x4c, 0x05, 0x28, 0x32, } } /*8947d0b8-c33b-43e1-8c56-9b674c052832*/, LOGTEXT("Office Groove 2010"),                             EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xca6b6639, 0x4ad6, 0x40ae, { 0xa5, 0x75, 0x14, 0xde, 0xe0, 0x7f, 0x64, 0x30, } } /*ca6b6639-4ad6-40ae-a575-14dee07f6430*/, LOGTEXT("Office InfoPath 2010"),                           EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x09ed9640, 0xf020, 0x400a, { 0xac, 0xd8, 0xd7, 0xd8, 0x67, 0xdf, 0xd9, 0xc2, } } /*09ed9640-f020-400a-acd8-d7d867dfd9c2*/, LOGTEXT("Office Mondo 1 2010"),                            EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xef3d4e49, 0xa53d, 0x4d81, { 0xa2, 0xb1, 0x2c, 0xa6, 0xc2, 0x55, 0x6b, 0x2c, } } /*ef3d4e49-a53d-4d81-a2b1-2ca6c2556b2c*/, LOGTEXT("Office Mondo 2 2010"),                            EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xab586f5c, 0x5256, 0x4632, { 0x96, 0x2f, 0xfe, 0xfd, 0x8b, 0x49, 0xe6, 0xf4, } } /*ab586f5c-5256-4632-962f-fefd8b49e6f4*/, LOGTEXT("Office OneNote 2010"),                            EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xecb7c192, 0x73ab, 0x4ded, { 0xac, 0xf4, 0x23, 0x99, 0xb0, 0x95, 0xd0, 0xcc, } } /*ecb7c192-73ab-4ded-acf4-2399b095d0cc*/, LOGTEXT("Office OutLook 2010"),                            EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x45593b1d, 0xdfb1, 0x4e91, { 0xbb, 0xfb, 0x2d, 0x5d, 0x0c, 0xe2, 0x22, 0x7a, } } /*45593b1d-dfb1-4e91-bbfb-2d5d0ce2227a*/, LOGTEXT("Office PowerPoint 2010"),                         EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x6f327760, 0x8c5c, 0x417c, { 0x9b, 0x61, 0x83, 0x6a, 0x98, 0x28, 0x7e, 0x0c, } } /*6f327760-8c5c-417c-9b61-836a98287e0c*/, LOGTEXT("Office Professional Plus 2010"),                  EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xdf133ff7, 0xbf14, 0x4f95, { 0xaf, 0xe3, 0x7b, 0x48, 0xe7, 0xe3, 0x31, 0xef, } } /*df133ff7-bf14-4f95-afe3-7b48e7e331ef*/, LOGTEXT("Office Project Pro 2010"),                        EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x5dc7bf61, 0x5ec9, 0x4996, { 0x9c, 0xcb, 0xdf, 0x80, 0x6a, 0x2d, 0x0e, 0xfe, } } /*5dc7bf61-5ec9-4996-9ccb-df806a2d0efe*/, LOGTEXT("Office Project Standard 2010"),                   EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xb50c4f75, 0x599b, 0x43e8, { 0x8d, 0xcd, 0x10, 0x81, 0xa7, 0x96, 0x72, 0x41, } } /*b50c4f75-599b-43e8-8dcd-1081a7967241*/, LOGTEXT("Office Publisher 2010"),                          EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xea509e87, 0x07a1, 0x4a45, { 0x9e, 0xdc, 0xeb, 0xa5, 0xa3, 0x9f, 0x36, 0xaf, } } /*ea509e87-07a1-4a45-9edc-eba5a39f36af*/, LOGTEXT("Office Small Business Basics 2010"),              EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x9da2a678, 0xfb6b, 0x4e67, { 0xab, 0x84, 0x60, 0xdd, 0x6a, 0x9c, 0x81, 0x9a, } } /*9da2a678-fb6b-4e67-ab84-60dd6a9c819a*/, LOGTEXT("Office Standard 2010"),                           EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x92236105, 0xbb67, 0x494f, { 0x94, 0xc7, 0x7f, 0x7a, 0x60, 0x79, 0x29, 0xbd, } } /*92236105-bb67-494f-94c7-7f7a607929bd*/, LOGTEXT("Office Visio Premium 2010"),                      EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0xe558389c, 0x83c3, 0x4b29, { 0xad, 0xfe, 0x5e, 0x4d, 0x7f, 0x46, 0xc3, 0x58, } } /*e558389c-83c3-4b29-adfe-5e4d7f46c358*/, LOGTEXT("Office Visio Pro 2010"),                          EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x9ed833ff, 0x4f92, 0x4f36, { 0xb3, 0x70, 0x86, 0x83, 0xa4, 0xf1, 0x32, 0x75, } } /*9ed833ff-4f92-4f36-b370-8683a4f13275*/, LOGTEXT("Office Visio Standard 2010"),                     EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
		{ { 0x2d0882e7, 0xa4e7, 0x423b, { 0x8c, 0xcc, 0x70, 0xd9, 0x1e, 0x01, 0x58, 0xb1, } } /*2d0882e7-a4e7-423b-8ccc-70d91e0158b1*/, LOGTEXT("Office Word 2010"),                               EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },

		// Office 2013
		{ { 0x6ee7622c, 0x18d8, 0x4005, { 0x9f, 0xb7, 0x92, 0xdb, 0x64, 0x4a, 0x27, 0x9b, } } /*6ee7622c-18d8-4005-9fb7-92db644a279b*/, LOGTEXT("Office Access 2013"),                             EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xf7461d52, 0x7c2b, 0x43b2, { 0x87, 0x44, 0xea, 0x95, 0x8e, 0x0b, 0xd0, 0x9a, } } /*f7461d52-7c2b-43b2-8744-ea958e0bd09a*/, LOGTEXT("Office Excel 2013"),                              EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xa30b8040, 0xd68a, 0x423f, { 0xb0, 0xb5, 0x9c, 0xe2, 0x92, 0xea, 0x5a, 0x8f, } } /*a30b8040-d68a-423f-b0b5-9ce292ea5a8f*/, LOGTEXT("Office InfoPath 2013"),                           EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0x1b9f11e3, 0xc85c, 0x4e1b, { 0xbb, 0x29, 0x87, 0x9a, 0xd2, 0xc9, 0x09, 0xe3, } } /*1b9f11e3-c85c-4e1b-bb29-879ad2c909e3*/, LOGTEXT("Office Lync 2013"),                               EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xdc981c6b, 0xfc8e, 0x420f, { 0xaa, 0x43, 0xf8, 0xf3, 0x3e, 0x5c, 0x09, 0x23, } } /*dc981c6b-fc8e-420f-aa43-f8f33e5c0923*/, LOGTEXT("Office Mondo 2013"),                              EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xefe1f3e6, 0xaea2, 0x4144, { 0xa2, 0x08, 0x32, 0xaa, 0x87, 0x2b, 0x65, 0x45, } } /*efe1f3e6-aea2-4144-a208-32aa872b6545*/, LOGTEXT("Office OneNote 2013"),                            EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0x771c3afa, 0x50c5, 0x443f, { 0xb1, 0x51, 0xff, 0x25, 0x46, 0xd8, 0x63, 0xa0, } } /*771c3afa-50c5-443f-b151-ff2546d863a0*/, LOGTEXT("Office OutLook 2013"),                            EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0x8c762649, 0x97d1, 0x4953, { 0xad, 0x27, 0xb7, 0xe2, 0xc2, 0x5b, 0x97, 0x2e, } } /*8c762649-97d1-4953-ad27-b7e2c25b972e*/, LOGTEXT("Office PowerPoint 2013"),                         EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xb322da9c, 0xa2e2, 0x4058, { 0x9e, 0x4e, 0xf5, 0x9a, 0x69, 0x70, 0xbd, 0x69, } } /*b322da9c-a2e2-4058-9e4e-f59a6970bd69*/, LOGTEXT("Office Professional Plus 2013"),                  EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0x4a5d124a, 0xe620, 0x44ba, { 0xb6, 0xff, 0x65, 0x89, 0x61, 0xb3, 0x3b, 0x9a, } } /*4a5d124a-e620-44ba-b6ff-658961b33b9a*/, LOGTEXT("Office Project Pro 2013"),                        EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0x427a28d1, 0xd17c, 0x4abf, { 0xb7, 0x17, 0x32, 0xc7, 0x80, 0xba, 0x6f, 0x07, } } /*427a28d1-d17c-4abf-b717-32c780ba6f07*/, LOGTEXT("Office Project Standard 2013"),                   EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0x00c79ff1, 0x6850, 0x443d, { 0xbf, 0x61, 0x71, 0xcd, 0xe0, 0xde, 0x30, 0x5f, } } /*00c79ff1-6850-443d-bf61-71cde0de305f*/, LOGTEXT("Office Publisher 2013"),                          EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xb13afb38, 0xcd79, 0x4ae5, { 0x9f, 0x7f, 0xee, 0xd0, 0x58, 0xd7, 0x50, 0xca, } } /*b13afb38-cd79-4ae5-9f7f-eed058d750ca*/, LOGTEXT("Office Standard 2013"),                           EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xe13ac10e, 0x75d0, 0x4aff, { 0xa0, 0xcd, 0x76, 0x49, 0x82, 0xcf, 0x54, 0x1c, } } /*e13ac10e-75d0-4aff-a0cd-764982cf541c*/, LOGTEXT("Office Visio Pro 2013"),                          EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xac4efaf0, 0xf81f, 0x4f61, { 0xbd, 0xf7, 0xea, 0x32, 0xb0, 0x2a, 0xb1, 0x17, } } /*ac4efaf0-f81f-4f61-bdf7-ea32b02ab117*/, LOGTEXT("Office Visio Standard 2013"),                     EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
		{ { 0xd9f5b1c6, 0x5386, 0x495a, { 0x88, 0xf9, 0x9a, 0xd6, 0xb4, 0x1a, 0xc9, 0xb3, } } /*d9f5b1c6-5386-495a-88f9-9ad6b41ac9b3*/, LOGTEXT("Office Word 2013"),                               EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },

		// Office 2013 (Pre-Release)
	#	ifdef INCLUDE_BETAS
		{ { 0x44b538e2, 0xfb34, 0x4732, { 0x81, 0xe4, 0x64, 0x4c, 0x17, 0xd2, 0xe7, 0x46, } } /*44b538e2-fb34-4732-81e4-644c17d2e746*/, LOGTEXT("Office Access 2013 (Pre-Release)"),               EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x9373bfa0, 0x97b3, 0x4587, { 0xab, 0x73, 0x30, 0x93, 0x44, 0x61, 0xd5, 0x5c, } } /*9373bfa0-97b3-4587-ab73-30934461d55c*/, LOGTEXT("Office Excel 2013 (Pre-Release)"),                EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0xaa286eb4, 0x556f, 0x4eeb, { 0x96, 0x7c, 0xc1, 0xb7, 0x71, 0xb7, 0x67, 0x3e, } } /*aa286eb4-556f-4eeb-967c-c1b771b7673e*/, LOGTEXT("Office Groove 2013 (Pre-Release)"),               EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x7ccc8256, 0xfbaa, 0x49c6, { 0xb2, 0xa9, 0xf5, 0xaf, 0xb4, 0x25, 0x7c, 0xd2, } } /*7ccc8256-fbaa-49c6-b2a9-f5afb4257cd2*/, LOGTEXT("Office InfoPath 2013 (Pre-Release)"),             EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0xc53dfe17, 0xcc00, 0x4967, { 0xb1, 0x88, 0xa0, 0x88, 0xa9, 0x65, 0x49, 0x4d, } } /*c53dfe17-cc00-4967-b188-a088a965494d*/, LOGTEXT("Office Lync 2013 (Pre-Release)"),                 EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x2816a87d, 0xe1ed, 0x4097, { 0xb3, 0x11, 0xe2, 0x34, 0x1c, 0x57, 0xb1, 0x79, } } /*2816a87d-e1ed-4097-b311-e2341c57b179*/, LOGTEXT("Office Mondo 2013 (Pre-Release)"),                EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x67c0f908, 0x184f, 0x4f64, { 0x82, 0x50, 0x12, 0xdb, 0x79, 0x7a, 0xb3, 0xc3, } } /*67c0f908-184f-4f64-8250-12db797ab3c3*/, LOGTEXT("Office OneNote 2013 (Pre-Release)"),              EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x7bce4e7a, 0xdd80, 0x4682, { 0x98, 0xfa, 0xf9, 0x93, 0x72, 0x58, 0x03, 0xd2, } } /*7bce4e7a-dd80-4682-98fa-f993725803d2*/, LOGTEXT("Office Outlook 2013 (Pre-Release)"),              EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x1ec10c0a, 0x54f6, 0x453e, { 0xb8, 0x5a, 0x6f, 0xa1, 0xbb, 0xfe, 0xa9, 0xb7, } } /*1ec10c0a-54f6-453e-b85a-6fa1bbfea9b7*/, LOGTEXT("Office PowerPoint 2013 (Pre-Release)"),           EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x87d2b5bf, 0xd47b, 0x41fb, { 0xaf, 0x62, 0x71, 0xc3, 0x82, 0xf5, 0xcc, 0x85, } } /*87d2b5bf-d47b-41fb-af62-71c382f5cc85*/, LOGTEXT("Office Professional Plus 2013 (Pre-Release)"),    EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x3cfe50a9, 0x0e03, 0x4b29, { 0x97, 0x54, 0x9f, 0x19, 0x3f, 0x07, 0xb7, 0x1f, } } /*3cfe50a9-0e03-4b29-9754-9f193f07b71f*/, LOGTEXT("Office Project Pro 2013 (Pre-Release)"),          EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x39e49e57, 0xae68, 0x4ee3, { 0xb0, 0x98, 0x26, 0x48, 0x0d, 0xf3, 0xda, 0x96, } } /*39e49e57-ae68-4ee3-b098-26480df3da96*/, LOGTEXT("Office Project Standard 2013 (Pre-Release)"),     EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x15aa2117, 0x8f79, 0x49a8, { 0x83, 0x17, 0x75, 0x30, 0x26, 0xd6, 0xa0, 0x54, } } /*15aa2117-8f79-49a8-8317-753026d6a054*/, LOGTEXT("Office Publisher 2013 (Pre-Release)"),            EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0xcfbfd60e, 0x0b5f, 0x427d, { 0x91, 0x7c, 0xa4, 0xdf, 0x42, 0xa8, 0x0e, 0x44, } } /*cfbfd60e-0b5f-427d-917c-a4df42a80e44*/, LOGTEXT("Office Visio Pro 2013 (Pre-Release)"),            EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0x7012cc81, 0x8887, 0x42e9, { 0xb1, 0x7d, 0x4e, 0x5e, 0x42, 0x76, 0x0f, 0x0d, } } /*7012cc81-8887-42e9-b17d-4e5e42760f0d*/, LOGTEXT("Office Visio Standard 2013 (Pre-Release)"),       EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
		{ { 0xde9c7eb6, 0x5a85, 0x420d, { 0x97, 0x03, 0xff, 0xf1, 0x1b, 0xdd, 0x4d, 0x43, } } /*de9c7eb6-5a85-420d-9703-fff11bdd4d43*/, LOGTEXT("Office Word 2013 (Pre-Release)"),                 EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013_BETA },
	#	endif // INCLUDE_BETAS

		// Office 2016
		{ { 0x67c0fc0c, 0xdeba, 0x401b, { 0xbf, 0x8b, 0x9c, 0x8a, 0xd8, 0x39, 0x58, 0x04, } } /*67c0fc0c-deba-401b-bf8b-9c8ad8395804*/, LOGTEXT("Office Access 2016"),                             EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xc3e65d36, 0x141f, 0x4d2f, { 0xa3, 0x03, 0xa8, 0x42, 0xee, 0x75, 0x6a, 0x29, } } /*c3e65d36-141f-4d2f-a303-a842ee756a29*/, LOGTEXT("Office Excel 2016"),                              EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0x9caabccb, 0x61b1, 0x4b4b, { 0x8b, 0xec, 0xd1, 0x0a, 0x3c, 0x3a, 0xc2, 0xce, } } /*9caabccb-61b1-4b4b-8bec-d10a3c3ac2ce*/, LOGTEXT("Office Mondo 2016"),                              EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xe914ea6e, 0xa5fa, 0x4439, { 0xa3, 0x94, 0xa9, 0xbb, 0x32, 0x93, 0xca, 0x09, } } /*e914ea6e-a5fa-4439-a394-a9bb3293ca09*/, LOGTEXT("Office Mondo R 2016"),                            EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xd8cace59, 0x33d2, 0x4ac7, { 0x9b, 0x1b, 0x9b, 0x72, 0x33, 0x9c, 0x51, 0xc8, } } /*d8cace59-33d2-4ac7-9b1b-9b72339c51c8*/, LOGTEXT("Office OneNote 2016"),                            EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xec9d9265, 0x9d1e, 0x4ed0, { 0x83, 0x8a, 0xcd, 0xc2, 0x0f, 0x25, 0x51, 0xa1, } } /*ec9d9265-9d1e-4ed0-838a-cdc20f2551a1*/, LOGTEXT("Office Outlook 2016"),                            EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xd70b1bba, 0xb893, 0x4544, { 0x96, 0xe2, 0xb7, 0xa3, 0x18, 0x09, 0x1c, 0x33, } } /*d70b1bba-b893-4544-96e2-b7a318091c33*/, LOGTEXT("Office Powerpoint 2016"),                         EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xd450596f, 0x894d, 0x49e0, { 0x96, 0x6a, 0xfd, 0x39, 0xed, 0x4c, 0x4c, 0x64, } } /*d450596f-894d-49e0-966a-fd39ed4c4c64*/, LOGTEXT("Office Professional Plus 2016"),                  EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0x4f414197, 0x0fc2, 0x4c01, { 0xb6, 0x8a, 0x86, 0xcb, 0xb9, 0xac, 0x25, 0x4c, } } /*4f414197-0fc2-4c01-b68a-86cbb9ac254c*/, LOGTEXT("Office Project Pro 2016"),                        EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0x829b8110, 0x0e6f, 0x4349, { 0xbc, 0xa4, 0x42, 0x80, 0x35, 0x77, 0x78, 0x8d, } } /*829b8110-0e6f-4349-bca4-42803577788d*/, LOGTEXT("Office Project Pro 2016 XC2R"),                   EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xda7ddabc, 0x3fbe, 0x4447, { 0x9e, 0x01, 0x6a, 0xb7, 0x44, 0x0b, 0x4c, 0xd4, } } /*da7ddabc-3fbe-4447-9e01-6ab7440b4cd4*/, LOGTEXT("Office Project Standard 2016"),                   EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xcbbaca45, 0x556a, 0x4416, { 0xad, 0x03, 0xbd, 0xa5, 0x98, 0xea, 0xa7, 0xc8, } } /*cbbaca45-556a-4416-ad03-bda598eaa7c8*/, LOGTEXT("Office Project Standard 2016 XC2R"),              EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0x041a06cb, 0xc5b8, 0x4772, { 0x80, 0x9f, 0x41, 0x6d, 0x03, 0xd1, 0x66, 0x54, } } /*041a06cb-c5b8-4772-809f-416d03d16654*/, LOGTEXT("Office Publisher 2016"),                          EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0x83e04ee1, 0xfa8d, 0x436d, { 0x89, 0x94, 0xd3, 0x1a, 0x86, 0x2c, 0xab, 0x77, } } /*83e04ee1-fa8d-436d-8994-d31a862cab77*/, LOGTEXT("Office Skype for Business 2016"),                 EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xdedfa23d, 0x6ed1, 0x45a6, { 0x85, 0xdc, 0x63, 0xca, 0xe0, 0x54, 0x6d, 0xe6, } } /*dedfa23d-6ed1-45a6-85dc-63cae0546de6*/, LOGTEXT("Office Standard 2016"),                           EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0x6bf301c1, 0xb94a, 0x43e9, { 0xba, 0x31, 0xd4, 0x94, 0x59, 0x8c, 0x47, 0xfb, } } /*6bf301c1-b94a-43e9-ba31-d494598c47fb*/, LOGTEXT("Office Visio Pro 2016"),                          EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xb234abe3, 0x0857, 0x4f9c, { 0xb0, 0x5a, 0x4d, 0xc3, 0x14, 0xf8, 0x55, 0x57, } } /*b234abe3-0857-4f9c-b05a-4dc314f85557*/, LOGTEXT("Office Visio Pro 2016 XC2R"),                     EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xaa2a7821, 0x1827, 0x4c2c, { 0x8f, 0x1d, 0x45, 0x13, 0xa3, 0x4d, 0xda, 0x97, } } /*aa2a7821-1827-4c2c-8f1d-4513a34dda97*/, LOGTEXT("Office Visio Standard 2016"),                     EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0x361fe620, 0x64f4, 0x41b5, { 0xba, 0x77, 0x84, 0xf8, 0xe0, 0x79, 0xb1, 0xf7, } } /*361fe620-64f4-41b5-ba77-84f8e079b1f7*/, LOGTEXT("Office Visio Standard 2016 XC2R"),                EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
		{ { 0xbb11badf, 0xd8aa, 0x470e, { 0x93, 0x11, 0x20, 0xea, 0xf8, 0x0f, 0xe5, 0xcc, } } /*bb11badf-d8aa-470e-9311-20eaf80fe5cc*/, LOGTEXT("Office Word 2016"),                               EPID_OFFICE2016, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },

	// End marker (necessity should be removed when time permits)

	{ { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, NULL, NULL, 0, 0 }
};


// necessary because other .c files cannot access _countof()
__pure ProdListIndex_t getExtendedProductListSize(void)
{
	return _countof(ExtendedProductList) - 1;
}

#ifndef NO_LOG
__pure ProdListIndex_t getAppListSize(void)
{
	return _countof(AppList);
}
#endif // NO_LOG

#endif // NO_EXTENDED_PRODUCT_LIST
#endif // IS_LIBRARY


#ifndef NO_RANDOM_EPID
// HostType and OSBuild
static const struct KMSHostOS { uint16_t Type; uint16_t Build; } HostOS[] =
{
	{ 55041, 6002 }, // Windows Server 2008 SP2
    { 55041, 7601 }, // Windows Server 2008 R2 SP1
    {  5426, 9200 }, // Windows Server 2012
    {  6401, 9600 }, // Windows Server 2012 R2
	{  3612, 14393 }, // Windows Server 2016
};

// GroupID and PIDRange
static const struct PKEYCONFIG { uint16_t GroupID; uint32_t RangeMin; uint32_t RangeMax; } pkeyconfig[] = {
    { 206, 471000000, 530999999 }, // Windows Server 2016
    {  96, 199000000, 217999999 }, // Office2010
    { 206, 234000000, 255999999 }, // Office2013
    { 206, 437000000, 458999999 }, // Office2016
};

// Valid language identifiers to be used in the ePID
static const uint16_t LcidList[] = {
	1078, 1052, 1025, 2049, 3073, 4097, 5121, 6145, 7169, 8193, 9217, 10241, 11265, 12289, 13313, 14337, 15361, 16385,
	1067, 1068, 2092, 1069, 1059, 1093, 5146, 1026, 1027, 1028, 2052, 3076, 4100, 5124, 1050, 4122, 1029, 1030, 1125, 1043, 2067,
	1033, 2057, 3081, 4105, 5129, 6153, 7177, 8201, 9225, 10249, 11273, 12297, 13321, 1061, 1080, 1065, 1035, 1036, 2060,
	3084, 4108, 5132, 6156, 1079, 1110, 1031, 2055, 3079, 4103, 5127, 1032, 1095, 1037, 1081, 1038, 1039, 1057, 1040, 2064, 1041, 1099,
	1087, 1111, 1042, 1088, 1062, 1063, 1071, 1086, 2110, 1100, 1082, 1153, 1102, 1104, 1044, 2068, 1045, 1046, 2070,
	1094, 1131, 2155, 3179, 1048, 1049, 9275, 4155, 5179, 3131, 1083, 2107, 8251, 6203, 7227, 1103, 2074, 6170, 3098,
	7194, 1051, 1060, 1034, 2058, 3082, 4106, 5130, 6154, 7178, 8202, 9226, 10250, 11274, 12298, 13322, 14346, 15370, 16394,
	17418, 18442, 19466, 20490, 1089, 1053, 2077, 1114, 1097, 1092, 1098, 1054, 1074, 1058, 1056, 1091, 2115, 1066, 1106, 1076, 1077
};


#ifdef _PEDANTIC
uint16_t IsValidLcid(const uint16_t Lcid)
{
	uint16_t i;

	for (i = 0; i < _countof(LcidList); i++)
	{
		if (Lcid == LcidList[i]) return Lcid;
	}

	return 0;
}
#endif // _PEDANTIC
#endif // NO_RANDOM_EPID


// Unix time is seconds from 1970-01-01. Should be 64 bits to avoid Year 2035 overflow bug.
// FILETIME is 100 nanoseconds from 1601-01-01. Must be 64 bits.
void getUnixTimeAsFileTime(FILETIME *const ts)
{
	int64_t unixtime = (int64_t)time(NULL);
	int64_t *filetime = (int64_t*)ts;

	*filetime = LE64( (unixtime + 11644473600LL) * 10000000LL );
}

__pure int64_t fileTimeToUnixTime(const FILETIME *const ts)
{
	return LE64( *((const int64_t *const)ts) ) / 10000000LL - 11644473600LL;
}


/*
 * Get's a product name with a GUID in host-endian order.
 * List can be any list defined above.
 */
const char* getProductNameHE(const GUID *const guid, const KmsIdList *const List, ProdListIndex_t *const i)
{
	for (*i = 0; List[*i].name != NULL; (*i)++)
	{
		if (IsEqualGUID(guid, &List[*i].guid))
			return List[*i].name;
	}

	return "Unknown";
}


/*
 * same as getProductnameHE except GUID is in little-endian (network) order
 */
const char* getProductNameLE(const GUID *const guid, const KmsIdList *const List, ProdListIndex_t *const i)
{
	#if __BYTE_ORDER != __LITTLE_ENDIAN
	GUID HeGUID;
	LEGUID(&HeGUID, guid);
	return getProductNameHE(&HeGUID, List, i);
	#else
	return getProductNameHE(guid, List, i);
	#endif
}


#ifndef NO_RANDOM_EPID
// formats an int with a fixed number of digits with leading zeros (helper for ePID generation)
static char* itoc(char *const c, const int i, uint_fast8_t digits)
{
	char formatString[8];
	if (digits > 9) digits = 0;
	strcpy(formatString,"%");

	if (digits)
	{
		formatString[1] = '0';
		formatString[2] = digits | 0x30;
		formatString[3] = 0;
	}

	strcat(formatString, "u");
	sprintf(c, formatString, i);
	return c;
}

static int getRandomServerType()
{
#	ifndef USE_MSRPC
	if (!UseRpcBTFN)
#	endif // USE_MSRPC
	{
		// This isn't possible at all, e.g. KMS host on XP
		return rand() % (int)_countof(HostOS);
	}
#	ifndef USE_MSRPC
	else
	{
		// return 9200/9600 if NDR64 is in use, otherwise 6002/7601
		// return (rand() % 2) + (UseRpcNDR64 ? 2 : 0);
		if (UseRpcNDR64) return (rand() % 3) + 2;
		return (rand() % 2);
	}
#	endif // USE_MSRPC
}


/*
 * Generates a random ePID
 */
static void generateRandomPid(int index, char *const szPid, int serverType, int16_t lang)
{
	char numberBuffer[12];

	if (serverType < 0 || serverType >= (int)_countof(HostOS))
	{
		serverType = getRandomServerType();
	}

	strcpy(szPid, itoc(numberBuffer, HostOS[serverType].Type, 5));
	strcat(szPid, "-");

	if (index > 3) index=0;

	strcat(szPid, itoc(numberBuffer, pkeyconfig[index].GroupID, 5));
	strcat(szPid, "-");

	int keyId = (rand32() % (pkeyconfig[index].RangeMax - pkeyconfig[index].RangeMin)) + pkeyconfig[index].RangeMin;
	strcat(szPid, itoc(numberBuffer, keyId / 1000000, 3));
	strcat(szPid, "-");
	strcat(szPid, itoc(numberBuffer, keyId % 1000000, 6));
	strcat(szPid, "-03-");

	if (lang < 0) lang = LcidList[rand() % _countof(LcidList)];
	strcat(szPid, itoc(numberBuffer, lang, 0));
	strcat(szPid, "-");

	strcat(szPid, itoc(numberBuffer, HostOS[serverType].Build, 0));
	strcat(szPid, ".0000-");

#	define minTime ((time_t)1470175200) /* Release Date Win 2016 */

	time_t maxTime, kmsTime;
	time(&maxTime);

	if (maxTime < (time_t)BUILD_TIME) // Just in case the system time is < 10/17/2013 1:00 pm
		maxTime = (time_t)BUILD_TIME;

	kmsTime = (rand32() % (maxTime - minTime)) + minTime;

	struct tm *pidTime;
	pidTime = gmtime(&kmsTime);

	strcat(szPid, itoc(numberBuffer, pidTime->tm_yday, 3));
	strcat(szPid, itoc(numberBuffer, pidTime->tm_year + 1900, 4));
}


/*
 * Generates random ePIDs and stores them if not already read from ini file.
 * For use with randomization level 1
 */
void randomPidInit()
{
	ProdListIndex_t i;

	int serverType = getRandomServerType();
	int16_t lang   = Lcid ? Lcid : LcidList[rand() % _countof(LcidList)];

	for (i = 0; i < MAX_KMSAPPS; i++)
	{
		if (KmsResponseParameters[i].Epid) continue;

		char Epid[PID_BUFFER_SIZE];

		generateRandomPid(i, Epid, serverType, lang);
		KmsResponseParameters[i].Epid = (const char*)vlmcsd_malloc(strlen(Epid) + 1);

		strcpy((char*)KmsResponseParameters[i].Epid, Epid);

		#ifndef NO_LOG
		KmsResponseParameters[i].EpidSource = "randomized at program start";
		#endif // NO_LOG
	}
}

#endif // NO_RANDOM_EPID


#ifndef NO_LOG
/*
 * Logs a Request
 */
static void logRequest(const REQUEST *const baseRequest)
{
	const char *productName;
	char clientname[64];
	ProdListIndex_t index;

	#ifndef NO_EXTENDED_PRODUCT_LIST
	productName = getProductNameLE(&baseRequest->ActID, ExtendedProductList, &index);
	if (++index >= (int)_countof(ExtendedProductList))
	#endif // NO_EXTENDED_PRODUCT_LIST
	{
		productName = getProductNameLE(&baseRequest->KMSID, ProductList, &index);
		if (++index >= (int)_countof(ProductList))
		{
			productName = getProductNameLE(&baseRequest->AppID, AppList, &index);
		}
	}

	#ifndef NO_VERBOSE_LOG
	if (logverbose)
	{
		logger("<<< Incoming KMS request\n");
		logRequestVerbose(baseRequest, &logger);
	}
	else
	{
	#endif // NO_VERBOSE_LOG
		ucs2_to_utf8(baseRequest->WorkstationName, clientname, 64, 64);
		logger("KMS v%i.%i request from %s for %s\n", LE16(baseRequest->MajorVer), LE16(baseRequest->MinorVer), clientname, productName);
	#ifndef NO_VERBOSE_LOG
	}
	#endif // NO_VERBOSE_LOG
}
#endif // NO_LOG


/*
 * Converts a utf-8 ePID string to UCS-2 and writes it to a RESPONSE struct
 */
#ifndef IS_LIBRARY
static void getEpidFromString(RESPONSE *const Response, const char *const pid)
{
	size_t length = utf8_to_ucs2(Response->KmsPID, pid, PID_BUFFER_SIZE, PID_BUFFER_SIZE * 3);
	Response->PIDSize = LE32(((unsigned int)length + 1) << 1);
}


/*
 * get ePID from appropriate source
 */
static void getEpid(RESPONSE *const baseResponse, const char** EpidSource, const ProdListIndex_t index, BYTE *const HwId)
{
	const char* pid;
	if (KmsResponseParameters[index].Epid == NULL)
	{
		#ifndef NO_RANDOM_EPID
		if (RandomizationLevel == 2)
		{
			char szPid[PID_BUFFER_SIZE];
			generateRandomPid(index, szPid, -1, Lcid ? Lcid : -1);
			pid = szPid;

			#ifndef NO_LOG
			*EpidSource = "randomized on every request";
			#endif // NO_LOG
		}
		else
		#endif // NO_RANDOM_EPID
		{
			switch(index)
			{
				case EPID_INDEX_OFFICE2016:
					pid = EPID_OFFICE2016;
					break;
				case EPID_INDEX_OFFICE2013:
					pid = EPID_OFFICE2013;
					break;
				case EPID_INDEX_OFFICE2010:
					pid = EPID_OFFICE2010;
					break;
				default:
					pid = EPID_WINDOWS;
					break;
			}
			#ifndef NO_LOG
			*EpidSource = "vlmcsd default";
			#endif // NO_LOG
		}
	}
	else
	{
		pid = KmsResponseParameters[index].Epid;

		if (HwId && KmsResponseParameters[index].HwId != NULL)
			memcpy(HwId, KmsResponseParameters[index].HwId, sizeof(((RESPONSE_V6 *)0)->HwId));

		#ifndef NO_LOG
		*EpidSource = KmsResponseParameters[index].EpidSource;
		#endif // NO_LOG
	}
	getEpidFromString(baseResponse, pid);
}
#endif // IS_LIBRARY


#if !defined(NO_LOG) && defined(_PEDANTIC)
static BOOL CheckVersion4Uuid(const GUID *const guid, const char *const szGuidName)
{
	if (LE16(guid->Data3) >> 12 != 4 || guid->Data4[0] >> 6 != 2)
	{
		logger("Warning: %s does not conform to version 4 UUID according to RFC 4122\n", szGuidName);
		return FALSE;
	}
	return TRUE;
}


static void CheckRequest(const REQUEST *const Request)
{
	CheckVersion4Uuid(&Request->CMID, "Client machine ID");
	CheckVersion4Uuid(&Request->AppID, "Application ID");
	CheckVersion4Uuid(&Request->KMSID, "Server SKU ID");
	CheckVersion4Uuid(&Request->ActID, "Client SKU ID");

	if (LE32(Request->IsClientVM) > 1)
		logger("Warning: Virtual Machine field in request must be 0 or 1 but is %u\n", LE32(Request->IsClientVM));

	if (LE32(Request->LicenseStatus) > 6 )
		logger("Warning: License status must be between 0 and 6 but is %u\n", LE32(Request->LicenseStatus));
}
#endif // !defined(NO_LOG) && defined(_PEDANTIC)


#ifndef NO_LOG
/*
 * Logs the Response
 */
static void logResponse(const RESPONSE *const baseResponse, const BYTE *const hwId, const char *const EpidSource)
{
	char utf8pid[PID_BUFFER_SIZE * 3];
	ucs2_to_utf8(baseResponse->KmsPID, utf8pid, PID_BUFFER_SIZE, PID_BUFFER_SIZE * 3);

	#ifndef NO_VERBOSE_LOG
	if (!logverbose)
	{
	#endif // NO_VERBOSE_LOG
		logger("Sending ePID (%s): %s\n", EpidSource, utf8pid);
	#ifndef NO_VERBOSE_LOG
	}
	else
	{
		logger(">>> Sending response, ePID source = %s\n", EpidSource);
		logResponseVerbose(utf8pid, hwId, baseResponse, &logger);
	}
	#endif // NO_VERBOSE_LOG

}
#endif


/*
 * Creates the unencrypted base response
 */
#ifndef IS_LIBRARY
static BOOL __stdcall CreateResponseBaseCallback(const REQUEST *const baseRequest, RESPONSE *const baseResponse, BYTE *const hwId, const char* const ipstr)
{
	const char* EpidSource;
	#ifndef NO_LOG
	logRequest(baseRequest);
	#ifdef _PEDANTIC
	CheckRequest(baseRequest);
	#endif // _PEDANTIC
	#endif // NO_LOG

	ProdListIndex_t index;

	getProductNameLE(&baseRequest->KMSID, ProductList, &index);

	switch(index)
	{
		case KMS_ID_OFFICE2016:
			index = EPID_INDEX_OFFICE2016;
			break;

		case KMS_ID_OFFICE2013:
		case KMS_ID_OFFICE2013_BETA:
			index = EPID_INDEX_OFFICE2013;
			break;

		case KMS_ID_OFFICE2010:
			index = EPID_INDEX_OFFICE2010;
			break;

		default:
			index = EPID_INDEX_WINDOWS;
			break;
	}

	getEpid(baseResponse, &EpidSource, index, hwId);

	baseResponse->Version = baseRequest->Version;

	memcpy(&baseResponse->CMID, &baseRequest->CMID, sizeof(GUID));
	memcpy(&baseResponse->ClientTime, &baseRequest->ClientTime, sizeof(FILETIME));

	baseResponse->Count  				= index == 1 || index == 2 ? LE32(10) : LE32(50);
	baseResponse->VLActivationInterval	= LE32(VLActivationInterval);
	baseResponse->VLRenewalInterval   	= LE32(VLRenewalInterval);

	if (LE32(baseRequest->N_Policy) > LE32(baseResponse->Count)) baseResponse->Count = LE32(LE32(baseRequest->N_Policy) << 1);

	#ifndef NO_LOG
	logResponse(baseResponse, hwId, EpidSource);
	#endif // NO_LOG

	return !0;
}

RequestCallback_t CreateResponseBase = &CreateResponseBaseCallback;

#else // IS_LIBRARY

RequestCallback_t CreateResponseBase = NULL;

#endif // IS_LIBRARY


////TODO: Move to helpers.c
void get16RandomBytes(void* ptr)
{
	int i;
	for (i = 0; i < 4; i++)	((DWORD*)ptr)[i] = rand32();
}

/*
 * Creates v4 response
 */
size_t CreateResponseV4(REQUEST_V4 *const request_v4, BYTE *const responseBuffer, const char* const ipstr)
{
	RESPONSE_V4* Response = (RESPONSE_V4*)responseBuffer;

	if ( !CreateResponseBase(&request_v4->RequestBase, &Response->ResponseBase, NULL, ipstr) ) return 0;

	DWORD pidSize = LE32(Response->ResponseBase.PIDSize);
	BYTE* postEpidPtr =	responseBuffer + V4_PRE_EPID_SIZE + pidSize;
	memmove(postEpidPtr, &Response->ResponseBase.CMID, V4_POST_EPID_SIZE);

	size_t encryptSize = V4_PRE_EPID_SIZE + V4_POST_EPID_SIZE + pidSize;
	AesCmacV4(responseBuffer, encryptSize, responseBuffer + encryptSize);

	return encryptSize + sizeof(Response->MAC);
}

/*
// Workaround for buggy GCC 4.2/4.3
#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 4)
__attribute__((noinline))
#endif
__pure static uint64_t TimestampInterval(void *ts)
{
	return ( GET_UA64LE(ts) / TIME_C1 ) * TIME_C2 + TIME_C3;
}*/


/*
 * Creates the HMAC for v6
 */
static int_fast8_t CreateV6Hmac(BYTE *const encrypt_start, const size_t encryptSize, int_fast8_t tolerance)
{
	BYTE hash[32];
#	define halfHashSize (sizeof(hash) >> 1)
	uint64_t timeSlot;
	BYTE *responseEnd = encrypt_start + encryptSize;

	// This is the time from the response
	FILETIME* ft = (FILETIME*)(responseEnd - V6_POST_EPID_SIZE + sizeof(((RESPONSE*)0)->CMID));

	// Generate a time slot that changes every 4.11 hours.
	// Request and repsonse time must match +/- 1 slot.
	// When generating a response tolerance must be 0.
	// If verifying the hash, try tolerance -1, 0 and +1. One of them must match.

	timeSlot = LE64( (GET_UA64LE(ft) / TIME_C1 * TIME_C2 + TIME_C3) + (tolerance * TIME_C1) );

	// The time slot is hashed with SHA256 so it is not so obvious that it is time
	Sha256((BYTE*) &timeSlot, sizeof(timeSlot), hash);

	// The last 16 bytes of the hashed time slot are the actual HMAC key
	if (!Sha256Hmac
	(
		hash + halfHashSize,								// Use last 16 bytes of SHA256 as HMAC key
		encrypt_start,										// hash only the encrypted part of the v6 response
		encryptSize - sizeof(((RESPONSE_V6*)0)->HMAC),		// encryptSize minus the HMAC itself
		hash												// use same buffer for resulting hash where the key came from
	))
	{
		return FALSE;
	}

	memcpy(responseEnd - sizeof(((RESPONSE_V6*)0)->HMAC), hash + halfHashSize, halfHashSize);
	return TRUE;
#	undef halfHashSize
}


/*
 * Creates v5 or v6 response
 */
size_t CreateResponseV6(REQUEST_V6 *restrict request_v6, BYTE *const responseBuffer, const char* const ipstr)
{
	// The response will be created in a fixed sized struct to
	// avoid unaligned access macros and packed structs on RISC systems
	// which largely increase code size.
	//
	// The fixed sized struct with 64 WCHARs for the ePID will be converted
	// to a variable sized struct later and requires unaligned access macros.

	RESPONSE_V6* Response = (RESPONSE_V6*)responseBuffer;
	RESPONSE* baseResponse = &Response->ResponseBase;

	#ifdef _DEBUG
		RESPONSE_V6_DEBUG* xxx = (RESPONSE_V6_DEBUG*)responseBuffer;
	#endif

	static const BYTE DefaultHwid[8] = { HWID };
	int_fast8_t v6 = LE16(request_v6->MajorVer) > 5;
	AesCtx aesCtx;

	AesInitKey(&aesCtx, v6 ? AesKeyV6 : AesKeyV5, v6, AES_KEY_BYTES);
	AesDecryptCbc(&aesCtx, NULL, request_v6->IV, V6_DECRYPT_SIZE);

	// get random salt and SHA256 it
	get16RandomBytes(Response->RandomXoredIVs);
	Sha256(Response->RandomXoredIVs, sizeof(Response->RandomXoredIVs), Response->Hash);

	if (v6) // V6 specific stuff
	{
		// In v6 a random IV is generated
		Response->Version = request_v6->Version;
		get16RandomBytes(Response->IV);

		// pre-fill with default HwId (not required for v5)
		memcpy(Response->HwId, DefaultHwid, sizeof(Response->HwId));

        // Just copy decrypted request IV (using Null IV) here. Note this is identical
        // to XORing non-decrypted request and reponse IVs
		memcpy(Response->XoredIVs, request_v6->IV, sizeof(Response->XoredIVs));
	}
	else // V5 specific stuff
	{
		// In v5 IVs of request and response must be identical (MS client checks this)
		// The following memcpy copies Version and IVs at once
		memcpy(Response, request_v6, V6_UNENCRYPTED_SIZE);
	}

	// Xor Random bytes with decrypted request IV
	XorBlock(request_v6->IV, Response->RandomXoredIVs);

	// Get the base response
	if ( !CreateResponseBase(&request_v6->RequestBase, baseResponse, Response->HwId, ipstr) ) return 0;

	// Convert the fixed sized struct into variable sized
	DWORD pidSize = LE32(baseResponse->PIDSize);
	BYTE* postEpidPtr =	responseBuffer + V6_PRE_EPID_SIZE + pidSize;
	size_t post_epid_size = v6 ? V6_POST_EPID_SIZE : V5_POST_EPID_SIZE;

	memmove(postEpidPtr, &baseResponse->CMID, post_epid_size);

	// number of bytes to encrypt
	size_t encryptSize =
		V6_PRE_EPID_SIZE
		- sizeof(Response->Version)
		+ pidSize
		+ post_epid_size;

	//AesDecryptBlock(&aesCtx, Response->IV);
	if (v6 && !CreateV6Hmac(Response->IV, encryptSize, 0)) return 0;

	// Padding auto handled by encryption func
	AesEncryptCbc(&aesCtx, NULL, Response->IV, &encryptSize);

	return encryptSize + sizeof(Response->Version);
}


// Create Hashed KMS Client Request Data for KMS Protocol Version 4
BYTE *CreateRequestV4(size_t *size, const REQUEST* requestBase)
{
	*size = sizeof(REQUEST_V4);

	// Build a proper KMS client request data
	BYTE *request = (BYTE *)vlmcsd_malloc(sizeof(REQUEST_V4));

	// Temporary Pointer for access to REQUEST_V4 structure
	REQUEST_V4 *request_v4 = (REQUEST_V4 *)request;

	// Set KMS Client Request Base
	memcpy(&request_v4->RequestBase, requestBase, sizeof(REQUEST));

	// Generate Hash Signature
	AesCmacV4(request, sizeof(REQUEST), request_v4->MAC);

	// Return Request Data
	return request;
}


// Create Encrypted KMS Client Request Data for KMS Protocol Version 6
BYTE* CreateRequestV6(size_t *size, const REQUEST* requestBase)
{
	*size = sizeof(REQUEST_V6);

	// Temporary Pointer for access to REQUEST_V5 structure
	REQUEST_V6 *request = (REQUEST_V6 *)vlmcsd_malloc(sizeof(REQUEST_V6));

	// KMS Protocol Version
	request->Version = requestBase->Version;

	// Initialize the IV
	get16RandomBytes(request->IV);

	// Set KMS Client Request Base
	memcpy(&request->RequestBase, requestBase, sizeof(REQUEST));

	// Encrypt KMS Client Request
	size_t encryptSize = sizeof(request->RequestBase);
	AesCtx Ctx;
	int_fast8_t v6 = LE16(request->MajorVer) > 5;
	AesInitKey(&Ctx, v6 ? AesKeyV6 : AesKeyV5, v6, 16);
	AesEncryptCbc(&Ctx, request->IV, (BYTE*)(&request->RequestBase), &encryptSize);

	// Return Proper Request Data
	return (BYTE*)request;
}


/*
 * Checks whether Length of ePID is valid
 */
static uint8_t checkPidLength(const RESPONSE *const responseBase)
{
	unsigned int i;

	if (LE32(responseBase->PIDSize) > (PID_BUFFER_SIZE << 1)) return FALSE;
	if (responseBase->KmsPID[(LE32(responseBase->PIDSize) >> 1) - 1]) return FALSE;

	for (i = 0; i < (LE32(responseBase->PIDSize) >> 1) - 2; i++)
	{
		if (!responseBase->KmsPID[i]) return FALSE;
	}

	return TRUE;
}


/*
 * "Decrypts" a KMS v4 response. Actually just copies to a fixed size buffer
 */
RESPONSE_RESULT DecryptResponseV4(RESPONSE_V4* response_v4, const int responseSize, BYTE* const rawResponse, const BYTE* const rawRequest)
{
	int copySize =
		V4_PRE_EPID_SIZE +
		(LE32(((RESPONSE_V4*)rawResponse)->ResponseBase.PIDSize) <= PID_BUFFER_SIZE << 1 ?
		LE32(((RESPONSE_V4*)rawResponse)->ResponseBase.PIDSize) :
		PID_BUFFER_SIZE << 1);

	int messageSize = copySize + V4_POST_EPID_SIZE;

	memcpy(response_v4, rawResponse, copySize);
	memcpy(&response_v4->ResponseBase.CMID, rawResponse + copySize, responseSize - copySize);

	// ensure PID is null terminated
	response_v4->ResponseBase.KmsPID[PID_BUFFER_SIZE-1] = 0;

	uint8_t* mac = rawResponse + messageSize;
	AesCmacV4(rawResponse, messageSize, mac);

	REQUEST_V4* request_v4 = (REQUEST_V4*)rawRequest;
	RESPONSE_RESULT result;

	result.mask					 = (DWORD)~0;
	result.PidLengthOK			 = checkPidLength((RESPONSE*)rawResponse);
	result.VersionOK			 = response_v4->ResponseBase.Version == request_v4->RequestBase.Version;
	result.HashOK				 = !memcmp(&response_v4->MAC, mac, sizeof(response_v4->MAC));
	result.TimeStampOK			 = !memcmp(&response_v4->ResponseBase.ClientTime, &request_v4->RequestBase.ClientTime, sizeof(FILETIME));
	result.ClientMachineIDOK	 = !memcmp(&response_v4->ResponseBase.CMID, &request_v4->RequestBase.CMID, sizeof(GUID));
	result.effectiveResponseSize = responseSize;
	result.correctResponseSize	 = sizeof(RESPONSE_V4) - sizeof(response_v4->ResponseBase.KmsPID) + LE32(response_v4->ResponseBase.PIDSize);

	return result;
}


static RESPONSE_RESULT VerifyResponseV6(RESPONSE_RESULT result, const AesCtx* Ctx,	RESPONSE_V6* response_v6, REQUEST_V6* request_v6, BYTE* const rawResponse)
{
	// Check IVs
	result.IVsOK = !memcmp // In V6 the XoredIV is actually the request IV
	(
		response_v6->XoredIVs,
		request_v6->IV,
		sizeof(response_v6->XoredIVs)
	);

	result.IVnotSuspicious = !!memcmp // If IVs are identical, it is obviously an emulator
	(
		request_v6->IV,
		response_v6->IV,
		sizeof(request_v6->IV)
	);

	// Check Hmac
	int_fast8_t tolerance;
	BYTE OldHmac[sizeof(response_v6->HMAC)];

	result.HmacSha256OK = FALSE;

	memcpy	// Save received HMAC to compare with calculated HMAC later
	(
		OldHmac,
		response_v6->HMAC,
		sizeof(response_v6->HMAC)
	);

	//AesEncryptBlock(Ctx, Response_v6->IV); // CreateV6Hmac needs original IV as received over the network

	for (tolerance = -1; tolerance < 2; tolerance++)
	{
		CreateV6Hmac
		(
			rawResponse + sizeof(response_v6->Version),                          // Pointer to start of the encrypted part of the response
			(size_t)result.correctResponseSize - sizeof(response_v6->Version),   // size of the encrypted part
			tolerance                                                            // tolerance -1, 0, or +1
		);

		result.HmacSha256OK = !memcmp // Compare both HMACs
		(
			OldHmac,
			rawResponse + (size_t)result.correctResponseSize - sizeof(response_v6->HMAC),
			sizeof(OldHmac)
		);

		if (result.HmacSha256OK) break;
	}

	return result;
}


static RESPONSE_RESULT VerifyResponseV5(RESPONSE_RESULT result, REQUEST_V5* request_v5, RESPONSE_V5* response_v5)
{
	// Check IVs: in V5 (and only v5) request and response IVs must match
	result.IVsOK = !memcmp(request_v5->IV, response_v5->IV,	sizeof(request_v5->IV));

	// V5 has no Hmac, always set to TRUE
	result.HmacSha256OK = TRUE;

	return result;
}


/*
 * Decrypts a KMS v5 or v6 response received from a server.
 * hwid must supply a valid 16 byte buffer for v6. hwid is ignored in v5
 */
RESPONSE_RESULT DecryptResponseV6(RESPONSE_V6* response_v6, int responseSize, BYTE* const response, const BYTE* const rawRequest, BYTE* hwid)
{
	RESPONSE_RESULT result;
	result.mask = ~0; // Set all bits in the results mask to 1. Assume success first.
	result.effectiveResponseSize = responseSize;

	int copySize1 =
		sizeof(response_v6->Version);

	// Decrypt KMS Server Response (encrypted part starts after RequestIV)
	responseSize -= copySize1;

	AesCtx Ctx;
	int_fast8_t v6 = LE16(((RESPONSE_V6*)response)->MajorVer) > 5;

	AesInitKey(&Ctx, v6 ? AesKeyV6 : AesKeyV5, v6, AES_KEY_BYTES);
	AesDecryptCbc(&Ctx, NULL, response + copySize1, responseSize);

	// Check padding
	BYTE* lastPadByte = response + (size_t)result.effectiveResponseSize - 1;

	// Must be from 1 to 16
	if (!*lastPadByte || *lastPadByte > AES_BLOCK_BYTES)
	{
		result.DecryptSuccess = FALSE;
		return result;
	}

	// Check if pad bytes are all the same
	BYTE* padByte;
	for (padByte = lastPadByte - *lastPadByte + 1; padByte < lastPadByte; padByte++)
	if (*padByte != *lastPadByte)
	{
		result.DecryptSuccess = FALSE;
		return result;
	}

	// Add size of Version, KmsPIDLen and variable size PID
	DWORD pidSize = LE32(((RESPONSE_V6*) response)->ResponseBase.PIDSize);

	copySize1 +=
		V6_UNENCRYPTED_SIZE	 +
		sizeof(response_v6->ResponseBase.PIDSize) +
		(pidSize <= PID_BUFFER_SIZE << 1 ?	pidSize : PID_BUFFER_SIZE << 1);

	// Copy part 1 of response up to variable sized PID
	memcpy(response_v6, response, copySize1);

	// ensure PID is null terminated
	response_v6->ResponseBase.KmsPID[PID_BUFFER_SIZE - 1] = 0;

	// Copy part 2
	size_t copySize2 = v6 ? V6_POST_EPID_SIZE : V5_POST_EPID_SIZE;
	memcpy(&response_v6->ResponseBase.CMID, response + copySize1, copySize2);

	// Decrypting the response is finished here. Now we check the results for validity
	// A basic client doesn't need the stuff below this comment but we want to use vlmcs
	// as a debug tool for KMS emulators.

	REQUEST_V6* request_v6 = (REQUEST_V6*) rawRequest;
	DWORD decryptSize = sizeof(request_v6->IV) + sizeof(request_v6->RequestBase) + sizeof(request_v6->Pad);

	AesDecryptCbc(&Ctx, NULL, request_v6->IV, decryptSize);

	// Check that all version informations are the same
	result.VersionOK =
		request_v6->Version == response_v6->ResponseBase.Version &&
		request_v6->Version == response_v6->Version &&
		request_v6->Version == request_v6->RequestBase.Version;

	// Check Base Request
	result.PidLengthOK			= checkPidLength(&((RESPONSE_V6*) response)->ResponseBase);
	result.TimeStampOK			= !memcmp(&response_v6->ResponseBase.ClientTime, &request_v6->RequestBase.ClientTime, sizeof(FILETIME));
	result.ClientMachineIDOK	= IsEqualGUID(&response_v6->ResponseBase.CMID, &request_v6->RequestBase.CMID);

	// Rebuild Random Key and Sha256 Hash
	BYTE HashVerify[sizeof(response_v6->Hash)];
	BYTE RandomKey[sizeof(response_v6->RandomXoredIVs)];

	memcpy(RandomKey, request_v6->IV, sizeof(RandomKey));
	XorBlock(response_v6->RandomXoredIVs, RandomKey);
	Sha256(RandomKey, sizeof(RandomKey), HashVerify);

	result.HashOK = !memcmp(response_v6->Hash, HashVerify, sizeof(HashVerify));

	// size before encryption (padding not included)
	result.correctResponseSize =
		(v6 ? sizeof(RESPONSE_V6) : sizeof(RESPONSE_V5))
		- sizeof(response_v6->ResponseBase.KmsPID)
		+ LE32(response_v6->ResponseBase.PIDSize);

	// Version specific stuff
	if (v6)
	{
		// Copy the HwId
		memcpy(hwid, response_v6->HwId, sizeof(response_v6->HwId));

		// Verify the V6 specific part of the response
		result = VerifyResponseV6(result, &Ctx, response_v6, request_v6, response);
	}
	else // V5
	{
		// Verify the V5 specific part of the response
		result = VerifyResponseV5(result, request_v6, (RESPONSE_V5*)response_v6);
	}

	// padded size after encryption
	result.correctResponseSize += (~(result.correctResponseSize - sizeof(response_v6->ResponseBase.Version)) & 0xf) + 1;

	return result;
}

