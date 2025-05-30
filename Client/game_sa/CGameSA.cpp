/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CGameSA.cpp
 *  PURPOSE:     Base game logic handling
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#define ALLOC_STATS_MODULE_NAME "game_sa"
#include "SharedUtil.hpp"
#include "SharedUtil.MemAccess.hpp"
#include "D3DResourceSystemSA.h"
#include "CFileLoaderSA.h"
#include "CColStoreSA.h"

unsigned int&  CGameSA::ClumpOffset = *(unsigned int*)0xB5F878;
unsigned long* CGameSA::VAR_SystemTime;
unsigned long* CGameSA::VAR_IsAtMenu;
unsigned long* CGameSA::VAR_IsGameLoaded;
bool*          CGameSA::VAR_GamePaused;
bool*          CGameSA::VAR_IsForegroundWindow;
unsigned long* CGameSA::VAR_SystemState;
void*          CGameSA::VAR_StartGame;
bool*          CGameSA::VAR_IsNastyGame;
float*         CGameSA::VAR_TimeScale;
float*         CGameSA::VAR_FPS;
float*         CGameSA::VAR_OldTimeStep;
float*         CGameSA::VAR_TimeStep;
unsigned long* CGameSA::VAR_Framelimiter;

unsigned int OBJECTDYNAMICINFO_MAX = *(uint32_t*)0x59FB4C != 0x90909090 ? *(uint32_t*)0x59FB4C : 160;            // default: 160

/**
 * \todo allow the addon to change the size of the pools (see 0x4C0270 - CPools::Initialise) (in start game?)
 */
CGameSA::CGameSA()
{
    pGame = this;
    m_bAsyncScriptEnabled = false;
    m_bAsyncScriptForced = false;
    m_bASyncLoadingSuspended = false;
    m_iCheckStatus = 0;

    const unsigned int modelInfoMax = GetCountOfAllFileIDs();
    ModelInfo = new CModelInfoSA[modelInfoMax];
    ObjectGroupsInfo = new CObjectGroupPhysicalPropertiesSA[OBJECTDYNAMICINFO_MAX];

    SetInitialVirtualProtect();

    // Initialize the offsets
    eGameVersion version = FindGameVersion();
    switch (version)
    {
        case VERSION_EU_10:
            COffsets::Initialize10EU();
            break;
        case VERSION_US_10:
            COffsets::Initialize10US();
            break;
        case VERSION_11:
            COffsets::Initialize11();
            break;
        case VERSION_20:
            COffsets::Initialize20();
            break;
    }

    // Set the model ids for all the CModelInfoSA instances
    for (int i = 0; i < modelInfoMax; i++)
    {
        ModelInfo[i].SetModelID(i);
    }

    // Prepare all object dynamic infos for CObjectGroupPhysicalPropertiesSA instances
    for (int i = 0; i < OBJECTDYNAMICINFO_MAX; i++)
    {
        ObjectGroupsInfo[i].SetGroup(i);
    }

    DEBUG_TRACE("CGameSA::CGameSA()");
    this->m_pAudioEngine = new CAudioEngineSA((CAudioEngineSAInterface*)CLASS_CAudioEngine);
    this->m_pAEAudioHardware = new CAEAudioHardwareSA((CAEAudioHardwareSAInterface*)CLASS_CAEAudioHardware);
    this->m_pAESoundManager = new CAESoundManagerSA((CAESoundManagerSAInterface*)CLASS_CAESoundManager);
    this->m_pAudioContainer = new CAudioContainerSA();
    this->m_pWorld = new CWorldSA();
    this->m_pPools = new CPoolsSA();
    this->m_pClock = new CClockSA();
    this->m_pRadar = new CRadarSA();
    this->m_pCamera = new CCameraSA((CCameraSAInterface*)CLASS_CCamera);
    this->m_pCoronas = new CCoronasSA();
    this->m_pCheckpoints = new CCheckpointsSA();
    this->m_pPickups = new CPickupsSA();
    this->m_pExplosionManager = new CExplosionManagerSA();
    this->m_pHud = new CHudSA();
    this->m_pFireManager = new CFireManagerSA();
    this->m_p3DMarkers = new C3DMarkersSA();
    this->m_pPad = new CPadSA((CPadSAInterface*)CLASS_CPad);
    this->m_pTheCarGenerators = new CTheCarGeneratorsSA();
    this->m_pCAERadioTrackManager = new CAERadioTrackManagerSA();
    this->m_pWeather = new CWeatherSA();
    this->m_pMenuManager = new CMenuManagerSA();
    this->m_pStats = new CStatsSA();
    this->m_pFont = new CFontSA();
    this->m_pPathFind = new CPathFindSA();
    this->m_pPopulation = new CPopulationSA();
    this->m_pTaskManagementSystem = new CTaskManagementSystemSA();
    this->m_pSettings = new CSettingsSA();
    this->m_pCarEnterExit = new CCarEnterExitSA();
    this->m_pControllerConfigManager = new CControllerConfigManagerSA();
    this->m_pProjectileInfo = new CProjectileInfoSA();
    this->m_pRenderWare = new CRenderWareSA(version);
    this->m_pHandlingManager = new CHandlingManagerSA();
    this->m_pEventList = new CEventListSA();
    this->m_pGarages = new CGaragesSA((CGaragesSAInterface*)CLASS_CGarages);
    this->m_pTasks = new CTasksSA((CTaskManagementSystemSA*)m_pTaskManagementSystem);
    this->m_pAnimManager = new CAnimManagerSA;
    this->m_pStreaming = new CStreamingSA;
    this->m_pVisibilityPlugins = new CVisibilityPluginsSA;
    this->m_pKeyGen = new CKeyGenSA;
    this->m_pRopes = new CRopesSA;
    this->m_pFx = new CFxSA((CFxSAInterface*)CLASS_CFx);
    this->m_pFxManager = new CFxManagerSA((CFxManagerSAInterface*)CLASS_CFxManager);
    this->m_pWaterManager = new CWaterManagerSA();
    this->m_pWeaponStatsManager = new CWeaponStatManagerSA();
    this->m_pPointLights = new CPointLightsSA();
    this->m_collisionStore = new CColStoreSA();

    // Normal weapon types (WEAPONSKILL_STD)
    for (int i = 0; i < NUM_WeaponInfosStdSkill; i++)
    {
        eWeaponType weaponType = (eWeaponType)(WEAPONTYPE_PISTOL + i);
        WeaponInfos[i] = new CWeaponInfoSA((CWeaponInfoSAInterface*)(ARRAY_WeaponInfo + i * CLASSSIZE_WeaponInfo), weaponType);
        m_pWeaponStatsManager->CreateWeaponStat(WeaponInfos[i], (eWeaponType)(weaponType - WEAPONTYPE_PISTOL), WEAPONSKILL_STD);
    }

    // Extra weapon types for skills (WEAPONSKILL_POOR,WEAPONSKILL_PRO,WEAPONSKILL_SPECIAL)
    int          index;
    eWeaponSkill weaponSkill = eWeaponSkill::WEAPONSKILL_POOR;
    for (int skill = 0; skill < 3; skill++)
    {
        // STD is created first, then it creates "extra weapon types" (poor, pro, special?) but in the enum 1 = STD which meant the STD weapon skill contained
        // pro info
        if (skill >= 1)
        {
            if (skill == 1)
            {
                weaponSkill = eWeaponSkill::WEAPONSKILL_PRO;
            }
            if (skill == 2)
            {
                weaponSkill = eWeaponSkill::WEAPONSKILL_SPECIAL;
            }
        }
        for (int i = 0; i < NUM_WeaponInfosOtherSkill; i++)
        {
            eWeaponType weaponType = (eWeaponType)(WEAPONTYPE_PISTOL + i);
            index = NUM_WeaponInfosStdSkill + skill * NUM_WeaponInfosOtherSkill + i;
            WeaponInfos[index] = new CWeaponInfoSA((CWeaponInfoSAInterface*)(ARRAY_WeaponInfo + index * CLASSSIZE_WeaponInfo), weaponType);
            m_pWeaponStatsManager->CreateWeaponStat(WeaponInfos[index], weaponType, weaponSkill);
        }
    }

    m_pPlayerInfo = new CPlayerInfoSA((CPlayerInfoSAInterface*)CLASS_CPlayerInfo);

    // Init cheat name => address map
    m_Cheats[CHEAT_HOVERINGCARS] = new SCheatSA((BYTE*)VAR_HoveringCarsEnabled);
    m_Cheats[CHEAT_FLYINGCARS] = new SCheatSA((BYTE*)VAR_FlyingCarsEnabled);
    m_Cheats[CHEAT_EXTRABUNNYHOP] = new SCheatSA((BYTE*)VAR_ExtraBunnyhopEnabled);
    m_Cheats[CHEAT_EXTRAJUMP] = new SCheatSA((BYTE*)VAR_ExtraJumpEnabled);

    // New cheats for Anticheat
    m_Cheats[CHEAT_TANKMODE] = new SCheatSA((BYTE*)VAR_TankModeEnabled, false);
    m_Cheats[CHEAT_NORELOAD] = new SCheatSA((BYTE*)VAR_NoReloadEnabled, false);
    m_Cheats[CHEAT_PERFECTHANDLING] = new SCheatSA((BYTE*)VAR_PerfectHandling, false);
    m_Cheats[CHEAT_ALLCARSHAVENITRO] = new SCheatSA((BYTE*)VAR_AllCarsHaveNitro, false);
    m_Cheats[CHEAT_BOATSCANFLY] = new SCheatSA((BYTE*)VAR_BoatsCanFly, false);
    m_Cheats[CHEAT_INFINITEOXYGEN] = new SCheatSA((BYTE*)VAR_InfiniteOxygen, false);
    m_Cheats[CHEAT_WALKUNDERWATER] = new SCheatSA((BYTE*)VAR_WalkUnderwater, false);
    m_Cheats[CHEAT_FASTERCLOCK] = new SCheatSA((BYTE*)VAR_FasterClock, false);
    m_Cheats[CHEAT_FASTERGAMEPLAY] = new SCheatSA((BYTE*)VAR_FasterGameplay, false);
    m_Cheats[CHEAT_SLOWERGAMEPLAY] = new SCheatSA((BYTE*)VAR_SlowerGameplay, false);
    m_Cheats[CHEAT_ALWAYSMIDNIGHT] = new SCheatSA((BYTE*)VAR_AlwaysMidnight, false);
    m_Cheats[CHEAT_FULLWEAPONAIMING] = new SCheatSA((BYTE*)VAR_FullWeaponAiming, false);
    m_Cheats[CHEAT_INFINITEHEALTH] = new SCheatSA((BYTE*)VAR_InfiniteHealth, false);
    m_Cheats[CHEAT_NEVERWANTED] = new SCheatSA((BYTE*)VAR_NeverWanted, false);
    m_Cheats[CHEAT_HEALTARMORMONEY] = new SCheatSA((BYTE*)VAR_HealthArmorMoney, false);

    // Change pool sizes here
    m_pPools->SetPoolCapacity(BUILDING_POOL, 13000);                                          // Default is 10000
    m_pPools->SetPoolCapacity(TASK_POOL, 5000);                                               // Default is 500
    m_pPools->SetPoolCapacity(OBJECT_POOL, MAX_OBJECTS);                                      // Default is 350
    m_pPools->SetPoolCapacity(EVENT_POOL, 5000);                                              // Default is 200
    m_pPools->SetPoolCapacity(COL_MODEL_POOL, 12000);                                         // Default is 10150
    m_pPools->SetPoolCapacity(ENV_MAP_MATERIAL_POOL, 16000);                                  // Default is 4096
    m_pPools->SetPoolCapacity(ENV_MAP_ATOMIC_POOL, 4000);                                     // Default is 1024
    m_pPools->SetPoolCapacity(SPEC_MAP_MATERIAL_POOL, 16000);                                 // Default is 4096
    m_pPools->SetPoolCapacity(ENTRY_INFO_NODE_POOL, MAX_ENTRY_INFO_NODES);                    // Default is 500
    m_pPools->SetPoolCapacity(POINTER_SINGLE_LINK_POOL, MAX_POINTER_SINGLE_LINKS);            // Default is 70000
    m_pPools->SetPoolCapacity(POINTER_DOUBLE_LINK_POOL, MAX_POINTER_DOUBLE_LINKS);            // Default is 3200
    dassert(m_pPools->GetPoolCapacity(POINTER_SINGLE_LINK_POOL) == MAX_POINTER_SINGLE_LINKS);

    // Increase streaming object instances list size
    MemPut<WORD>(0x05B8E55, MAX_RWOBJECT_INSTANCES * 12);            // Default is 1000 * 12
    MemPut<WORD>(0x05B8EB0, MAX_RWOBJECT_INSTANCES * 12);            // Default is 1000 * 12

    // Increase matrix array size
    MemPut<int>(0x054F3A1, MAX_OBJECTS * 3);            // Default is 900

    CEntitySAInterface::StaticSetHooks();
    CPhysicalSAInterface::StaticSetHooks();
    CObjectSA::StaticSetHooks();
    CModelInfoSA::StaticSetHooks();
    CPlayerPedSA::StaticSetHooks();
    CRenderWareSA::StaticSetHooks();
    CRenderWareSA::StaticSetClothesReplacingHooks();
    CTasksSA::StaticSetHooks();
    CPedSA::StaticSetHooks();
    CSettingsSA::StaticSetHooks();
    CFxSystemSA::StaticSetHooks();
    CFileLoaderSA::StaticSetHooks();
    D3DResourceSystemSA::StaticSetHooks();
}

CGameSA::~CGameSA()
{
    delete reinterpret_cast<CPlayerInfoSA*>(m_pPlayerInfo);

    for (int i = 0; i < NUM_WeaponInfosTotal; i++)
    {
        delete reinterpret_cast<CWeaponInfoSA*>(WeaponInfos[i]);
    }

    delete reinterpret_cast<CFxSA*>(m_pFx);
    delete reinterpret_cast<CRopesSA*>(m_pRopes);
    delete reinterpret_cast<CKeyGenSA*>(m_pKeyGen);
    delete reinterpret_cast<CVisibilityPluginsSA*>(m_pVisibilityPlugins);
    delete reinterpret_cast<CStreamingSA*>(m_pStreaming);
    delete reinterpret_cast<CAnimManagerSA*>(m_pAnimManager);
    delete reinterpret_cast<CTasksSA*>(m_pTasks);
    delete reinterpret_cast<CTaskManagementSystemSA*>(m_pTaskManagementSystem);
    delete reinterpret_cast<CHandlingManagerSA*>(m_pHandlingManager);
    delete reinterpret_cast<CPopulationSA*>(m_pPopulation);
    delete reinterpret_cast<CPathFindSA*>(m_pPathFind);
    delete reinterpret_cast<CFontSA*>(m_pFont);
    delete reinterpret_cast<CStatsSA*>(m_pStats);
    delete reinterpret_cast<CMenuManagerSA*>(m_pMenuManager);
    delete reinterpret_cast<CWeatherSA*>(m_pWeather);
    delete reinterpret_cast<CAERadioTrackManagerSA*>(m_pCAERadioTrackManager);
    delete reinterpret_cast<CTheCarGeneratorsSA*>(m_pTheCarGenerators);
    delete reinterpret_cast<CPadSA*>(m_pPad);
    delete reinterpret_cast<C3DMarkersSA*>(m_p3DMarkers);
    delete reinterpret_cast<CFireManagerSA*>(m_pFireManager);
    delete reinterpret_cast<CHudSA*>(m_pHud);
    delete reinterpret_cast<CExplosionManagerSA*>(m_pExplosionManager);
    delete reinterpret_cast<CPickupsSA*>(m_pPickups);
    delete reinterpret_cast<CCheckpointsSA*>(m_pCheckpoints);
    delete reinterpret_cast<CCoronasSA*>(m_pCoronas);
    delete reinterpret_cast<CCameraSA*>(m_pCamera);
    delete reinterpret_cast<CRadarSA*>(m_pRadar);
    delete reinterpret_cast<CClockSA*>(m_pClock);
    delete reinterpret_cast<CPoolsSA*>(m_pPools);
    delete reinterpret_cast<CWorldSA*>(m_pWorld);
    delete reinterpret_cast<CAudioEngineSA*>(m_pAudioEngine);
    delete reinterpret_cast<CAEAudioHardwareSA*>(m_pAEAudioHardware);
    delete reinterpret_cast<CAudioContainerSA*>(m_pAudioContainer);
    delete reinterpret_cast<CPointLightsSA*>(m_pPointLights);
    delete static_cast<CColStoreSA*>(m_collisionStore);

    delete[] ModelInfo;
    delete[] ObjectGroupsInfo;
}

CWeaponInfo* CGameSA::GetWeaponInfo(eWeaponType weapon, eWeaponSkill skill)
{
    DEBUG_TRACE("CWeaponInfo * CGameSA::GetWeaponInfo(eWeaponType weapon)");

    if ((skill == WEAPONSKILL_STD && weapon >= WEAPONTYPE_UNARMED && weapon < WEAPONTYPE_LAST_WEAPONTYPE) ||
        (skill != WEAPONSKILL_STD && weapon >= WEAPONTYPE_PISTOL && weapon <= WEAPONTYPE_TEC9))
    {
        int offset = 0;
        switch (skill)
        {
            case WEAPONSKILL_STD:
                offset = 0;
                break;
            case WEAPONSKILL_POOR:
                offset = NUM_WeaponInfosStdSkill - WEAPONTYPE_PISTOL;
                break;
            case WEAPONSKILL_PRO:
                offset = NUM_WeaponInfosStdSkill + NUM_WeaponInfosOtherSkill - WEAPONTYPE_PISTOL;
                break;
            case WEAPONSKILL_SPECIAL:
                offset = NUM_WeaponInfosStdSkill + 2 * NUM_WeaponInfosOtherSkill - WEAPONTYPE_PISTOL;
                break;
            default:
                break;
        }
        return WeaponInfos[offset + weapon];
    }
    else
        return NULL;
}

VOID CGameSA::Pause(bool bPaused)
{
    *VAR_GamePaused = bPaused;
}

bool CGameSA::IsPaused()
{
    return *VAR_GamePaused;
}

bool CGameSA::IsInForeground()
{
    return *VAR_IsForegroundWindow;
}

CModelInfo* CGameSA::GetModelInfo(DWORD dwModelID, bool bCanBeInvalid)
{
    DEBUG_TRACE("CModelInfo * CGameSA::GetModelInfo(DWORD dwModelID, bool bCanBeInvalid)");
    if (dwModelID < GetCountOfAllFileIDs())
    {
        if (ModelInfo[dwModelID].IsValid() || bCanBeInvalid)
        {
            return &ModelInfo[dwModelID];
        }
        return nullptr;
    }
    return nullptr;
}

/**
 * Starts the game
 * \todo make addresses into constants
 */
VOID CGameSA::StartGame()
{
    DEBUG_TRACE("VOID CGameSA::StartGame()");
    //  InitScriptInterface();
    //*(BYTE *)VAR_StartGame = 1;
    this->SetSystemState(GS_INIT_PLAYING_GAME);
    MemPutFast<BYTE>(0xB7CB49, 0);
    MemPutFast<BYTE>(0xBA67A4, 0);
}

/**
 * Sets the part of the game loading process the game is in.
 * @param dwState DWORD containing a valid state 0 - 9
 */
VOID CGameSA::SetSystemState(eSystemState State)
{
    DEBUG_TRACE("VOID CGameSA::SetSystemState( eSystemState State )");
    *VAR_SystemState = (DWORD)State;
}

eSystemState CGameSA::GetSystemState()
{
    DEBUG_TRACE("eSystemState CGameSA::GetSystemState( )");
    return (eSystemState)*VAR_SystemState;
}

/**
 * This adds the local player to the ped pool, nothing else
 * @return BOOL TRUE if success, FALSE otherwise
 */
BOOL CGameSA::InitLocalPlayer(CClientPed* pClientPed)
{
    DEBUG_TRACE("BOOL CGameSA::InitLocalPlayer(  )");

    CPoolsSA* pools = (CPoolsSA*)this->GetPools();
    if (pools)
    {
        //* HACKED IN HERE FOR NOW *//
        CPedSAInterface* pInterface = pools->GetPedInterface((DWORD)1);

        if (pInterface)
        {
            pools->ResetPedPoolCount();
            pools->AddPed(pClientPed, (DWORD*)pInterface);
            return TRUE;
        }

        return FALSE;
    }
    return FALSE;
}

float CGameSA::GetGravity()
{
    return *(float*)(0x863984);
}

void CGameSA::SetGravity(float fGravity)
{
    MemPut<float>(0x863984, fGravity);
}

float CGameSA::GetGameSpeed()
{
    return *(float*)(0xB7CB64);
}

void CGameSA::SetGameSpeed(float fSpeed)
{
    MemPutFast<float>(0xB7CB64, fSpeed);
}

// this prevents some crashes (respawning mainly)
VOID CGameSA::DisableRenderer(bool bDisabled)
{
    // ENABLED:
    // 0053DF40   D915 2C13C800    FST DWORD PTR DS:[C8132C]
    // DISABLED:
    // 0053DF40   C3               RETN

    if (bDisabled)
    {
        MemPut<BYTE>(0x53DF40, 0xC3);
    }
    else
    {
        MemPut<BYTE>(0x53DF40, 0xD9);
    }
}

VOID CGameSA::SetRenderHook(InRenderer* pInRenderer)
{
    if (pInRenderer)
        HookInstall((DWORD)FUNC_CDebug_DebugDisplayTextBuffer, (DWORD)pInRenderer, 6);
    else
    {
        MemPut<BYTE>(FUNC_CDebug_DebugDisplayTextBuffer, 0xC3);
    }
}

VOID CGameSA::TakeScreenshot(char* szFileName)
{
    DWORD dwFunc = FUNC_JPegCompressScreenToFile;
    _asm
    {
        mov     eax, CLASS_RwCamera
        mov     eax, [eax]
        push    szFileName
        push    eax
        call    dwFunc
        add     esp,8
    }
}

DWORD* CGameSA::GetMemoryValue(DWORD dwOffset)
{
    if (dwOffset <= MAX_MEMORY_OFFSET_1_0)
        return (DWORD*)dwOffset;
    else
        return NULL;
}

void CGameSA::Reset()
{
    // Things to do if the game was loaded
    if (GetSystemState() == GS_PLAYING_GAME)
    {
        // Extinguish all fires
        m_pFireManager->ExtinguishAllFires();

        // Restore camera stuff
        m_pCamera->Restore();
        m_pCamera->SetFadeColor(0, 0, 0);
        m_pCamera->Fade(0, FADE_OUT);

        Pause(false);            // We don't have to pause as the fadeout will stop the sound. Pausing it will make the fadein next start ugly
        m_pHud->Disable(false);

        DisableRenderer(false);

        // Restore the HUD
        m_pHud->Disable(false);
        m_pHud->SetComponentVisible(HUD_ALL, true);

        // Restore model dummies' positions
        CModelInfoSA::ResetAllVehicleDummies();
        CModelInfoSA::RestoreAllObjectsPropertiesGroups();
        // restore default properties of all CObjectGroupPhysicalPropertiesSA instances
        CObjectGroupPhysicalPropertiesSA::RestoreDefaultValues();

        // Restore vehicle model wheel sizes
        CModelInfoSA::ResetAllVehiclesWheelSizes();
    }
}

void CGameSA::Terminate()
{
    // Initiate the destruction
    delete this;

    // Dump any memory leaks if DETECT_LEAK is defined
    #ifdef DETECT_LEAKS
    DumpUnfreed();
    #endif
}

void CGameSA::Initialize()
{
    // Initialize garages
    m_pGarages->Initialize();
    SetupSpecialCharacters();
    SetupBrokenModels();
    m_pRenderWare->Initialize();

    // *Sebas* Hide the GTA:SA Main menu.
    MemPutFast<BYTE>(CLASS_CMenuManager + 0x5C, 0);
}

eGameVersion CGameSA::GetGameVersion()
{
    return m_eGameVersion;
}

eGameVersion CGameSA::FindGameVersion()
{
    unsigned char ucA = *reinterpret_cast<unsigned char*>(0x748ADD);
    unsigned char ucB = *reinterpret_cast<unsigned char*>(0x748ADE);
    if (ucA == 0xFF && ucB == 0x53)
    {
        m_eGameVersion = VERSION_US_10;
    }
    else if (ucA == 0x0F && ucB == 0x84)
    {
        m_eGameVersion = VERSION_EU_10;
    }
    else if (ucA == 0xFE && ucB == 0x10)
    {
        m_eGameVersion = VERSION_11;
    }
    else
    {
        m_eGameVersion = VERSION_UNKNOWN;
    }

    return m_eGameVersion;
}

float CGameSA::GetFPS()
{
    return *VAR_FPS;
}

float CGameSA::GetTimeStep()
{
    return *VAR_TimeStep;
}

float CGameSA::GetOldTimeStep()
{
    return *VAR_OldTimeStep;
}

float CGameSA::GetTimeScale()
{
    return *VAR_TimeScale;
}

void CGameSA::SetTimeScale(float fTimeScale)
{
    *VAR_TimeScale = fTimeScale;
}

unsigned char CGameSA::GetBlurLevel()
{
    return *(unsigned char*)0x8D5104;
}

void CGameSA::SetBlurLevel(unsigned char ucLevel)
{
    MemPutFast<unsigned char>(0x8D5104, ucLevel);
}

unsigned long CGameSA::GetMinuteDuration()
{
    return *(unsigned long*)0xB7015C;
}

void CGameSA::SetMinuteDuration(unsigned long ulTime)
{
    MemPutFast<unsigned long>(0xB7015C, ulTime);
}

bool CGameSA::IsCheatEnabled(const char* szCheatName)
{
    if (!strcmp(szCheatName, PROP_RANDOM_FOLIAGE))
        return IsRandomFoliageEnabled();

    if (!strcmp(szCheatName, PROP_SNIPER_MOON))
        return IsMoonEasterEggEnabled();

    if (!strcmp(szCheatName, PROP_EXTRA_AIR_RESISTANCE))
        return IsExtraAirResistanceEnabled();

    if (!strcmp(szCheatName, PROP_UNDERWORLD_WARP))
        return IsUnderWorldWarpEnabled();

    std::map<std::string, SCheatSA*>::iterator it = m_Cheats.find(szCheatName);
    if (it == m_Cheats.end())
        return false;
    return *(it->second->m_byAddress) != 0;
}

bool CGameSA::SetCheatEnabled(const char* szCheatName, bool bEnable)
{
    if (!strcmp(szCheatName, PROP_RANDOM_FOLIAGE))
    {
        SetRandomFoliageEnabled(bEnable);
        return true;
    }

    if (!strcmp(szCheatName, PROP_SNIPER_MOON))
    {
        SetMoonEasterEggEnabled(bEnable);
        return true;
    }

    if (!strcmp(szCheatName, PROP_EXTRA_AIR_RESISTANCE))
    {
        SetExtraAirResistanceEnabled(bEnable);
        return true;
    }

    if (!strcmp(szCheatName, PROP_UNDERWORLD_WARP))
    {
        SetUnderWorldWarpEnabled(bEnable);
        return true;
    }

    std::map<std::string, SCheatSA*>::iterator it = m_Cheats.find(szCheatName);
    if (it == m_Cheats.end())
        return false;
    if (!it->second->m_bCanBeSet)
        return false;
    MemPutFast<BYTE>(it->second->m_byAddress, bEnable);
    it->second->m_bEnabled = bEnable;
    return true;
}

void CGameSA::ResetCheats()
{
    SetRandomFoliageEnabled(true);
    SetMoonEasterEggEnabled(false);
    SetExtraAirResistanceEnabled(true);
    SetUnderWorldWarpEnabled(true);

    std::map<std::string, SCheatSA*>::iterator it;
    for (it = m_Cheats.begin(); it != m_Cheats.end(); it++)
    {
        if (it->second->m_byAddress > (BYTE*)0x8A4000)
            MemPutFast<BYTE>(it->second->m_byAddress, 0);
        else
            MemPut<BYTE>(it->second->m_byAddress, 0);
        it->second->m_bEnabled = false;
    }
}

bool CGameSA::IsRandomFoliageEnabled()
{
    return *(unsigned char*)0x5DD01B == 0x74;
}

void CGameSA::SetRandomFoliageEnabled(bool bEnabled)
{
    // 0xEB skip random foliage generation
    MemPut<BYTE>(0x5DD01B, bEnabled ? 0x74 : 0xEB);
    // 0x74 destroy random foliage loaded
    MemPut<BYTE>(0x5DC536, bEnabled ? 0x75 : 0x74);
}

bool CGameSA::IsMoonEasterEggEnabled()
{
    return *(unsigned char*)0x73ABCF == 0x75;
}

void CGameSA::SetMoonEasterEggEnabled(bool bEnable)
{
    // replace JNZ with JMP (short)
    MemPut<BYTE>(0x73ABCF, bEnable ? 0x75 : 0xEB);
}

bool CGameSA::IsExtraAirResistanceEnabled()
{
    return *(unsigned char*)0x72DDD9 == 0x01;
}

void CGameSA::SetExtraAirResistanceEnabled(bool bEnable)
{
    MemPut<BYTE>(0x72DDD9, bEnable ? 0x01 : 0x00);
}

void CGameSA::SetUnderWorldWarpEnabled(bool bEnable)
{
    m_bUnderworldWarp = !bEnable;
}

bool CGameSA::IsUnderWorldWarpEnabled()
{
    return !m_bUnderworldWarp;
}

bool CGameSA::GetJetpackWeaponEnabled(eWeaponType weaponType)
{
    if (weaponType >= WEAPONTYPE_BRASSKNUCKLE && weaponType < WEAPONTYPE_LAST_WEAPONTYPE)
    {
        return m_JetpackWeapons[weaponType];
    }
    return false;
}

void CGameSA::SetJetpackWeaponEnabled(eWeaponType weaponType, bool bEnabled)
{
    if (weaponType >= WEAPONTYPE_BRASSKNUCKLE && weaponType < WEAPONTYPE_LAST_WEAPONTYPE)
    {
        m_JetpackWeapons[weaponType] = bEnabled;
    }
}

bool CGameSA::PerformChecks()
{
    std::map<std::string, SCheatSA*>::iterator it;
    for (it = m_Cheats.begin(); it != m_Cheats.end(); it++)
    {
        if (*(it->second->m_byAddress) != BYTE(it->second->m_bEnabled))
            return false;
    }
    return true;
}
bool CGameSA::VerifySADataFileNames()
{
    return !strcmp(*(char**)0x5B65AE, "DATA\\CARMODS.DAT") && !strcmp(*(char**)0x5BD839, "DATA") && !strcmp(*(char**)0x5BD84C, "HANDLING.CFG") &&
           !strcmp(*(char**)0x5BEEE8, "DATA\\melee.dat") && !strcmp(*(char**)0x4D563E, "ANIM\\PED.IFP") && !strcmp(*(char**)0x5B925B, "DATA\\OBJECT.DAT") &&
           !strcmp(*(char**)0x55D0FC, "data\\surface.dat") && !strcmp(*(char**)0x55F2BB, "data\\surfaud.dat") &&
           !strcmp(*(char**)0x55EB9E, "data\\surfinfo.dat") && !strcmp(*(char**)0x6EAEF8, "DATA\\water.dat") &&
           !strcmp(*(char**)0x6EAEC3, "DATA\\water1.dat") && !strcmp(*(char**)0x5BE686, "DATA\\WEAPON.DAT");
}

void CGameSA::SetAsyncLoadingFromScript(bool bScriptEnabled, bool bScriptForced)
{
    m_bAsyncScriptEnabled = bScriptEnabled;
    m_bAsyncScriptForced = bScriptForced;
}

void CGameSA::SuspendASyncLoading(bool bSuspend, uint uiAutoUnsuspendDelay)
{
    m_bASyncLoadingSuspended = bSuspend;
    // Setup auto unsuspend time if required
    if (uiAutoUnsuspendDelay && bSuspend)
        m_llASyncLoadingAutoUnsuspendTime = CTickCount::Now() + CTickCount((long long)uiAutoUnsuspendDelay);
    else
        m_llASyncLoadingAutoUnsuspendTime = CTickCount();
}

bool CGameSA::IsASyncLoadingEnabled(bool bIgnoreSuspend)
{
    // Process auto unsuspend time if set
    if (m_llASyncLoadingAutoUnsuspendTime.ToLongLong() != 0)
    {
        if (CTickCount::Now() > m_llASyncLoadingAutoUnsuspendTime)
        {
            m_llASyncLoadingAutoUnsuspendTime = CTickCount();
            m_bASyncLoadingSuspended = false;
        }
    }

    if (m_bASyncLoadingSuspended && !bIgnoreSuspend)
        return false;

    if (m_bAsyncScriptForced)
        return m_bAsyncScriptEnabled;
    return true;
}

void CGameSA::SetupSpecialCharacters()
{
    ModelInfo[1].MakePedModel("TRUTH");
    ModelInfo[2].MakePedModel("MACCER");
    // ModelInfo[190].MakePedModel ( "BARBARA" );
    // ModelInfo[191].MakePedModel ( "HELENA" );
    // ModelInfo[192].MakePedModel ( "MICHELLE" );
    // ModelInfo[193].MakePedModel ( "KATIE" );
    // ModelInfo[194].MakePedModel ( "MILLIE" );
    // ModelInfo[195].MakePedModel ( "DENISE" );
    ModelInfo[265].MakePedModel("TENPEN");
    ModelInfo[266].MakePedModel("PULASKI");
    ModelInfo[267].MakePedModel("HERN");
    ModelInfo[268].MakePedModel("DWAYNE");
    ModelInfo[269].MakePedModel("SMOKE");
    ModelInfo[270].MakePedModel("SWEET");
    ModelInfo[271].MakePedModel("RYDER");
    ModelInfo[272].MakePedModel("FORELLI");
    ModelInfo[290].MakePedModel("ROSE");
    ModelInfo[291].MakePedModel("PAUL");
    ModelInfo[292].MakePedModel("CESAR");
    ModelInfo[293].MakePedModel("OGLOC");
    ModelInfo[294].MakePedModel("WUZIMU");
    ModelInfo[295].MakePedModel("TORINO");
    ModelInfo[296].MakePedModel("JIZZY");
    ModelInfo[297].MakePedModel("MADDOGG");
    ModelInfo[298].MakePedModel("CAT");
    ModelInfo[299].MakePedModel("CLAUDE");
    ModelInfo[300].MakePedModel("RYDER2");
    ModelInfo[301].MakePedModel("RYDER3");
    ModelInfo[302].MakePedModel("EMMET");
    ModelInfo[303].MakePedModel("ANDRE");
    ModelInfo[304].MakePedModel("KENDL");
    ModelInfo[305].MakePedModel("JETHRO");
    ModelInfo[306].MakePedModel("ZERO");
    ModelInfo[307].MakePedModel("TBONE");
    ModelInfo[308].MakePedModel("SINDACO");
    ModelInfo[309].MakePedModel("JANITOR");
    ModelInfo[310].MakePedModel("BBTHIN");
    ModelInfo[311].MakePedModel("SMOKEV");
    ModelInfo[312].MakePedModel("PSYCHO");
    /* Hot-coffee only models
    ModelInfo[313].MakePedModel ( "GANGRL2" );
    ModelInfo[314].MakePedModel ( "MECGRL2" );
    ModelInfo[315].MakePedModel ( "GUNGRL2" );
    ModelInfo[316].MakePedModel ( "COPGRL2" );
    ModelInfo[317].MakePedModel ( "NURGRL2" );
    */
}

void CGameSA::FixModelCol(uint iFixModel, uint iFromModel)
{
    CBaseModelInfoSAInterface* pFixModelInterface = ModelInfo[iFixModel].GetInterface();
    if (!pFixModelInterface || pFixModelInterface->pColModel)
        return;

    CBaseModelInfoSAInterface* pAviableModelInterface = ModelInfo[iFromModel].GetInterface();

    if (!pAviableModelInterface)
        return;

    pFixModelInterface->pColModel = pAviableModelInterface->pColModel;
}

void CGameSA::SetupBrokenModels()
{
    FixModelCol(3118, 3059);
    FixModelCol(3553, 3554);
}

// Well, has it?
bool CGameSA::HasCreditScreenFadedOut()
{
    BYTE ucAlpha = *(BYTE*)0xBAB320;
    bool bCreditScreenFadedOut = (GetSystemState() >= 7) && (ucAlpha < 6);
    return bCreditScreenFadedOut;
}

// Ensure replaced/restored textures for models in the GTA map are correct
void CGameSA::FlushPendingRestreamIPL()
{
    CModelInfoSA::StaticFlushPendingRestreamIPL();
    m_pRenderWare->ResetStats();
}

void CGameSA::GetShaderReplacementStats(SShaderReplacementStats& outStats)
{
    m_pRenderWare->GetShaderReplacementStats(outStats);
}

// Ensure models have the default lod distances
void CGameSA::ResetModelLodDistances()
{
    CModelInfoSA::StaticResetLodDistances();
}

void CGameSA::ResetModelTimes()
{
    CModelInfoSA::StaticResetModelTimes();
}

void CGameSA::ResetAlphaTransparencies()
{
    CModelInfoSA::StaticResetAlphaTransparencies();
}

// Disable VSync by forcing what normally happends at the end of the loading screens
// Note #1: This causes the D3D device to be reset after the next frame
// Note #2: Some players do not need this to disable VSync. (Possibly because their video card driver settings override it somewhere)
void CGameSA::DisableVSync()
{
    MemPutFast<BYTE>(0xBAB318, 0);
}
CWeapon* CGameSA::CreateWeapon()
{
    return new CWeaponSA(new CWeaponSAInterface, NULL, WEAPONSLOT_MAX);
}

CWeaponStat* CGameSA::CreateWeaponStat(eWeaponType weaponType, eWeaponSkill weaponSkill)
{
    return m_pWeaponStatsManager->CreateWeaponStatUnlisted(weaponType, weaponSkill);
}

void CGameSA::OnPedContextChange(CPed* pPedContext)
{
    m_pPedContext = pPedContext;
}

CPed* CGameSA::GetPedContext()
{
    if (!m_pPedContext)
        m_pPedContext = pGame->GetPools()->GetPedFromRef((DWORD)1);
    return m_pPedContext;
}

CObjectGroupPhysicalProperties* CGameSA::GetObjectGroupPhysicalProperties(unsigned char ucObjectGroup)
{
    DEBUG_TRACE("CObjectGroupPhysicalProperties * CGameSA::GetObjectGroupPhysicalProperties(unsigned char ucObjectGroup)");
    if (ucObjectGroup < OBJECTDYNAMICINFO_MAX && ObjectGroupsInfo[ucObjectGroup].IsValid())
        return &ObjectGroupsInfo[ucObjectGroup];

    return nullptr;
}
