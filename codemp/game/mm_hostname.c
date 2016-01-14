
#include "q_shared.h"
#include "g_local.h" // for gclient_t
#include "windows.h" // for HANDLE
#include <process.h> // for _beginthread

void ThreadProc(void *ptr)
{
	gclient_t *client = ptr;
	struct in_addr addr = { 0 };

	char ip[24] = { 0 };
	strcpy(ip, client->sess.ip);
	for (int i = 0; i < sizeof(ip); i++)
	{
		if (ip[i] == ':')
		{
			ip[i] = 0;
			break;
		}
	}
	addr.s_addr = inet_addr(ip);

	if (addr.s_addr == INADDR_NONE)
	{
		Q_strncpyz(client->sess.hostname, "ILLEGAL_IP", 256); // shouldn't happen, but it was on MSDN so...
		_endthread();
		return;
	}

	struct hostent *remoteHost = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);

	if (remoteHost == NULL)
	{
		DWORD error = WSAGetLastError();
		switch (error)
		{
			case 0:
				break;
			case WSAHOST_NOT_FOUND:
				Q_strncpyz(client->sess.hostname, "NOT_FOUND", 256);
				break;
			case WSANO_DATA:
				Q_strncpyz(client->sess.hostname, "NO_DATA", 256);
				break;
			default:
				Q_strncpyz(client->sess.hostname, va("ERR_%ld", error), 256);
				break;
		}
	}
	else
		Q_strncpyz(client->sess.hostname, remoteHost->h_name, 256);

	_endthread();
}

void MM_GetHostname(gclient_t *client)
{
	HANDLE h_thread = (HANDLE)_beginthread(ThreadProc, 0, client);
}