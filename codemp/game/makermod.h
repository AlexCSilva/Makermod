
void BG_GetVehicleModelName(char *modelname);
void BG_GetVehicleModelNameDontADD(char *modelname);
char *NPC_CheckNPCModel ( char *NPCName, gentity_t *NPC, char *classname );


//Bobafetts fix:
void JKG_PatchEngine();
void JKG_UnpatchEngine();

// g_client.c
gentity_t *SelectRandomFurthestSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, team_t team );


int numClientsFromIP[MAX_CLIENTS];

struct connectedClients_t
{
	char	PortlessIp[24];
	int		numClientsFromIP;
} cnctClients[MAX_CLIENTS];

#define MAX_NPC_MODELS 14

 // SpioR - declarations
void MM_GetHostname(gclient_t *client);
void MM_ReadHostnames(void);
void MM_WriteHostnames(void);

typedef struct hostnameCache_s
{
	byte ip[4];
	char hostname[64];

	struct hostnameCache_t *next; // don't think we need a prev
} hostnameCache_t;

