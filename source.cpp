
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <WinDNS.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <fstream>
#include <iostream>
#include <string>
#include <ShlObj.h>

#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

using namespace std;

BOOL debugging_enabled = FALSE;

BOOL dnsCheck();
BOOL pingCheck();
void logWrite(string);
string getDocumentsFolder();
void createLogDirectory(LPCWSTR);
string getDateTime();
string getDate();
wstring convertStringToWString(string);
string convertPCWSTRToString(PCWSTR);
string convertWcharBufferToString(wchar_t*);

PCWSTR domains[5] = { L"google.com", L"microsoft.com", L"yahoo.com", L"amazon.com", L"cisco.com" };
BOOL domainResult[5];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	string currentStatus = "UP";
	string previousStatus = "UP";

	// DEBUG output
	if (debugging_enabled == TRUE) { logWrite("DEBUG: WinMain function running"); }

	// Internet status check loop
	while (true) {
		if (dnsCheck() == FALSE && pingCheck() == FALSE) {
			// Internet is down
			if (debugging_enabled == TRUE) { logWrite("DEBUG: Internet currently down. Ping and DNS was unsuccessful."); }
			currentStatus = "DOWN";
		}
		else
		{
			// Internet is up
			if (debugging_enabled == TRUE) { logWrite("DEBUG: Internet currently up. Ping and/or DNS was successful."); }
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

	// DEBUG output
	if (debugging_enabled == TRUE) { logWrite("DEBUG: dnsCheck function running"); }

	for (int x = 0; x < 5; x++) {
		// Perform the DNS query for the A (IPv4) record on domains
		queryStatus = DnsQuery(domains[x], DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &pDnsRecord, NULL);

		if (queryStatus == 0) {
			if (debugging_enabled == TRUE) { 
				string currentDomain = convertPCWSTRToString(domains[x]);
				logWrite("DEBUG: DNS resolution succeeded for: " + currentDomain); 
			}

			// Traverse the linked list of DNS records returned
			while (pDnsRecord) {
				if (pDnsRecord->wType == DNS_TYPE_A) {
					wchar_t ipStr[INET_ADDRSTRLEN];  // Buffer to hold the IP address string
					IP4_ADDRESS ipaddr = pDnsRecord->Data.A.IpAddress;
					if (debugging_enabled == TRUE) {
						InetNtop(AF_INET, &ipaddr, ipStr, INET_ADDRSTRLEN);

						string currentIP = convertWcharBufferToString(ipStr);
						logWrite("DEBUG: IP Address: " + currentIP);
					}
				}
				pDnsRecord = pDnsRecord->pNext;
			}
			// Free memory allocated for DNS records
			DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
			domainResult[x] = TRUE;
		}
		else {
			if (debugging_enabled == TRUE) { 
				logWrite("DEBUG: DNS resolution failed with status: " + queryStatus); }
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
	string currentDomain;
	string currentReplyIP;

	// DEBUG output
	if (debugging_enabled == TRUE) { logWrite("DEBUG: dnsCheck function running"); }

	// Initialize WinSock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		if (debugging_enabled == TRUE) { logWrite("DEBUG: pingCheck: WSAStartup failed."); }
		return FALSE;
	}

	// Open the ping handle
	HANDLE hIcmp = IcmpCreateFile();
	if (hIcmp == INVALID_HANDLE_VALUE) {
		if (debugging_enabled == TRUE) { logWrite("DEBUG: pingCheck: Failed to create ICMP handle."); }
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
			if (debugging_enabled == TRUE) {
				currentDomain = convertPCWSTRToString(domains[x]);
				logWrite("DEBUG: DNS resolution failed for: " + currentDomain);
			}
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

				if (debugging_enabled == TRUE) {
					logWrite("DEBUG: ICMP Echo Sent to: " + currentDomain);
					currentReplyIP = convertWcharBufferToString(replyIp);
					logWrite("DEBUG: Reply from: " + currentReplyIP + " - RTT: " + to_string(echoReply->RoundTripTime) + "ms");
				}
				domainResult[x] = TRUE;
			}
			else
			{	
				if (debugging_enabled == TRUE) {
					logWrite("DEBUG: Ping to: " + currentDomain + " failed with status : " + to_string(echoReply->Status));
				}
				domainResult[x] = FALSE;
			}
		}
		else
		{	
			if (debugging_enabled == TRUE) {
				logWrite("DEBUG: Call to IcmpSendEcho failed. Error: " + GetLastError());
			}
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
	string logFileName;
	string currentDate = getDate();
	string dateTime = getDateTime();

	if (debugging_enabled == TRUE) {
		logFileName = "InternetCheck-Report-" + currentDate + "-DEBUG.log";
	}
	else
	{
		logFileName = "InternetCheck-Report-" + currentDate + ".log";
	}	

	string logDirPath = getDocumentsFolder() + "\\InternetCheck";
	LPCWSTR logDirPathW = convertStringToWString(logDirPath).c_str();
	createLogDirectory(logDirPathW);

	string logFullPath = logDirPath + "\\" + logFileName;
	ofstream logFile(logFullPath, ios::app); // Create log file and output dateTime and message to the log file

	if (!logFile.is_open()) {
		// Error handling for not being able to create log file
	}
	else
	{
		logFile << dateTime << " - " << message << endl;
		logFile.close();
	}
}

string getDocumentsFolder() {
	PWSTR path = nullptr;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);

	if (SUCCEEDED(result) && path != nullptr) {
		std::wstring ws(path);
		CoTaskMemFree(static_cast<LPVOID>(path)); // Free the allocated path

		std::string documentsPath(ws.begin(), ws.end());
		return documentsPath;
	}
	else {
		// Handle error
		std::cerr << "Failed to get Documents folder path." << std::endl;
		return "";
	}
}

void createLogDirectory(LPCWSTR directoryPath) {
	if (CreateDirectory(directoryPath, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
		// Directory created successfully or already exists
	}
	else
	{
		cerr << "Failed to create directory at " << directoryPath << endl;
	}
}

string getDateTime() {
	time_t currentTime = time(nullptr);
	tm localTime;
	localtime_s(&localTime, &currentTime);
	char dateTime[100];

	strftime(dateTime, sizeof(dateTime), "%m-%d-%Y %H:%M:%S", &localTime);
	return string(dateTime);
}

string getDate() {
	time_t currentTime = time(nullptr);
	tm localTime;
	localtime_s(&localTime, &currentTime);
	char date[100];

	strftime(date, sizeof(date), "%m-%d-%Y", &localTime);
	return string(date);
}

wstring convertStringToWString (string string) {
	return wstring(string.begin(), string.end());
}

string convertPCWSTRToString(PCWSTR pcwstr) {
	if (pcwstr == nullptr) return string();

	// Get the length of the required buffer
	int len = WideCharToMultiByte(CP_UTF8, 0, pcwstr, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 0) return string();

	// Allocate buffer and convert
	string result(len - 1, '\0'); // Length includes the null terminator
	WideCharToMultiByte(CP_UTF8, 0, pcwstr, -1, &result[0], len, nullptr, nullptr);
	return result;
}

string convertWcharBufferToString(wchar_t* wcharBuffer) {
	if(wcharBuffer == nullptr) return std::string();  // Handle null pointer

	// Determine the length of the buffer we need
	int len = WideCharToMultiByte(CP_UTF8, 0, wcharBuffer, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 0) return std::string();  // Handle error

	std::string str(len - 1, '\0');  // Allocate string of sufficient length
	WideCharToMultiByte(CP_UTF8, 0, wcharBuffer, -1, &str[0], len, nullptr, nullptr);
	return str;
}