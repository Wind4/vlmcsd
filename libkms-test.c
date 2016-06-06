#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkms.h"
#include "kms.h"
#include "endian.h"

static const char ePID[] = { 'T', 0, 'E', 0, 'S', 0, 'T', 0, 0, 0 };

__stdcall BOOL KmsCallBack(const REQUEST *const baseRequest, RESPONSE *const baseResponse, BYTE *const hwId, const char* const ipstr)
{
	printf("libvlmcs-test.c: Entered KmsCallBack for client %s\n", ipstr);

	memcpy(&baseResponse->CMID, &baseRequest->CMID, sizeof(GUID));
	memcpy(&baseResponse->ClientTime, &baseRequest->ClientTime, sizeof(FILETIME));
	memcpy(&baseResponse->KmsPID, ePID, sizeof(ePID));

	baseResponse->Version = baseRequest->Version;
	baseResponse->Count = LE32(LE32(baseRequest->N_Policy) << 1);
	baseResponse->PIDSize = sizeof(ePID);
	baseResponse->VLActivationInterval = LE32(120);
	baseResponse->VLRenewalInterval = LE32(10080);

	if (hwId && baseResponse->MajorVer > 5) memcpy(hwId, "\x01\x02\x03\x04\x05\x06\x07\x08", 8);

	return TRUE;
}

int main(int argc, char** argv)
{
	int version = GetLibKmsVersion();

	if (version < 0x30001)
	{
		fprintf(stderr, "KMS library version %u.%u or greater required\n", (unsigned int)(version >> 16), (unsigned int)(version & 0xffff));
	}

	printf("%s: Program start\n", GetEmulatorVersion());
	StartKmsServer(1688, KmsCallBack);
	return 0;
}
