
#include "q_shared.h"
#include "g_local.h" // for gclient_t
#include "windows.h" // for HANDLE
#include <process.h> // for _beginthread
#include "../../libs/cjson/cJSON.h" // for JSON functions
#include "makermod.h" // for hostnameCache_t

hostnameCache_t hostnameCache;

// I kept getting corrupted stack errors due to my lazy ip parsing\
	so I'll just make a function for it
byte *parseIP(const char *str, byte *ip)
{
	unsigned short ip_[4] = { 0 };

	int n = sscanf(str, "%hu.%hu.%hu.%hu", &ip_[0], &ip_[1], &ip_[2], &ip_[3]);

	if (n != 4)
	{
		G_LogPrintf(__FUNCTION__"(): error parsing the IP.\n");
		return NULL;
	}

	ip[0] = ip_[0];
	ip[1] = ip_[1];
	ip[2] = ip_[2];
	ip[3] = ip_[3];
	return ip;
}

hostnameCache_t *hostnameCache_add(byte ip[4], const char *hostname)
{
	hostnameCache_t *root = &hostnameCache;
	while (root->next)
	{
		if (root->ip[0] == ip[0] &&
			root->ip[1] == ip[1] &&
			root->ip[2] == ip[2] &&
			root->ip[3] == ip[3])
			return NULL;
		root = root->next;
	}

	hostnameCache_t *newItem = calloc(1, sizeof(hostnameCache_t));
	//	newItem->prev = root;
	root->next = newItem;
	for (int i = 0; i < 4; i++)
		newItem->ip[i] = ip[i];
	Q_strncpyz(newItem->hostname, hostname, sizeof(newItem->hostname));
}

void ThreadProc(void *ptr)
{
	gclient_t *client = ptr;
	struct in_addr addr = { 0 };

	char ip[24] = { 0 };
	strcpy(ip, client->sess.ip);
	char *c = strchr(ip, ':');
	if (c) ip[c - ip] = '\0';

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

	byte ip_b[4] = { 0 };
	if (parseIP(ip, ip_b))
		hostnameCache_add(ip_b, client->sess.hostname);
	_endthread();
}

void MM_GetHostname(gclient_t *client)
{
	hostnameCache_t *root = hostnameCache.next;

	while (root)
	{
		byte ip[4] = { 0 };

		if (!parseIP(client->sess.ip, ip))
			continue;

		if (root->ip[0] == ip[0] &&
			root->ip[1] == ip[1] &&
			root->ip[2] == ip[2] &&
			root->ip[3] == ip[3])
		{
			Q_strncpyz(client->sess.hostname, root->hostname, sizeof(client->sess.hostname));
			return;
		}
		root = root->next;
	}

	HANDLE h_thread = (HANDLE)_beginthread(ThreadProc, 0, client);
}

void MM_ReadHostnames(void)
{
	fileHandle_t f;
	char fileData[MAX_DATA_SIZE] = { 0 };
	int length = trap_FS_FOpenFile("data\\hostname_cache.json", &f, FS_READ);

	if (!f || length <= 0)
	{ // file not found, welp
		return;
	}

	trap_FS_Read(fileData, MAX_DATA_SIZE, f);

	cJSON * root = cJSON_Parse(fileData);
	int len = root ? cJSON_GetArraySize(root) : 0;

	for (int i = 0; i < len; i++)
	{
		cJSON *item = cJSON_GetArrayItem(root, i);
		if (!item)
		{
			G_LogPrintf(__FUNCTION__"(): item %i is NULL, skipping.\n", i);
			continue;
		}

		cJSON *ip = cJSON_GetObjectItem(item, "ip");
		if (!ip)
		{
			G_LogPrintf(__FUNCTION__"(): item %i has NULL ip, skipping.\n", i);
			continue;
		}

		if (cJSON_GetArraySize(ip) != 4)
		{
			G_LogPrintf(__FUNCTION__"(): item %i has corrupted ip, skipping.\n", i);
			continue;
		}

		byte ip_b[4] = { 0 };
		for (int j = 0; j < 4; j++)
		{
			cJSON *ip_m = cJSON_GetArrayItem(ip, j);
			if(ip_m->type == cJSON_Number)
				ip_b[j] = (byte)cJSON_GetArrayItem(ip, j)->valueint;
			else
			{
				G_LogPrintf(__FUNCTION__"(): item %i has corrupted ip byte, skipping.\n", i);
				continue;
			}
		}

		cJSON *hostname = cJSON_GetObjectItem(item, "hostname");
		if (!hostname)
		{
			G_LogPrintf(__FUNCTION__"(): item %i has NULL hostname, skipping.\n", i);
			continue;
		}
		
		hostnameCache_add(ip_b, hostname->valuestring);
	}

	cJSON_Delete(root);
	trap_FS_FCloseFile(f);
}

void MM_WriteHostnames(void)
{
	fileHandle_t f;
	char fileData[MAX_DATA_SIZE] = { 0 };

	trap_FS_FOpenFile("data\\hostname_cache.json", &f, FS_WRITE);

	cJSON *json_arr = cJSON_CreateArray();
	hostnameCache_t *root = hostnameCache.next;
	while (root)
	{
		cJSON *item = cJSON_CreateObject();
		cJSON *ip = cJSON_CreateArray();

		if (!item || !ip)
		{
			G_LogPrintf(__FUNCTION__"(): failed to create JSON object/array.\n");
			continue;
		}

		for (int i = 0; i < 4; i++)
			cJSON_AddItemToArray(ip, cJSON_CreateNumber(root->ip[i]));

		cJSON_AddItemToObject(item, "ip", ip);
		cJSON_AddStringToObject(item, "hostname", root->hostname);

		cJSON_AddItemToArray(json_arr, item);

		char *s = cJSON_Print(json_arr);

		while (strlen(s) >= sizeof(fileData))
		{
			G_LogPrintf(__FUNCTION__"(): maximum filesize reached, removing items from start.\n");
			cJSON_DeleteItemFromArray(json_arr, 0);
		}

		root = root->next;
	}

	Q_strncpyz(fileData, cJSON_Print(json_arr), sizeof(fileData));
	trap_FS_Write(fileData, strlen(fileData), f);

	cJSON_Delete(json_arr);
	trap_FS_FCloseFile(f);
}