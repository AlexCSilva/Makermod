
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

// mm_data.c
void MM_ReadData(const char *fileName, void(*processData)(const char *fileData));
void MM_WriteData(const char *fileName, void(*processData)(char *fileData, int *fileSize));

// mm_hostname.c
typedef struct hostnameCache_s
{
	byte ip[4];
	char hostname[64];

	struct hostnameCache_s *next; // don't think we need a prev
} hostnameCache_t;

extern hostnameCache_t hostnameCache; // for g_session.c

byte *parseIP(const char *str, byte *ip);
void MM_GetHostname(gclient_t *client);
void MM_ReadHostnames(const char *fileData);
void MM_WriteHostnames(char *fileData, int *fileSize);

// mm_jail.c
typedef struct jail_s
{
	vec3_t	origin, angles;

	struct jail_s *next, *prev; // we need a prev here for the removal

	int		count; // this should ONLY be used in the root node\
						(kinda bad method, but fuck having another global var)
} jail_t;

jail_t *MM_GetJail(void);
void MM_ReadJails(const char *fileData);
void MM_WriteJails(char *fileData, int *fileSize);
void MM_JailClient(gentity_t *ent, qboolean respawn);
void Cmd_Jail_f(gentity_t *ent);
void Cmd_NewJail_f(gentity_t *ent);
void Cmd_DelJail_f(gentity_t *ent);
void Cmd_ListJail_f(gentity_t *ent);
