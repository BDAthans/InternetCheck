
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <WinDNS.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <fstream>
#include <iostream>
#include <string>

#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

using namespace std;

BOOL debugging_enabled = TRUE;

BOOL dnsCheck();
BOOL pingCheck();
void logWrite(string);
string getDateTime();

PCWSTR domains[5] = { L"google.com", L"microsoft.com", L"yahoo.com", L"amazon.com", L"cisco.com" };
BOOL domainResult[5];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	string currentStatus = "UP";
	string previousStatus= "UP";

	// Internet status check loop
	while (true) {
		if (dnsCheck() == FALSE && pingCheck() == FALSE) {
			// Internet is down
			if (debugging_enabled == TRUE) { cout << "Internet currently down. Ping and DNS was unsuccessful." << endl << endl; }
			currentStatus = "DOWN";
		}
		else
		{
			// Internet is up
			if (debugging_enabled == TRUE) { cout << "Internet currently up. Ping and/or DNS was successful." << endl << endl; }
			currentStatus = "UP";
		}

		if (currentStatus != previousStatus) {
			logWrite("Internet status change: " + currentStatus);
		}
		
		previousStatus = currentStatus;
		Sleep(5000); // Sleep for 5 second before running pingCheck and dnsCheck again.
	}
	return 0;
}

BOOL dnsCheck() {
	PDNS_RECORD pDnsRecord = NULL;  // Pointer to DNS_RECORD structure.
	DNS_STATUS queryStatus;  // Return value of DnsQuery.

	for (int x = 0; x < 5; x++) {
		// Perform the DNS query for the A (IPv4) record on domains
		queryStatus = DnsQuery(domains[x], DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &pDnsRecord, NULL);

		if (queryStatus == 0) {
			if (debugging_enabled == TRUE) { wcout << L"DNS resolution succeeded for: " << domains[x] << endl; }

			// Traverse the linked list of DNS records returned
			while (pDnsRecord) {
				if (pDnsRecord->wType == DNS_TYPE_A) {
					wchar_t ipStr[INET_ADDRSTRLEN];  // Buffer to hold the IP address string
					IP4_ADDRESS ipaddr = pDnsRecord->Data.A.IpAddress;
					if (debugging_enabled == TRUE) {
						InetNtop(AF_INET, &ipaddr, ipStr, INET_ADDRSTRLEN);
						wcout << L"IP Address: " << ipStr << endl;
					}
				}
				pDnsRecord = pDnsRecord->pNext;
			}
			if (debugging_enabled == TRUE) { wcout << endl; }

			// Free memory allocated for DNS records
			DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
			domainResult[x] = TRUE;
		}
		else {
			if (debugging_enabled == TRUE) { wcout << L"DNS resolution failed with status: " << queryStatus << endl; }
			domainResult[x] = FALSE;
		}
	}

	// DNS comparison to determine if any DNS resolutions completed successfully
	if (domainResult[0] == TRUE || domainResult[1] == TRUE || domainResult[2] == TRUE || domainResult[3] == TRUE || domainResult[4] == TRUE) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

BOOL pingCheck() {
	// Initialize WinSock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::wcerr << L"WSAStartup failed." << std::endl;
		return FALSE;
	}

	// Open the ping handle
	HANDLE hIcmp = IcmpCreateFile();
	if (hIcmp == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Failed to create ICMP handle." << std::endl;
		WSACleanup();
		return FALSE;
	}

	// Setup the echo request
	wchar_t sendData[] = L"Data Buffer";
	DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8; 
	auto* replyBuffer = new char[replySize];
	memset(replyBuffer, 0, replySize);

	// Resolve the domain name to an IP address
	struct addrinfoW hints = {};
	hints.ai_family = AF_INET;  // IPv4 addresses only
	struct addrinfoW* result = nullptr;

	for (int x = 0; x < 5; x++) {

		// Domains IP resolution
		if (GetAddrInfoW(domains[x], NULL, &hints, &result) != 0) {
			std::wcerr << L"DNS resolution failed for: " << domains[x] << std::endl;
			IcmpCloseHandle(hIcmp);
			WSACleanup();
			return FALSE;
		}

		// Convert the first resolved IP to a string
		wchar_t ipStr[INET_ADDRSTRLEN];
		struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)result->ai_addr;
		InetNtop(AF_INET, &sockaddr_ipv4->sin_addr, ipStr, sizeof(ipStr));

		// Convert string IP to binary form using InetPton
		IN_ADDR ipaddr;
		InetPton(AF_INET, ipStr, &ipaddr);

		// Send the ping to the resolved IP address
		DWORD dwRetVal = IcmpSendEcho(hIcmp, ipaddr.S_un.S_addr, sendData, sizeof(sendData), NULL, replyBuffer, replySize, 1000);
		if (dwRetVal != 0) {
			ICMP_ECHO_REPLY* echoReply = (ICMP_ECHO_REPLY*)replyBuffer;

			// Output and handling of ICMP echo reply
			if (echoReply->Status == IP_SUCCESS) {
				wchar_t replyIp[INET_ADDRSTRLEN];
				InetNtop(AF_INET, &echoReply->Address, replyIp, INET_ADDRSTRLEN);
				wcout << L"ICMP Echo Sent to: " << domains[x] << endl;
				wcout << L"Reply from: " << replyIp << L" - RTT: " << echoReply->RoundTripTime << L"ms" << endl << endl;
				domainResult[x] = TRUE;
			}
			else
			{
				wcout << L"Ping to: " << domains[x] << " failed with status : " << echoReply->Status << endl << endl;
				domainResult[x] = FALSE;
			}
		}
		else
		{
			wcerr << L"Call to IcmpSendEcho failed. Error: " << GetLastError() << endl << endl;
			domainResult[x] = FALSE;
		}
	}

	// Clean up
	FreeAddrInfoW(result);
	delete[] replyBuffer;
	IcmpCloseHandle(hIcmp);
	WSACleanup();

	// Ping comparison to determine if any pings completed successfully
	if (domainResult[0] == TRUE || domainResult[1] == TRUE || domainResult[2] == TRUE || domainResult[3] == TRUE || domainResult[4] == TRUE) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

void logWrite(string message) {
	// Create log file and output dateTime and message to the log file
	ofstream logFile("InternetCheck-Log.txt", ios::app);
	string dateTime = getDateTime();

	if (!logFile.is_open()) {
		cerr << "Failed to open 'InternetCheck-Log.txt'." << endl;
	}

	logFile << dateTime << " - " << message << endl;
	logFile.close();
}

string getDateTime() {
	time_t currentTime = time(nullptr);
	tm localTime;
	localtime_s(&localTime, &currentTime);
	char dateTime[100];

	strftime(dateTime, sizeof(dateTime), "%m-%d-%Y %H:%M:%S", &localTime);
	return string(dateTime);
}