#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG
#include "helpers.h"
#include "wintap.h"

#ifndef NO_TAP

#include "types.h"
#include "endian.h"
#include "output.h"
#include "tap-windows.h"
#include <iphlpapi.h>

#if !_WIN32
#include <arpa/inet.h>
#endif // !_WIN32

static char* szIpAddress = "10.10.10.9";
static char* szMask = "30";
static char* szTapName;
static char *ActiveTapName, *AdapterClass;
static char* szLeaseDuration = "1d";
static uint32_t IpAddress, Mask, Network, Broadcast, DhcpServer; // These are host-endian (=little-endian) for easier calculations
static uint32_t Mtu;
static uint_fast8_t Cidr;
static HANDLE TapHandle;
static TapDriverVersion_t DriverVersion;
static IpPacket_t* IpPacket;
static uint32_t DhcpLeaseDuration;


static BOOL isAddressAssigned()
{
	PMIB_IPADDRTABLE pIPAddrTable;
	DWORD dwSize = 0;
	BOOL result = FALSE;

	pIPAddrTable = (PMIB_IPADDRTABLE)vlmcsd_malloc(sizeof(MIB_IPADDRTABLE));
	const DWORD status = GetIpAddrTable(pIPAddrTable, &dwSize, 0);
	free(pIPAddrTable);

	if (status != ERROR_INSUFFICIENT_BUFFER) return FALSE;
	pIPAddrTable = (MIB_IPADDRTABLE *)vlmcsd_malloc(dwSize);

	if (GetIpAddrTable(pIPAddrTable, &dwSize, 0))
	{
		free(pIPAddrTable);
		return FALSE;
	}

	PMIB_IPADDRROW row;
	for (row = pIPAddrTable->table; row < pIPAddrTable->table + pIPAddrTable->dwNumEntries; row++)
	{
		if (
			row->dwAddr == BE32(IpAddress) &&
			!(row->wType & (MIB_IPADDR_DELETED | MIB_IPADDR_DISCONNECTED | MIB_IPADDR_TRANSIENT))
			)
		{
			result = TRUE;
			break;
		}
	}

	free(pIPAddrTable);
	return result;
}


static void parseTapArgument(char* argument)
{
	char* equalSignPosition = strchr(argument, (int)'=');
	char* slashPosition = strchr(argument, (int)'/');
	char* colonPosition = strchr(argument, (int)':');

	szTapName = argument;

	if (equalSignPosition)
	{
		*equalSignPosition = 0;
		szIpAddress = equalSignPosition + 1;
	}

	if (slashPosition)
	{
		*slashPosition = 0;
		szMask = slashPosition + 1;
	}

	if (colonPosition)
	{
		*colonPosition = 0;
		szLeaseDuration = colonPosition + 1;
	}

	IpAddress = BE32(inet_addr(szIpAddress));

	if (IpAddress == BE32(INADDR_NONE))
	{
		printerrorf("Fatal: %s is not a valid IPv4 address\n", szIpAddress);
		exit(VLMCSD_EINVAL);
	}

	char* next;
	Cidr = (uint8_t)strtol(szMask, &next, 10);

	if (*next || Cidr < 8 || Cidr > 30)
	{
		printerrorf("Fatal: /%s is not a valid CIDR mask between /8 and /30\n", szMask);
		exit(VLMCSD_EINVAL);
	}

	if (!((DhcpLeaseDuration = timeSpanString2Seconds(szLeaseDuration))))
	{
		printerrorf("Fatal: No valid time span specified in option -%c.\n", 'O');
		exit(VLMCSD_EINVAL);
	}

	Mask = (uint32_t)~(0xffffffff >> Cidr);
	Network = IpAddress & Mask;
	Broadcast = IpAddress | ~Mask;
	DhcpServer = IpAddress + 1;

	if (IpAddress <= Network || IpAddress + 1 >= Broadcast)
	{
		uint32_t lowerIpBE = BE32(Network + 1);
		uint32_t upperIpBE = BE32(Broadcast - 2);
		const char* szLower = vlmcsd_strdup(inet_ntoa(*(struct in_addr*)&lowerIpBE));
		const char* szUpper = vlmcsd_strdup(inet_ntoa(*(struct in_addr*)&upperIpBE));

		printerrorf("Fatal: For this subnet the IPv4 address must be ");

		if (lowerIpBE == upperIpBE)
		{
			printerrorf("%s\n", szLower);
		}
		else
		{
			printerrorf("between %s and %s\n", szLower, szUpper);
		}

		exit(VLMCSD_EINVAL);
	}
}


__noreturn static void WinErrorExit(DWORD error)
{
	printerrorf("Registry read error: %s\n", win_strerror((int)error));
	exit(error);
}


static HANDLE OpenTapHandle()
{
	HANDLE handle = INVALID_HANDLE_VALUE;
	HKEY regAdapterKey;
	DWORD regResult;
	if ((regResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ADAPTER_KEY, 0, KEY_READ | KEY_WOW64_64KEY, &regAdapterKey)) != ERROR_SUCCESS)
	{
		WinErrorExit(regResult);
	}

	char subKeyName[TAP_REGISTRY_DATA_SIZE];
	DWORD i, subKeySize = sizeof(subKeyName);

	for (i = 0; (regResult = RegEnumKeyEx(regAdapterKey, i, subKeyName, &subKeySize, NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS; i++)
	{
		HKEY regSubKey;
		DWORD type, regDataSize;
		char regData[TAP_REGISTRY_DATA_SIZE];

		if (regResult) WinErrorExit(regResult);

		if ((regResult = RegOpenKeyEx(regAdapterKey, subKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &regSubKey)) == ERROR_SUCCESS)
		{
			regDataSize = sizeof(regData);

			if (RegQueryValueEx(regSubKey, "ComponentId", NULL, &type, (LPBYTE)regData, &regDataSize) == ERROR_SUCCESS)
			{
				if (
					type == REG_SZ &&
					(
						!strncmp(regData, "tap0801", sizeof(regData)) ||
						!strncmp(regData, "tap0901", sizeof(regData)) ||
						!strncmp(regData, "TEAMVIEWERVPN", sizeof(regData))
						)
					)
				{
					AdapterClass = vlmcsd_strdup(regData);
					regDataSize = sizeof(regData);

					if (RegQueryValueEx(regSubKey, "NetCfgInstanceId", NULL, &type, (LPBYTE)regData, &regDataSize) == ERROR_SUCCESS && type == REG_SZ)
					{
						HKEY connectionKey;
						char connectionKeyName[TAP_REGISTRY_DATA_SIZE];

						strncpy(connectionKeyName, NETWORK_CONNECTIONS_KEY "\\", sizeof(connectionKeyName));
						strncat(connectionKeyName, regData, sizeof(connectionKeyName) - strlen(connectionKeyName) - 1);
						strncat(connectionKeyName, "\\Connection", sizeof(connectionKeyName) - strlen(connectionKeyName) - 1);

						if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, connectionKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &connectionKey) == ERROR_SUCCESS)
						{
							char deviceName[TAP_REGISTRY_DATA_SIZE];
							regDataSize = sizeof(deviceName);

							if (RegQueryValueEx(connectionKey, "Name", NULL, &type, (LPBYTE)deviceName, &regDataSize) == ERROR_SUCCESS && type == REG_SZ)
							{
								if (!strcmp(szTapName, ".") || !strncasecmp(szTapName, deviceName, sizeof(deviceName)))
								{
									ActiveTapName = vlmcsd_strdup(deviceName);
									strncpy(deviceName, USERMODEDEVICEDIR, sizeof(deviceName));
									strncat(deviceName, regData, sizeof(deviceName) - strlen(deviceName) - 1);
									strncat(deviceName, strcmp(AdapterClass, "TEAMVIEWERVPN") ? TAP_WIN_SUFFIX : ".dgt", sizeof(deviceName) - strlen(deviceName) - 1);
									handle = CreateFile(deviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);
								}
							}
						}

						RegCloseKey(connectionKey);
					}

					if (handle == INVALID_HANDLE_VALUE) free(AdapterClass);
				}
			}
		}

		RegCloseKey(regSubKey);
		subKeySize = sizeof(subKeyName);
		if (handle != INVALID_HANDLE_VALUE) break;
	}

	RegCloseKey(regAdapterKey);

	if (handle == INVALID_HANDLE_VALUE)
	{
		printerrorf("Fatal: No compatible VPN adapter");

		if (!strcmp(szTapName, "."))
		{
			printerrorf("s");
		}
		else
		{
			printerrorf(" with name \"%s\"", szTapName);
		}

		printerrorf(" available for use\n");
		exit(ERROR_DEVICE_NOT_AVAILABLE);
	}

	return handle;
}


static int DevCtl(DWORD code, void* data, DWORD len)
{
	if (!DeviceIoControl(TapHandle, code, data, len, data, len, &len, NULL))
	{
		const DWORD error = GetLastError();
		printerrorf("Fatal: VPN adapter error: %s\n", win_strerror(error));
		exit(error);
	}

	return len;
}


static DWORD WINAPI TapMirror(LPVOID data_unused)
{
	while (TRUE)
	{
		DWORD bytesRead, bytesWritten;
		if (!ReadFile(TapHandle, IpPacket, Mtu, &bytesRead, NULL)) break;

		const uint32_t temp = IpPacket->ip_src;
		IpPacket->ip_src = IpPacket->ip_dst;
		IpPacket->ip_dst = temp;

		if (!WriteFile(TapHandle, IpPacket, bytesRead, &bytesWritten, NULL)) break;

#		if !defined(NO_LOG) && defined(_PEDANTIC)
		if (bytesRead != bytesWritten) logger("Warning: VPN device \"%s\": %u bytes could not be written\n", ActiveTapName, bytesRead - bytesWritten);
#		endif // !defined(NO_LOG) && defined(_PEDANTIC)
	}

	const DWORD error = GetLastError();

#	ifndef NO_LOG
	logger("Warning: VPN thread for device \"%s\" exiting: %s\n", ActiveTapName, win_strerror(error));
#	endif // NO_LOG

	free(ActiveTapName);
	CloseHandle(TapHandle);
	exitOnWarningLevel(1);
	return error;
}


void startTap(char* const argument)
{
	if (!strcmp(argument, "-")) return;
	parseTapArgument(argument);

	TapHandle = OpenTapHandle();

	// Get MTU and driver version
	DevCtl(TAP_WIN_IOCTL_GET_MTU, &Mtu, sizeof(Mtu));
	DevCtl(TAP_WIN_IOCTL_GET_VERSION, &DriverVersion, sizeof(DriverVersion));

	// Configure TUN mode
	TapConfigTun_t tapTunCfg;
	tapTunCfg.Address.s_addr = BE32(IpAddress);
	tapTunCfg.Network.s_addr = BE32(Network);
	tapTunCfg.Mask.s_addr = BE32(Mask);
	DevCtl(TAP_WIN_IOCTL_CONFIG_TUN, &tapTunCfg, sizeof(tapTunCfg));

	// Setup the drivers internal DHCP server
	TapConfigDhcp_t tapDhcpCfg;
	tapDhcpCfg.Address.s_addr = BE32(IpAddress);
	tapDhcpCfg.Mask.s_addr = BE32(Mask);
	tapDhcpCfg.DhcpServer.s_addr = BE32(IpAddress + 1);
	tapDhcpCfg.LeaseDuration = DhcpLeaseDuration;
	DevCtl(TAP_WIN_IOCTL_CONFIG_DHCP_MASQ, &tapDhcpCfg, sizeof(tapDhcpCfg));

	// Connect the virtual network cable
	BOOL isCableConnected = TRUE;
	DevCtl(TAP_WIN_IOCTL_SET_MEDIA_STATUS, &isCableConnected, sizeof(isCableConnected));

	// Allocate buffer and start mirror thread
	IpPacket = (IpPacket_t*)vlmcsd_malloc(Mtu);
	HANDLE threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TapMirror, NULL, 0, NULL);

	if (!threadHandle)
	{
		DWORD error = GetLastError();
		printerrorf("Fatal: Unable to start VPN thread: %s\n", win_strerror(error));
		exit(error);
	}

	CloseHandle(threadHandle);

#	ifndef NO_LOG
	logger("%s %u.%u.%u device \"%s\" started\n", AdapterClass, DriverVersion.Major, DriverVersion.Minor, DriverVersion.Build, ActiveTapName);
#	endif // NO_LOG

	DWORD i;
	BOOL isAssigned;

	// Wait up to 4 seconds until the IP address is up and running
	// so vlmcsd can actually bind to and listen on it
	for (i = 0; !((isAssigned = isAddressAssigned())) && i < 20; i++) Sleep(200);

	if (!isAssigned)
	{
		printerrorf("Warning: IPv4 address %s not assigned\n", szIpAddress);
	}
	else
	{
#		ifndef NO_LOG
		logger("IPv4 address %s assigned\n", szIpAddress);
#		endif // NO_LOG
	}
}

#endif // NO_TAP

