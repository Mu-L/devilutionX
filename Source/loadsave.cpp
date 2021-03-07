/**
 * @file loadsave.cpp
 *
 * Implementation of save game functionality.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

bool gbIsHellfireSaveGame;
int giNumberOfLevels;
int giNumberQuests;
int giNumberOfSmithPremiumItems;

namespace {

Uint16 SwapLE(Uint16 in)
{
	return SDL_SwapLE16(in);
}

Uint32 SwapLE(Uint32 in)
{
	return SDL_SwapLE32(in);
}

Uint64 SwapLE(Uint64 in)
{
	return SDL_SwapLE64(in);
}

Uint16 SwapBE(Uint16 in)
{
	return SDL_SwapBE16(in);
}

Uint32 SwapBE(Uint32 in)
{
	return SDL_SwapBE32(in);
}

Uint64 SwapBE(Uint64 in)
{
	return SDL_SwapBE64(in);
}

class LoadHelper {
	Uint8 *m_buffer;
	Uint32 m_bufferPtr = 0;
	Uint32 m_bufferLen;

public:
	LoadHelper(const char *szFileName)
	{
		m_buffer = pfile_read(szFileName, &m_bufferLen);
	}

	bool isValid(Uint32 size = 1)
	{
		return m_buffer != nullptr
		    && m_bufferLen >= (m_bufferPtr + size);
	}

	void skip(Uint32 size)
	{
		m_bufferPtr += size;
	}

	void nextBytes(void *bytes, size_t size)
	{
		if (!isValid(size))
			return;

		memcpy(bytes, &m_buffer[m_bufferPtr], size);
		m_bufferPtr += size;
	}

	template <class T>
	T next()
	{
		const auto size = sizeof(T);
		if (!isValid(size))
			return 0;

		T value;
		memcpy(&value, &m_buffer[m_bufferPtr], size);
		m_bufferPtr += size;

		return value;
	}

	template <class T>
	T nextBE()
	{
		return SwapBE(next<T>());
	}

	template <class T>
	T nextLE()
	{
		return SwapLE(next<T>());
	}

	bool nextBool8()
	{
		return next<Uint8>() != 0;
	}

	bool nextBool32()
	{
		return next<Uint32>() != 0;
	}

	~LoadHelper()
	{
		mem_free_dbg(m_buffer);
	}
};

class SaveHelper {
	const char *m_szFileName;
	Uint8 *m_buffer;
	Uint32 m_bufferPtr = 0;
	Uint32 m_bufferLen;

public:
	SaveHelper(const char *szFileName, size_t bufferLen)
	{
		m_szFileName = szFileName;
		m_bufferLen = bufferLen;
		m_buffer = DiabloAllocPtr(codec_get_encoded_len(m_bufferLen));
	}

	bool isValid(Uint32 len = 1)
	{
		return m_buffer != nullptr
		    && m_bufferLen >= (m_bufferPtr + len);
	}

	void skip(Uint32 len)
	{
		m_bufferPtr += len;
	}

	void writeBytes(const void *bytes, size_t len)
	{
		if (!isValid(len))
			return;

		memcpy(&m_buffer[m_bufferPtr], bytes, len);
		m_bufferPtr += len;
	}

	void writeByte(Uint8 value)
	{
		writeBytes(&value, sizeof(value));
	}

	template <class T>
	void writeLE(T value)
	{
		value = SwapLE(value);
		writeBytes(&value, sizeof(value));
	}

	template <class T>
	void writeBE(T value)
	{
		value = SwapBE(value);
		writeBytes(&value, sizeof(value));
	}

	void flush()
	{
		if (m_buffer == nullptr)
			return;

		pfile_write_save_file(m_szFileName, m_buffer, m_bufferPtr, codec_get_encoded_len(m_bufferPtr));
		mem_free_dbg(m_buffer);
		m_buffer = nullptr;
	}

	~SaveHelper()
	{
		flush();
	}
};

}

static void LoadItemData(LoadHelper *file, ItemStruct *pItem)
{
	pItem->_iSeed = file->nextLE<Uint32>();
	pItem->_iCreateInfo = file->nextLE<Uint16>();
	file->skip(2); // Alignment
	pItem->_itype = (item_type)file->nextLE<Uint32>();
	pItem->_ix = file->nextLE<Uint32>();
	pItem->_iy = file->nextLE<Uint32>();
	pItem->_iAnimFlag = file->nextBool32();
	file->skip(4); // Skip pointer _iAnimData
	pItem->_iAnimLen = file->nextLE<Uint32>();
	pItem->_iAnimFrame = file->nextLE<Uint32>();
	pItem->_iAnimWidth = file->nextLE<Uint32>();
	pItem->_iAnimWidth2 = file->nextLE<Uint32>();
	file->skip(4); // Unused since 1.02
	pItem->_iSelFlag = file->next<Uint8>();
	file->skip(3); // Alignment
	pItem->_iPostDraw = file->nextBool32();
	pItem->_iIdentified = file->nextBool32();
	pItem->_iMagical = file->next<Uint8>();
	file->nextBytes(pItem->_iName, 64);
	file->nextBytes(pItem->_iIName, 64);
	pItem->_iLoc = (item_equip_type)file->next<Uint8>();
	pItem->_iClass = (item_class)file->next<Uint8>();
	file->skip(1); // Alignment
	pItem->_iCurs = file->nextLE<Uint32>();
	pItem->_ivalue = file->nextLE<Uint32>();
	pItem->_iIvalue = file->nextLE<Uint32>();
	pItem->_iMinDam = file->nextLE<Uint32>();
	pItem->_iMaxDam = file->nextLE<Uint32>();
	pItem->_iAC = file->nextLE<Uint32>();
	pItem->_iFlags = file->nextLE<Uint32>();
	pItem->_iMiscId = (item_misc_id)file->nextLE<Uint32>();
	pItem->_iSpell = (spell_id)file->nextLE<Uint32>();
	pItem->_iCharges = file->nextLE<Uint32>();
	pItem->_iMaxCharges = file->nextLE<Uint32>();
	pItem->_iDurability = file->nextLE<Uint32>();
	pItem->_iMaxDur = file->nextLE<Uint32>();
	pItem->_iPLDam = file->nextLE<Uint32>();
	pItem->_iPLToHit = file->nextLE<Uint32>();
	pItem->_iPLAC = file->nextLE<Uint32>();
	pItem->_iPLStr = file->nextLE<Uint32>();
	pItem->_iPLMag = file->nextLE<Uint32>();
	pItem->_iPLDex = file->nextLE<Uint32>();
	pItem->_iPLVit = file->nextLE<Uint32>();
	pItem->_iPLFR = file->nextLE<Uint32>();
	pItem->_iPLLR = file->nextLE<Uint32>();
	pItem->_iPLMR = file->nextLE<Uint32>();
	pItem->_iPLMana = file->nextLE<Uint32>();
	pItem->_iPLHP = file->nextLE<Uint32>();
	pItem->_iPLDamMod = file->nextLE<Uint32>();
	pItem->_iPLGetHit = file->nextLE<Uint32>();
	pItem->_iPLLight = file->nextLE<Uint32>();
	pItem->_iSplLvlAdd = file->next<Uint8>();
	pItem->_iRequest = file->next<Uint8>();
	file->skip(2); // Alignment
	pItem->_iUid = file->nextLE<Uint32>();
	pItem->_iFMinDam = file->nextLE<Uint32>();
	pItem->_iFMaxDam = file->nextLE<Uint32>();
	pItem->_iLMinDam = file->nextLE<Uint32>();
	pItem->_iLMaxDam = file->nextLE<Uint32>();
	pItem->_iPLEnAc = file->nextLE<Uint32>();
	pItem->_iPrePower = (item_effect_type)file->next<Uint8>();
	pItem->_iSufPower = (item_effect_type)file->next<Uint8>();
	file->skip(2); // Alignment
	pItem->_iVAdd1 = file->nextLE<Uint32>();
	pItem->_iVMult1 = file->nextLE<Uint32>();
	pItem->_iVAdd2 = file->nextLE<Uint32>();
	pItem->_iVMult2 = file->nextLE<Uint32>();
	pItem->_iMinStr = file->next<Uint8>();
	pItem->_iMinMag = file->next<Uint8>();
	pItem->_iMinDex = file->next<Uint8>();
	file->skip(1); // Alignment
	pItem->_iStatFlag = file->nextBool32();
	pItem->IDidx = file->nextLE<Uint32>();
	if (!gbIsHellfireSaveGame) {
		pItem->IDidx = RemapItemIdxFromDiablo(pItem->IDidx);
	}
	file->skip(4); // Unused
	if (gbIsHellfireSaveGame)
		pItem->_iDamAcFlags = file->nextLE<Uint32>();
	else
		pItem->_iDamAcFlags = 0;

	if (!IsItemAvailable(pItem->IDidx)) {
		pItem->IDidx = 0;
		pItem->_itype = ITYPE_NONE;
	}
}

static void LoadItems(LoadHelper *file, const int n, ItemStruct *pItem)
{
	for (int i = 0; i < n; i++) {
		LoadItemData(file, &pItem[i]);
	}
}

static void LoadPlayer(LoadHelper *file, int p)
{
	PlayerStruct *pPlayer = &plr[p];

	pPlayer->_pmode = file->nextLE<Uint32>();

	for (int i = 0; i < MAX_PATH_LENGTH; i++) {
		pPlayer->walkpath[i] = file->next<Uint8>();
	}
	pPlayer->plractive = file->nextBool8();
	file->skip(2); // Alignment
	pPlayer->destAction = file->nextLE<Uint32>();
	pPlayer->destParam1 = file->nextLE<Uint32>();
	pPlayer->destParam2 = file->nextLE<Uint32>();
	pPlayer->destParam3 = file->nextLE<Uint32>();
	pPlayer->destParam4 = file->nextLE<Uint32>();
	pPlayer->plrlevel = file->nextLE<Uint32>();
	pPlayer->_px = file->nextLE<Uint32>();
	pPlayer->_py = file->nextLE<Uint32>();
	pPlayer->_pfutx = file->nextLE<Uint32>();
	pPlayer->_pfuty = file->nextLE<Uint32>();
	pPlayer->_ptargx = file->nextLE<Uint32>();
	pPlayer->_ptargy = file->nextLE<Uint32>();
	pPlayer->_pownerx = file->nextLE<Uint32>();
	pPlayer->_pownery = file->nextLE<Uint32>();
	pPlayer->_poldx = file->nextLE<Uint32>();
	pPlayer->_poldy = file->nextLE<Uint32>();
	pPlayer->_pxoff = file->nextLE<Uint32>();
	pPlayer->_pyoff = file->nextLE<Uint32>();
	pPlayer->_pxvel = file->nextLE<Uint32>();
	pPlayer->_pyvel = file->nextLE<Uint32>();
	pPlayer->_pdir = file->nextLE<Uint32>();
	file->skip(4); // Unused
	pPlayer->_pgfxnum = file->nextLE<Uint32>();
	file->skip(4); // Skip pointer _pAnimData
	pPlayer->_pAnimDelay = file->nextLE<Uint32>();
	pPlayer->_pAnimCnt = file->nextLE<Uint32>();
	pPlayer->_pAnimLen = file->nextLE<Uint32>();
	pPlayer->_pAnimFrame = file->nextLE<Uint32>();
	pPlayer->_pAnimWidth = file->nextLE<Uint32>();
	pPlayer->_pAnimWidth2 = file->nextLE<Uint32>();
	file->skip(4); // Skip _peflag
	pPlayer->_plid = file->nextLE<Uint32>();
	pPlayer->_pvid = file->nextLE<Uint32>();

	pPlayer->_pSpell = file->nextLE<Uint32>();
	pPlayer->_pSplType = file->next<Uint8>();
	pPlayer->_pSplFrom = file->next<Uint8>();
	file->skip(2); // Alignment
	pPlayer->_pTSpell = file->nextLE<Uint32>();
	pPlayer->_pTSplType = file->next<Uint8>();
	file->skip(3); // Alignment
	pPlayer->_pRSpell = file->nextLE<Uint32>();
	pPlayer->_pRSplType = file->next<Uint8>();
	file->skip(3); // Alignment
	pPlayer->_pSBkSpell = file->nextLE<Uint32>();
	pPlayer->_pSBkSplType = file->next<Uint8>();
	for (int i = 0; i < 64; i++)
		pPlayer->_pSplLvl[i] = file->next<Uint8>();
	file->skip(7); // Alignment
	pPlayer->_pMemSpells = file->nextLE<Uint64>();
	pPlayer->_pAblSpells = file->nextLE<Uint64>();
	pPlayer->_pScrlSpells = file->nextLE<Uint64>();
	pPlayer->_pSpellFlags = file->next<Uint8>();
	file->skip(3); // Alignment
	for (int i = 0; i < 4; i++)
		pPlayer->_pSplHotKey[i] = file->nextLE<Uint32>();
	for (int i = 0; i < 4; i++)
		pPlayer->_pSplTHotKey[i] = file->next<Uint8>();

	pPlayer->_pwtype = file->nextLE<Uint32>();
	pPlayer->_pBlockFlag = file->next<Uint8>();
	pPlayer->_pInvincible = file->next<Uint8>();
	pPlayer->_pLightRad = file->next<Uint8>();
	pPlayer->_pLvlChanging = file->next<Uint8>();

	file->nextBytes(pPlayer->_pName, PLR_NAME_LEN);
	pPlayer->_pClass = file->next<Uint8>();
	file->skip(3); // Alignment
	pPlayer->_pStrength = file->nextLE<Uint32>();
	pPlayer->_pBaseStr = file->nextLE<Uint32>();
	pPlayer->_pMagic = file->nextLE<Uint32>();
	pPlayer->_pBaseMag = file->nextLE<Uint32>();
	pPlayer->_pDexterity = file->nextLE<Uint32>();
	pPlayer->_pBaseDex = file->nextLE<Uint32>();
	pPlayer->_pVitality = file->nextLE<Uint32>();
	pPlayer->_pBaseVit = file->nextLE<Uint32>();
	pPlayer->_pStatPts = file->nextLE<Uint32>();
	pPlayer->_pDamageMod = file->nextLE<Uint32>();
	pPlayer->_pBaseToBlk = file->nextLE<Uint32>();
	if (pPlayer->_pBaseToBlk == 0) {
		pPlayer->_pBaseToBlk = ToBlkTbl[pPlayer->_pClass];
	}
	pPlayer->_pHPBase = file->nextLE<Uint32>();
	pPlayer->_pMaxHPBase = file->nextLE<Uint32>();
	pPlayer->_pHitPoints = file->nextLE<Uint32>();
	pPlayer->_pMaxHP = file->nextLE<Uint32>();
	pPlayer->_pHPPer = file->nextLE<Uint32>();
	pPlayer->_pManaBase = file->nextLE<Uint32>();
	pPlayer->_pMaxManaBase = file->nextLE<Uint32>();
	pPlayer->_pMana = file->nextLE<Uint32>();
	pPlayer->_pMaxMana = file->nextLE<Uint32>();
	pPlayer->_pManaPer = file->nextLE<Uint32>();
	pPlayer->_pLevel = file->next<Uint8>();
	pPlayer->_pMaxLvl = file->next<Uint8>();
	file->skip(2); // Alignment
	pPlayer->_pExperience = file->nextLE<Uint32>();
	pPlayer->_pMaxExp = file->nextLE<Uint32>();
	pPlayer->_pNextExper = file->nextLE<Uint32>();
	pPlayer->_pArmorClass = file->next<Uint8>();
	pPlayer->_pMagResist = file->next<Uint8>();
	pPlayer->_pFireResist = file->next<Uint8>();
	pPlayer->_pLghtResist = file->next<Uint8>();
	pPlayer->_pGold = file->nextLE<Uint32>();

	pPlayer->_pInfraFlag = file->nextLE<Uint32>();
	pPlayer->_pVar1 = file->nextLE<Uint32>();
	pPlayer->_pVar2 = file->nextLE<Uint32>();
	pPlayer->_pVar3 = file->nextLE<Uint32>();
	pPlayer->_pVar4 = file->nextLE<Uint32>();
	pPlayer->_pVar5 = file->nextLE<Uint32>();
	pPlayer->_pVar6 = file->nextLE<Uint32>();
	pPlayer->_pVar7 = file->nextLE<Uint32>();
	pPlayer->_pVar8 = file->nextLE<Uint32>();
	for (int i = 0; i < giNumberOfLevels; i++)
		pPlayer->_pLvlVisited[i] = file->nextBool8();
	for (int i = 0; i < giNumberOfLevels; i++)
		pPlayer->_pSLvlVisited[i] = file->nextBool8();

	file->skip(2); // Alignment

	pPlayer->_pGFXLoad = file->nextLE<Uint32>();
	file->skip(4 * 8); // Skip pointers _pNAnim
	pPlayer->_pNFrames = file->nextLE<Uint32>();
	pPlayer->_pNWidth = file->nextLE<Uint32>();
	file->skip(4 * 8); // Skip pointers _pWAnim
	pPlayer->_pWFrames = file->nextLE<Uint32>();
	pPlayer->_pWWidth = file->nextLE<Uint32>();
	file->skip(4 * 8); // Skip pointers _pAAnim
	pPlayer->_pAFrames = file->nextLE<Uint32>();
	pPlayer->_pAWidth = file->nextLE<Uint32>();
	pPlayer->_pAFNum = file->nextLE<Uint32>();
	file->skip(4 * 8); // Skip pointers _pLAnim
	file->skip(4 * 8); // Skip pointers _pFAnim
	file->skip(4 * 8); // Skip pointers _pTAnim
	pPlayer->_pSFrames = file->nextLE<Uint32>();
	pPlayer->_pSWidth = file->nextLE<Uint32>();
	pPlayer->_pSFNum = file->nextLE<Uint32>();
	file->skip(4 * 8); // Skip pointers _pHAnim
	pPlayer->_pHFrames = file->nextLE<Uint32>();
	pPlayer->_pHWidth = file->nextLE<Uint32>();
	file->skip(4 * 8); // Skip pointers _pDAnim
	pPlayer->_pDFrames = file->nextLE<Uint32>();
	pPlayer->_pDWidth = file->nextLE<Uint32>();
	file->skip(4 * 8); // Skip pointers _pBAnim
	pPlayer->_pBFrames = file->nextLE<Uint32>();
	pPlayer->_pBWidth = file->nextLE<Uint32>();

	LoadItems(file, NUM_INVLOC, pPlayer->InvBody);
	LoadItems(file, NUM_INV_GRID_ELEM, pPlayer->InvList);
	pPlayer->_pNumInv = file->nextLE<Uint32>();
	for (int i = 0; i < NUM_INV_GRID_ELEM; i++)
		pPlayer->InvGrid[i] = file->next<Uint8>();
	LoadItems(file, MAXBELTITEMS, pPlayer->SpdList);
	LoadItemData(file, &pPlayer->HoldItem);

	pPlayer->_pIMinDam = file->nextLE<Uint32>();
	pPlayer->_pIMaxDam = file->nextLE<Uint32>();
	pPlayer->_pIAC = file->nextLE<Uint32>();
	pPlayer->_pIBonusDam = file->nextLE<Uint32>();
	pPlayer->_pIBonusToHit = file->nextLE<Uint32>();
	pPlayer->_pIBonusAC = file->nextLE<Uint32>();
	pPlayer->_pIBonusDamMod = file->nextLE<Uint32>();
	file->skip(4); // Alignment

	pPlayer->_pISpells = file->nextLE<Uint64>();
	pPlayer->_pIFlags = file->nextLE<Uint32>();
	pPlayer->_pIGetHit = file->nextLE<Uint32>();
	pPlayer->_pISplLvlAdd = file->next<Uint8>();
	pPlayer->_pISplCost = file->next<Uint8>();
	file->skip(2); // Alignment
	pPlayer->_pISplDur = file->nextLE<Uint32>();
	pPlayer->_pIEnAc = file->nextLE<Uint32>();
	pPlayer->_pIFMinDam = file->nextLE<Uint32>();
	pPlayer->_pIFMaxDam = file->nextLE<Uint32>();
	pPlayer->_pILMinDam = file->nextLE<Uint32>();
	pPlayer->_pILMaxDam = file->nextLE<Uint32>();
	pPlayer->_pOilType = file->nextLE<Uint32>();
	pPlayer->pTownWarps = file->next<Uint8>();
	pPlayer->pDungMsgs = file->next<Uint8>();
	pPlayer->pLvlLoad = file->next<Uint8>();

	if (gbIsHellfireSaveGame) {
		pPlayer->pDungMsgs2 = file->next<Uint8>();
		pPlayer->pBattleNet = false;
	} else {
		pPlayer->pDungMsgs2 = 0;
		pPlayer->pBattleNet = file->next<Uint8>();
	}
	pPlayer->pManaShield = file->next<Uint8>();
	if (gbIsHellfireSaveGame) {
		pPlayer->pOriginalCathedral = file->next<Uint8>();
	} else {
		file->skip(1);
		pPlayer->pOriginalCathedral = true;
	}
	file->skip(2); // Available bytes
	pPlayer->wReflections = file->nextLE<Uint16>();
	file->skip(14); // Available bytes

	pPlayer->pDiabloKillLevel = file->nextLE<Uint32>();
	pPlayer->pDifficulty = file->nextLE<Uint32>();
	pPlayer->pDamAcFlags = file->nextLE<Uint32>();
	file->skip(20); // Available bytes
	CalcPlrItemVals(p, FALSE);

	// Omit pointer _pNData
	// Omit pointer _pWData
	// Omit pointer _pAData
	// Omit pointer _pLData
	// Omit pointer _pFData
	// Omit pointer  _pTData
	// Omit pointer _pHData
	// Omit pointer _pDData
	// Omit pointer _pBData
	// Omit pointer pReserved
}

bool gbSkipSync = false;

static void LoadMonster(LoadHelper *file, int i)
{
	MonsterStruct *pMonster = &monster[i];

	pMonster->_mMTidx = file->nextLE<Uint32>();
	pMonster->_mmode = file->nextLE<Uint32>();
	pMonster->_mgoal = file->next<Uint8>();
	file->skip(3); // Alignment
	pMonster->_mgoalvar1 = file->nextLE<Uint32>();
	pMonster->_mgoalvar2 = file->nextLE<Uint32>();
	pMonster->_mgoalvar3 = file->nextLE<Uint32>();
	file->skip(4); // Unused
	pMonster->_pathcount = file->next<Uint8>();
	file->skip(3); // Alignment
	pMonster->_mx = file->nextLE<Uint32>();
	pMonster->_my = file->nextLE<Uint32>();
	pMonster->_mfutx = file->nextLE<Uint32>();
	pMonster->_mfuty = file->nextLE<Uint32>();
	pMonster->_moldx = file->nextLE<Uint32>();
	pMonster->_moldy = file->nextLE<Uint32>();
	pMonster->_mxoff = file->nextLE<Uint32>();
	pMonster->_myoff = file->nextLE<Uint32>();
	pMonster->_mxvel = file->nextLE<Uint32>();
	pMonster->_myvel = file->nextLE<Uint32>();
	pMonster->_mdir = file->nextLE<Uint32>();
	pMonster->_menemy = file->nextLE<Uint32>();
	pMonster->_menemyx = file->next<Uint8>();
	pMonster->_menemyy = file->next<Uint8>();
	file->skip(2); // Unused

	file->skip(4); // Skip pointer _mAnimData
	pMonster->_mAnimDelay = file->nextLE<Uint32>();
	pMonster->_mAnimCnt = file->nextLE<Uint32>();
	pMonster->_mAnimLen = file->nextLE<Uint32>();
	pMonster->_mAnimFrame = file->nextLE<Uint32>();
	file->skip(4); // Skip _meflag
	pMonster->_mDelFlag = file->nextLE<Uint32>();
	pMonster->_mVar1 = file->nextLE<Uint32>();
	pMonster->_mVar2 = file->nextLE<Uint32>();
	pMonster->_mVar3 = file->nextLE<Uint32>();
	pMonster->_mVar4 = file->nextLE<Uint32>();
	pMonster->_mVar5 = file->nextLE<Uint32>();
	pMonster->_mVar6 = file->nextLE<Uint32>();
	pMonster->_mVar7 = file->nextLE<Uint32>();
	pMonster->_mVar8 = file->nextLE<Uint32>();
	pMonster->_mmaxhp = file->nextLE<Uint32>();
	pMonster->_mhitpoints = file->nextLE<Uint32>();

	pMonster->_mAi = file->next<Uint8>();
	pMonster->_mint = file->next<Uint8>();
	file->skip(2); // Alignment
	pMonster->_mFlags = file->nextLE<Uint32>();
	pMonster->_msquelch = file->next<Uint8>();
	file->skip(3); // Alignment
	file->skip(4); // Unused
	pMonster->_lastx = file->nextLE<Uint32>();
	pMonster->_lasty = file->nextLE<Uint32>();
	pMonster->_mRndSeed = file->nextLE<Uint32>();
	pMonster->_mAISeed = file->nextLE<Uint32>();
	file->skip(4); // Unused

	pMonster->_uniqtype = file->next<Uint8>();
	pMonster->_uniqtrans = file->next<Uint8>();
	pMonster->_udeadval = file->next<Uint8>();

	pMonster->mWhoHit = file->next<Uint8>();
	pMonster->mLevel = file->next<Uint8>();
	file->skip(1); // Alignment
	pMonster->mExp = file->nextLE<Uint16>();

	file->skip(1); // Skip mHit as it's already initialized
	pMonster->mMinDamage = file->next<Uint8>();
	pMonster->mMaxDamage = file->next<Uint8>();
	file->skip(1); // Skip mHit2 as it's already initialized
	pMonster->mMinDamage2 = file->next<Uint8>();
	pMonster->mMaxDamage2 = file->next<Uint8>();
	pMonster->mArmorClass = file->next<Uint8>();
	file->skip(1); // Alignment
	pMonster->mMagicRes = file->nextLE<Uint16>();
	file->skip(2); // Alignment

	pMonster->mtalkmsg = file->nextLE<Uint32>();
	pMonster->leader = file->next<Uint8>();
	pMonster->leaderflag = file->next<Uint8>();
	pMonster->packsize = file->next<Uint8>();
	pMonster->mlid = file->next<Uint8>();
	if (pMonster->mlid == plr[myplr]._plid)
		pMonster->mlid = NO_LIGHT; // Correct incorect values in old saves

	// Omit pointer mName;
	// Omit pointer MType;
	// Omit pointer MData;

	if (gbSkipSync)
		return;

	SyncMonsterAnim(i);
}

static void LoadMissile(LoadHelper *file, int i)
{
	MissileStruct *pMissile = &missile[i];

	pMissile->_mitype = file->nextLE<Uint32>();
	pMissile->_mix = file->nextLE<Uint32>();
	pMissile->_miy = file->nextLE<Uint32>();
	pMissile->_mixoff = file->nextLE<Uint32>();
	pMissile->_miyoff = file->nextLE<Uint32>();
	pMissile->_mixvel = file->nextLE<Uint32>();
	pMissile->_miyvel = file->nextLE<Uint32>();
	pMissile->_misx = file->nextLE<Uint32>();
	pMissile->_misy = file->nextLE<Uint32>();
	pMissile->_mitxoff = file->nextLE<Uint32>();
	pMissile->_mityoff = file->nextLE<Uint32>();
	pMissile->_mimfnum = file->nextLE<Uint32>();
	pMissile->_mispllvl = file->nextLE<Uint32>();
	pMissile->_miDelFlag = file->nextBool32();
	pMissile->_miAnimType = file->next<Uint8>();
	file->skip(3); // Alignment
	pMissile->_miAnimFlags = file->nextLE<Uint32>();
	file->skip(4); // Skip pointer _miAnimData
	pMissile->_miAnimDelay = file->nextLE<Uint32>();
	pMissile->_miAnimLen = file->nextLE<Uint32>();
	pMissile->_miAnimWidth = file->nextLE<Uint32>();
	pMissile->_miAnimWidth2 = file->nextLE<Uint32>();
	pMissile->_miAnimCnt = file->nextLE<Uint32>();
	pMissile->_miAnimAdd = file->nextLE<Uint32>();
	pMissile->_miAnimFrame = file->nextLE<Uint32>();
	pMissile->_miDrawFlag = file->nextBool32();
	pMissile->_miLightFlag = file->nextBool32();
	pMissile->_miPreFlag = file->nextBool32();
	pMissile->_miUniqTrans = file->nextLE<Uint32>();
	pMissile->_mirange = file->nextLE<Uint32>();
	pMissile->_misource = file->nextLE<Uint32>();
	pMissile->_micaster = file->nextLE<Uint32>();
	pMissile->_midam = file->nextLE<Uint32>();
	pMissile->_miHitFlag = file->nextBool32();
	pMissile->_midist = file->nextLE<Uint32>();
	pMissile->_mlid = file->nextLE<Uint32>();
	pMissile->_mirnd = file->nextLE<Uint32>();
	pMissile->_miVar1 = file->nextLE<Uint32>();
	pMissile->_miVar2 = file->nextLE<Uint32>();
	pMissile->_miVar3 = file->nextLE<Uint32>();
	pMissile->_miVar4 = file->nextLE<Uint32>();
	pMissile->_miVar5 = file->nextLE<Uint32>();
	pMissile->_miVar6 = file->nextLE<Uint32>();
	pMissile->_miVar7 = file->nextLE<Uint32>();
	pMissile->_miVar8 = file->nextLE<Uint32>();
}

static void LoadObject(LoadHelper *file, int i)
{
	ObjectStruct *pObject = &object[i];

	pObject->_otype = file->nextLE<Uint32>();
	pObject->_ox = file->nextLE<Uint32>();
	pObject->_oy = file->nextLE<Uint32>();
	pObject->_oLight = file->nextLE<Uint32>();
	pObject->_oAnimFlag = file->nextLE<Uint32>();
	file->skip(4); // Skip pointer _oAnimData
	pObject->_oAnimDelay = file->nextLE<Uint32>();
	pObject->_oAnimCnt = file->nextLE<Uint32>();
	pObject->_oAnimLen = file->nextLE<Uint32>();
	pObject->_oAnimFrame = file->nextLE<Uint32>();
	pObject->_oAnimWidth = file->nextLE<Uint32>();
	pObject->_oAnimWidth2 = file->nextLE<Uint32>();
	pObject->_oDelFlag = file->nextLE<Uint32>();
	pObject->_oBreak = file->next<Uint8>();
	file->skip(3); // Alignment
	pObject->_oSolidFlag = file->nextLE<Uint32>();
	pObject->_oMissFlag = file->nextLE<Uint32>();

	pObject->_oSelFlag = file->next<Uint8>();
	file->skip(3); // Alignment
	pObject->_oPreFlag = file->nextLE<Uint32>();
	pObject->_oTrapFlag = file->nextLE<Uint32>();
	pObject->_oDoorFlag = file->nextLE<Uint32>();
	pObject->_olid = file->nextLE<Uint32>();
	pObject->_oRndSeed = file->nextLE<Uint32>();
	pObject->_oVar1 = file->nextLE<Uint32>();
	pObject->_oVar2 = file->nextLE<Uint32>();
	pObject->_oVar3 = file->nextLE<Uint32>();
	pObject->_oVar4 = file->nextLE<Uint32>();
	pObject->_oVar5 = file->nextLE<Uint32>();
	pObject->_oVar6 = file->nextLE<Uint32>();
	pObject->_oVar7 = file->nextLE<Uint32>();
	pObject->_oVar8 = file->nextLE<Uint32>();
}

static void LoadItem(LoadHelper *file, int i)
{
	LoadItemData(file, &item[i]);
	GetItemFrm(i);
}

static void LoadPremium(LoadHelper *file, int i)
{
	LoadItemData(file, &premiumitem[i]);
}

static void LoadQuest(LoadHelper *file, int i)
{
	QuestStruct *pQuest = &quests[i];

	pQuest->_qlevel = file->next<Uint8>();
	pQuest->_qtype = file->next<Uint8>();
	pQuest->_qactive = file->next<Uint8>();
	pQuest->_qlvltype = file->next<Uint8>();
	pQuest->_qtx = file->nextLE<Uint32>();
	pQuest->_qty = file->nextLE<Uint32>();
	pQuest->_qslvl = file->next<Uint8>();
	pQuest->_qidx = file->next<Uint8>();
	if (gbIsHellfireSaveGame) {
		file->skip(2); // Alignment
		pQuest->_qmsg = file->nextLE<Uint32>();
	} else {
		pQuest->_qmsg = file->next<Uint8>();
	}
	pQuest->_qvar1 = file->next<Uint8>();
	pQuest->_qvar2 = file->next<Uint8>();
	file->skip(2); // Alignment
	if (!gbIsHellfireSaveGame)
		file->skip(1); // Alignment
	pQuest->_qlog = file->nextBool32();

	ReturnLvlX = file->nextBE<Uint32>();
	ReturnLvlY = file->nextBE<Uint32>();
	ReturnLvl = file->nextBE<Uint32>();
	ReturnLvlT = file->nextBE<Uint32>();
	DoomQuestState = file->nextBE<Uint32>();
}

static void LoadLighting(LoadHelper *file, int i)
{
	LightListStruct *pLight = &LightList[i];

	pLight->_lx = file->nextLE<Uint32>();
	pLight->_ly = file->nextLE<Uint32>();
	pLight->_lradius = file->nextLE<Uint32>();
	pLight->_lid = file->nextLE<Uint32>();
	pLight->_ldel = file->nextLE<Uint32>();
	pLight->_lunflag = file->nextLE<Uint32>();
	file->skip(4); // Unused
	pLight->_lunx = file->nextLE<Uint32>();
	pLight->_luny = file->nextLE<Uint32>();
	pLight->_lunr = file->nextLE<Uint32>();
	pLight->_xoff = file->nextLE<Uint32>();
	pLight->_yoff = file->nextLE<Uint32>();
	pLight->_lflags = file->nextLE<Uint32>();
}

static void LoadVision(LoadHelper *file, int i)
{
	LightListStruct *pVision = &VisionList[i];

	pVision->_lx = file->nextLE<Uint32>();
	pVision->_ly = file->nextLE<Uint32>();
	pVision->_lradius = file->nextLE<Uint32>();
	pVision->_lid = file->nextLE<Uint32>();
	pVision->_ldel = file->nextLE<Uint32>();
	pVision->_lunflag = file->nextLE<Uint32>();
	file->skip(4); // Unused
	pVision->_lunx = file->nextLE<Uint32>();
	pVision->_luny = file->nextLE<Uint32>();
	pVision->_lunr = file->nextLE<Uint32>();
	pVision->_xoff = file->nextLE<Uint32>();
	pVision->_yoff = file->nextLE<Uint32>();
	pVision->_lflags = file->nextLE<Uint32>();
}

static void LoadPortal(LoadHelper *file, int i)
{
	PortalStruct *pPortal = &portal[i];

	pPortal->open = file->nextLE<Uint32>();
	pPortal->x = file->nextLE<Uint32>();
	pPortal->y = file->nextLE<Uint32>();
	pPortal->level = file->nextLE<Uint32>();
	pPortal->ltype = file->nextLE<Uint32>();
	pPortal->setlvl = file->nextLE<Uint32>();
}

int RemapItemIdxFromDiablo(int i)
{
	if (i == IDI_SORCEROR) {
		return 166;
	}
	if (i >= 156) {
		i += 5; // Hellfire exclusive items
	}
	if (i >= 88) {
		i += 1; // Scroll of Search
	}
	if (i >= 83) {
		i += 4; // Oils
	}

	return i;
}

int RemapItemIdxToDiablo(int i)
{
	if (i == 166) {
		return IDI_SORCEROR;
	}
	if ((i >= 83 && i <= 86) || i == 92 || i >= 161) {
		return -1; // Hellfire exclusive items
	}
	if (i >= 93) {
		i -= 1; // Scroll of Search
	}
	if (i >= 87) {
		i -= 4; // Oils
	}

	return i;
}

bool IsHeaderValid(Uint32 magicNumber)
{
	gbIsHellfireSaveGame = false;
	if (magicNumber == LOAD_LE32("SHAR")) {
		return true;
	} else if (magicNumber == LOAD_LE32("SHLF")) {
		gbIsHellfireSaveGame = true;
		return true;
	} else if (!gbIsSpawn && magicNumber == LOAD_LE32("RETL")) {
		return true;
	} else if (!gbIsSpawn && magicNumber == LOAD_LE32("HELF")) {
		gbIsHellfireSaveGame = true;
		return true;
	}

	return false;
}

void ConvertLevels()
{
	// Backup current level state
	bool _setlevel = setlevel;
	int _setlvlnum = setlvlnum;
	int _currlevel = currlevel;
	int _leveltype = leveltype;

	gbSkipSync = true;

	setlevel = false; // Convert regular levels
	for (int i = 0; i < giNumberOfLevels; i++) {
		currlevel = i;
		if (!LevelFileExists())
			continue;

		leveltype = gnLevelTypeTbl[i];

		LoadLevel();
		SaveLevel();
	}

	setlevel = true; // Convert quest levels
	for (int i = 0; i < MAXQUESTS; i++) {
		if (quests[i]._qactive == QUEST_NOTAVAIL) {
			continue;
		}

		leveltype = quests[i]._qlvltype;
		if (leveltype == DTYPE_NONE) {
			continue;
		}

		setlvlnum = quests[i]._qslvl;
		if (!LevelFileExists())
			continue;

		LoadLevel();
		SaveLevel();
	}

	gbSkipSync = false;

	// Restor current level state
	setlevel = _setlevel;
	setlvlnum = _setlvlnum;
	currlevel = _currlevel;
	leveltype = _leveltype;
}

void LoadHotkeys()
{
	LoadHelper file("hotkeys");
	if (!file.isValid())
		return;

	const size_t nHotkeyTypes = sizeof(plr[myplr]._pSplHotKey) / sizeof(plr[myplr]._pSplHotKey[0]);
	const size_t nHotkeySpells = sizeof(plr[myplr]._pSplTHotKey) / sizeof(plr[myplr]._pSplTHotKey[0]);

	for (size_t i = 0; i < nHotkeyTypes; i++) {
		plr[myplr]._pSplHotKey[i] = file.nextLE<Uint32>();
	}
	for (size_t i = 0; i < nHotkeySpells; i++) {
		plr[myplr]._pSplTHotKey[i] = file.next<Uint8>();
	}
	plr[myplr]._pRSpell = file.nextLE<Uint32>();
	plr[myplr]._pRSplType = file.next<Uint8>();
}

void SaveHotkeys()
{
	const size_t nHotkeyTypes = sizeof(plr[myplr]._pSplHotKey) / sizeof(plr[myplr]._pSplHotKey[0]);
	const size_t nHotkeySpells = sizeof(plr[myplr]._pSplTHotKey) / sizeof(plr[myplr]._pSplTHotKey[0]);

	SaveHelper file("hotkeys", (nHotkeyTypes * 4) + nHotkeySpells + 4 + 1);

	for (size_t i = 0; i < nHotkeyTypes; i++) {
		file.writeLE<Uint32>(plr[myplr]._pSplHotKey[i]);
	}
	for (size_t i = 0; i < nHotkeySpells; i++) {
		file.writeByte(plr[myplr]._pSplTHotKey[i]);
	}
	file.writeLE<Uint32>(plr[myplr]._pRSpell);
	file.writeByte(plr[myplr]._pRSplType);
}

/**
 * @brief Load game state
 * @param firstflag Can be set to false if we are simply reloading the current game
 */
void LoadGame(BOOL firstflag)
{
	FreeGameMem();
	pfile_remove_temp_files();

	LoadHelper file("game");
	if (!file.isValid())
		app_fatal("Unable to open save file archive");

	if (!IsHeaderValid(file.nextLE<Uint32>()))
		app_fatal("Invalid save file");

	if (gbIsHellfireSaveGame) {
		giNumberOfLevels = 25;
		giNumberQuests = 24;
		giNumberOfSmithPremiumItems = 15;
	} else {
		// Todo initialize additional levels and quests if we are running Hellfire
		giNumberOfLevels = 17;
		giNumberQuests = 16;
		giNumberOfSmithPremiumItems = 6;
	}

	setlevel = file.nextBool8();
	setlvlnum = file.nextBE<Uint32>();
	currlevel = file.nextBE<Uint32>();
	leveltype = file.nextBE<Uint32>();
	if (!setlevel)
		leveltype = gnLevelTypeTbl[currlevel];
	int _ViewX = file.nextBE<Uint32>();
	int _ViewY = file.nextBE<Uint32>();
	invflag = file.nextBool8();
	chrflag = file.nextBool8();
	int _nummonsters = file.nextBE<Uint32>();
	int _numitems = file.nextBE<Uint32>();
	int _nummissiles = file.nextBE<Uint32>();
	int _nobjects = file.nextBE<Uint32>();

	if (!gbIsHellfire && currlevel > 17)
		app_fatal("Player is on a Hellfire only level");

	for (int i = 0; i < giNumberOfLevels; i++) {
		glSeedTbl[i] = file.nextBE<Uint32>();
		file.skip(4); // Skip loading gnLevelTypeTbl
	}

	LoadPlayer(&file, myplr);

	gnDifficulty = plr[myplr].pDifficulty;
	if (gnDifficulty < DIFF_NORMAL || gnDifficulty > DIFF_HELL)
		gnDifficulty = DIFF_NORMAL;

	for (int i = 0; i < giNumberQuests; i++)
		LoadQuest(&file, i);
	for (int i = 0; i < MAXPORTAL; i++)
		LoadPortal(&file, i);

	if (gbIsHellfireSaveGame != gbIsHellfire)
		ConvertLevels();

	LoadGameLevel(firstflag, ENTRY_LOAD);
	SyncInitPlr(myplr);
	SyncPlrAnim(myplr);

	ViewX = _ViewX;
	ViewY = _ViewY;
	nummonsters = _nummonsters;
	numitems = _numitems;
	nummissiles = _nummissiles;
	nobjects = _nobjects;

	for (int i = 0; i < MAXMONSTERS; i++)
		monstkills[i] = file.nextBE<Uint32>();

	if (leveltype != DTYPE_TOWN) {
		for (int i = 0; i < MAXMONSTERS; i++)
			monstactive[i] = file.nextBE<Uint32>();
		for (int i = 0; i < nummonsters; i++)
			LoadMonster(&file, monstactive[i]);
		for (int i = 0; i < MAXMISSILES; i++)
			missileactive[i] = file.next<Uint8>();
		for (int i = 0; i < MAXMISSILES; i++)
			missileavail[i] = file.next<Uint8>();
		for (int i = 0; i < nummissiles; i++)
			LoadMissile(&file, missileactive[i]);
		for (int i = 0; i < MAXOBJECTS; i++)
			objectactive[i] = file.next<Uint8>();
		for (int i = 0; i < MAXOBJECTS; i++)
			objectavail[i] = file.next<Uint8>();
		for (int i = 0; i < nobjects; i++)
			LoadObject(&file, objectactive[i]);
		for (int i = 0; i < nobjects; i++)
			SyncObjectAnim(objectactive[i]);

		numlights = file.nextBE<Uint32>();

		for (int i = 0; i < MAXLIGHTS; i++)
			lightactive[i] = file.next<Uint8>();
		for (int i = 0; i < numlights; i++)
			LoadLighting(&file, lightactive[i]);

		visionid = file.nextBE<Uint32>();
		numvision = file.nextBE<Uint32>();

		for (int i = 0; i < numvision; i++)
			LoadVision(&file, i);
	}

	for (int i = 0; i < MAXITEMS; i++)
		itemactive[i] = file.next<Uint8>();
	for (int i = 0; i < MAXITEMS; i++)
		itemavail[i] = file.next<Uint8>();
	for (int i = 0; i < numitems; i++)
		LoadItem(&file, itemactive[i]);
	for (int i = 0; i < 128; i++)
		UniqueItemFlag[i] = file.nextBool8();

	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			dLight[i][j] = file.next<Uint8>();
	}
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			dFlags[i][j] = file.next<Uint8>();
	}
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			dPlayer[i][j] = file.next<Uint8>();
	}
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			dItem[i][j] = file.next<Uint8>();
	}

	if (leveltype != DTYPE_TOWN) {
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dMonster[i][j] = file.nextBE<Uint32>();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dDead[i][j] = file.next<Uint8>();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dObject[i][j] = file.next<Uint8>();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dLight[i][j] = file.next<Uint8>();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dPreLight[i][j] = file.next<Uint8>();
		}
		for (int j = 0; j < DMAXY; j++) {
			for (int i = 0; i < DMAXX; i++)
				automapview[i][j] = file.nextBool8();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dMissile[i][j] = file.next<Uint8>();
		}
	}

	numpremium = file.nextBE<Uint32>();
	premiumlevel = file.nextBE<Uint32>();

	for (int i = 0; i < giNumberOfSmithPremiumItems; i++)
		LoadPremium(&file, i);
	if (gbIsHellfire && !gbIsHellfireSaveGame)
		SpawnPremium(myplr);

	automapflag = file.nextBool8();
	AutoMapScale = file.nextBE<Uint32>();
	AutomapZoomReset();
	ResyncQuests();

	if (leveltype != DTYPE_TOWN)
		ProcessLightList();

	RedoPlayerVision();
	ProcessVisionList();
	missiles_process_charge();
	ResetPal();
	SetCursor_(CURSOR_HAND);
	gbProcessPlayers = TRUE;

	if (gbIsHellfireSaveGame != gbIsHellfire)
		SaveGame();

	gbIsHellfireSaveGame = gbIsHellfire;
}

static void SaveItem(SaveHelper *file, ItemStruct *pItem)
{
	int idx = pItem->IDidx;
	if (!gbIsHellfire)
		idx = RemapItemIdxToDiablo(idx);
	int iType = pItem->_itype;
	if (idx == -1) {
		idx = 0;
		iType = ITYPE_NONE;
	}

	file->writeLE<Uint32>(pItem->_iSeed);
	file->writeLE<Uint16>(pItem->_iCreateInfo);
	file->skip(2); // Alignment
	file->writeLE<Uint32>(iType);
	file->writeLE<Uint32>(pItem->_ix);
	file->writeLE<Uint32>(pItem->_iy);
	file->writeLE<Uint32>(pItem->_iAnimFlag);
	file->skip(4); // Skip pointer _iAnimData
	file->writeLE<Uint32>(pItem->_iAnimLen);
	file->writeLE<Uint32>(pItem->_iAnimFrame);
	file->writeLE<Uint32>(pItem->_iAnimWidth);
	file->writeLE<Uint32>(pItem->_iAnimWidth2);
	file->skip(4); // Unused since 1.02
	file->writeByte(pItem->_iSelFlag);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pItem->_iPostDraw);
	file->writeLE<Uint32>(pItem->_iIdentified);
	file->writeByte(pItem->_iMagical);
	file->writeBytes(pItem->_iName, 64);
	file->writeBytes(pItem->_iIName, 64);
	file->writeByte(pItem->_iLoc);
	file->writeByte(pItem->_iClass);
	file->skip(1); // Alignment
	file->writeLE<Uint32>(pItem->_iCurs);
	file->writeLE<Uint32>(pItem->_ivalue);
	file->writeLE<Uint32>(pItem->_iIvalue);
	file->writeLE<Uint32>(pItem->_iMinDam);
	file->writeLE<Uint32>(pItem->_iMaxDam);
	file->writeLE<Uint32>(pItem->_iAC);
	file->writeLE<Uint32>(pItem->_iFlags);
	file->writeLE<Uint32>(pItem->_iMiscId);
	file->writeLE<Uint32>(pItem->_iSpell);
	file->writeLE<Uint32>(pItem->_iCharges);
	file->writeLE<Uint32>(pItem->_iMaxCharges);
	file->writeLE<Uint32>(pItem->_iDurability);
	file->writeLE<Uint32>(pItem->_iMaxDur);
	file->writeLE<Uint32>(pItem->_iPLDam);
	file->writeLE<Uint32>(pItem->_iPLToHit);
	file->writeLE<Uint32>(pItem->_iPLAC);
	file->writeLE<Uint32>(pItem->_iPLStr);
	file->writeLE<Uint32>(pItem->_iPLMag);
	file->writeLE<Uint32>(pItem->_iPLDex);
	file->writeLE<Uint32>(pItem->_iPLVit);
	file->writeLE<Uint32>(pItem->_iPLFR);
	file->writeLE<Uint32>(pItem->_iPLLR);
	file->writeLE<Uint32>(pItem->_iPLMR);
	file->writeLE<Uint32>(pItem->_iPLMana);
	file->writeLE<Uint32>(pItem->_iPLHP);
	file->writeLE<Uint32>(pItem->_iPLDamMod);
	file->writeLE<Uint32>(pItem->_iPLGetHit);
	file->writeLE<Uint32>(pItem->_iPLLight);
	file->writeByte(pItem->_iSplLvlAdd);
	file->writeByte(pItem->_iRequest);
	file->skip(2); // Alignment
	file->writeLE<Uint32>(pItem->_iUid);
	file->writeLE<Uint32>(pItem->_iFMinDam);
	file->writeLE<Uint32>(pItem->_iFMaxDam);
	file->writeLE<Uint32>(pItem->_iLMinDam);
	file->writeLE<Uint32>(pItem->_iLMaxDam);
	file->writeLE<Uint32>(pItem->_iPLEnAc);
	file->writeByte(pItem->_iPrePower);
	file->writeByte(pItem->_iSufPower);
	file->skip(2); // Alignment
	file->writeLE<Uint32>(pItem->_iVAdd1);
	file->writeLE<Uint32>(pItem->_iVMult1);
	file->writeLE<Uint32>(pItem->_iVAdd2);
	file->writeLE<Uint32>(pItem->_iVMult2);
	file->writeByte(pItem->_iMinStr);
	file->writeByte(pItem->_iMinMag);
	file->writeByte(pItem->_iMinDex);
	file->skip(1); // Alignment
	file->writeLE<Uint32>(pItem->_iStatFlag);
	file->writeLE<Uint32>(idx);
	file->skip(4); // Unused
	if (gbIsHellfire)
		file->writeLE<Uint32>(pItem->_iDamAcFlags);
}

static void SaveItems(SaveHelper *file, ItemStruct *pItem, const int n)
{
	for (int i = 0; i < n; i++) {
		SaveItem(file, &pItem[i]);
	}
}

static void SavePlayer(SaveHelper *file, int p)
{
	PlayerStruct *pPlayer = &plr[p];

	file->writeLE<Uint32>(pPlayer->_pmode);
	for (int i = 0; i < MAX_PATH_LENGTH; i++)
		file->writeByte(pPlayer->walkpath[i]);
	file->writeByte(pPlayer->plractive);
	file->skip(2); // Alignment
	file->writeLE<Uint32>(pPlayer->destAction);
	file->writeLE<Uint32>(pPlayer->destParam1);
	file->writeLE<Uint32>(pPlayer->destParam2);
	file->writeLE<Uint32>(pPlayer->destParam3);
	file->writeLE<Uint32>(pPlayer->destParam4);
	file->writeLE<Uint32>(pPlayer->plrlevel);
	file->writeLE<Uint32>(pPlayer->_px);
	file->writeLE<Uint32>(pPlayer->_py);
	file->writeLE<Uint32>(pPlayer->_pfutx);
	file->writeLE<Uint32>(pPlayer->_pfuty);
	file->writeLE<Uint32>(pPlayer->_ptargx);
	file->writeLE<Uint32>(pPlayer->_ptargy);
	file->writeLE<Uint32>(pPlayer->_pownerx);
	file->writeLE<Uint32>(pPlayer->_pownery);
	file->writeLE<Uint32>(pPlayer->_poldx);
	file->writeLE<Uint32>(pPlayer->_poldy);
	file->writeLE<Uint32>(pPlayer->_pxoff);
	file->writeLE<Uint32>(pPlayer->_pyoff);
	file->writeLE<Uint32>(pPlayer->_pxvel);
	file->writeLE<Uint32>(pPlayer->_pyvel);
	file->writeLE<Uint32>(pPlayer->_pdir);
	file->skip(4); // Unused
	file->writeLE<Uint32>(pPlayer->_pgfxnum);
	file->skip(4); // Skip pointer _pAnimData
	file->writeLE<Uint32>(pPlayer->_pAnimDelay);
	file->writeLE<Uint32>(pPlayer->_pAnimCnt);
	file->writeLE<Uint32>(pPlayer->_pAnimLen);
	file->writeLE<Uint32>(pPlayer->_pAnimFrame);
	file->writeLE<Uint32>(pPlayer->_pAnimWidth);
	file->writeLE<Uint32>(pPlayer->_pAnimWidth2);
	file->skip(4); // Skip _peflag
	file->writeLE<Uint32>(pPlayer->_plid);
	file->writeLE<Uint32>(pPlayer->_pvid);

	file->writeLE<Uint32>(pPlayer->_pSpell);
	file->writeByte(pPlayer->_pSplType);
	file->writeByte(pPlayer->_pSplFrom);
	file->skip(2); // Alignment
	file->writeLE<Uint32>(pPlayer->_pTSpell);
	file->writeByte(pPlayer->_pTSplType);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pPlayer->_pRSpell);
	file->writeByte(pPlayer->_pRSplType);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pPlayer->_pSBkSpell);
	file->writeByte(pPlayer->_pSBkSplType);
	for (int i = 0; i < 64; i++)
		file->writeByte(pPlayer->_pSplLvl[i]);
	file->skip(7); // Alignment
	file->writeLE<Uint64>(pPlayer->_pMemSpells);
	file->writeLE<Uint64>(pPlayer->_pAblSpells);
	file->writeLE<Uint64>(pPlayer->_pScrlSpells);
	file->writeByte(pPlayer->_pSpellFlags);
	file->skip(3); // Alignment
	for (int i = 0; i < 4; i++)
		file->writeLE<Uint32>(pPlayer->_pSplHotKey[i]);
	for (int i = 0; i < 4; i++)
		file->writeByte(pPlayer->_pSplTHotKey[i]);

	file->writeLE<Uint32>(pPlayer->_pwtype);
	file->writeByte(pPlayer->_pBlockFlag);
	file->writeByte(pPlayer->_pInvincible);
	file->writeByte(pPlayer->_pLightRad);
	file->writeByte(pPlayer->_pLvlChanging);

	file->writeBytes(pPlayer->_pName, PLR_NAME_LEN);
	file->writeByte(pPlayer->_pClass);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pPlayer->_pStrength);
	file->writeLE<Uint32>(pPlayer->_pBaseStr);
	file->writeLE<Uint32>(pPlayer->_pMagic);
	file->writeLE<Uint32>(pPlayer->_pBaseMag);
	file->writeLE<Uint32>(pPlayer->_pDexterity);
	file->writeLE<Uint32>(pPlayer->_pBaseDex);
	file->writeLE<Uint32>(pPlayer->_pVitality);
	file->writeLE<Uint32>(pPlayer->_pBaseVit);
	file->writeLE<Uint32>(pPlayer->_pStatPts);
	file->writeLE<Uint32>(pPlayer->_pDamageMod);
	file->writeLE<Uint32>(pPlayer->_pBaseToBlk);
	file->writeLE<Uint32>(pPlayer->_pHPBase);
	file->writeLE<Uint32>(pPlayer->_pMaxHPBase);
	file->writeLE<Uint32>(pPlayer->_pHitPoints);
	file->writeLE<Uint32>(pPlayer->_pMaxHP);
	file->writeLE<Uint32>(pPlayer->_pHPPer);
	file->writeLE<Uint32>(pPlayer->_pManaBase);
	file->writeLE<Uint32>(pPlayer->_pMaxManaBase);
	file->writeLE<Uint32>(pPlayer->_pMana);
	file->writeLE<Uint32>(pPlayer->_pMaxMana);
	file->writeLE<Uint32>(pPlayer->_pManaPer);
	file->writeByte(pPlayer->_pLevel);
	file->writeByte(pPlayer->_pMaxLvl);
	file->skip(2); // Alignment
	file->writeLE<Uint32>(pPlayer->_pExperience);
	file->writeLE<Uint32>(pPlayer->_pMaxExp);
	file->writeLE<Uint32>(pPlayer->_pNextExper);
	file->writeByte(pPlayer->_pArmorClass);
	file->writeByte(pPlayer->_pMagResist);
	file->writeByte(pPlayer->_pFireResist);
	file->writeByte(pPlayer->_pLghtResist);
	file->writeLE<Uint32>(pPlayer->_pGold);

	file->writeLE<Uint32>(pPlayer->_pInfraFlag);
	file->writeLE<Uint32>(pPlayer->_pVar1);
	file->writeLE<Uint32>(pPlayer->_pVar2);
	file->writeLE<Uint32>(pPlayer->_pVar3);
	file->writeLE<Uint32>(pPlayer->_pVar4);
	file->writeLE<Uint32>(pPlayer->_pVar5);
	file->writeLE<Uint32>(pPlayer->_pVar6);
	file->writeLE<Uint32>(pPlayer->_pVar7);
	file->writeLE<Uint32>(pPlayer->_pVar8);
	for (int i = 0; i < giNumberOfLevels; i++)
		file->writeByte(pPlayer->_pLvlVisited[i]);
	for (int i = 0; i < giNumberOfLevels; i++)
		file->writeByte(pPlayer->_pSLvlVisited[i]); // only 10 used

	file->skip(2); // Alignment

	file->writeLE<Uint32>(pPlayer->_pGFXLoad);
	file->skip(4 * 8); // Skip pointers _pNAnim
	file->writeLE<Uint32>(pPlayer->_pNFrames);
	file->writeLE<Uint32>(pPlayer->_pNWidth);
	file->skip(4 * 8); // Skip pointers _pWAnim
	file->writeLE<Uint32>(pPlayer->_pWFrames);
	file->writeLE<Uint32>(pPlayer->_pWWidth);
	file->skip(4 * 8); // Skip pointers _pAAnim
	file->writeLE<Uint32>(pPlayer->_pAFrames);
	file->writeLE<Uint32>(pPlayer->_pAWidth);
	file->writeLE<Uint32>(pPlayer->_pAFNum);
	file->skip(4 * 8); // Skip pointers _pLAnim
	file->skip(4 * 8); // Skip pointers _pFAnim
	file->skip(4 * 8); // Skip pointers _pTAnim
	file->writeLE<Uint32>(pPlayer->_pSFrames);
	file->writeLE<Uint32>(pPlayer->_pSWidth);
	file->writeLE<Uint32>(pPlayer->_pSFNum);
	file->skip(4 * 8); // Skip pointers _pHAnim
	file->writeLE<Uint32>(pPlayer->_pHFrames);
	file->writeLE<Uint32>(pPlayer->_pHWidth);
	file->skip(4 * 8); // Skip pointers _pDAnim
	file->writeLE<Uint32>(pPlayer->_pDFrames);
	file->writeLE<Uint32>(pPlayer->_pDWidth);
	file->skip(4 * 8); // Skip pointers _pBAnim
	file->writeLE<Uint32>(pPlayer->_pBFrames);
	file->writeLE<Uint32>(pPlayer->_pBWidth);

	SaveItems(file, pPlayer->InvBody, NUM_INVLOC);
	SaveItems(file, pPlayer->InvList, NUM_INV_GRID_ELEM);
	file->writeLE<Uint32>(pPlayer->_pNumInv);
	for (int i = 0; i < NUM_INV_GRID_ELEM; i++)
		file->writeByte(pPlayer->InvGrid[i]);
	SaveItems(file, pPlayer->SpdList, MAXBELTITEMS);
	SaveItem(file, &pPlayer->HoldItem);

	file->writeLE<Uint32>(pPlayer->_pIMinDam);
	file->writeLE<Uint32>(pPlayer->_pIMaxDam);
	file->writeLE<Uint32>(pPlayer->_pIAC);
	file->writeLE<Uint32>(pPlayer->_pIBonusDam);
	file->writeLE<Uint32>(pPlayer->_pIBonusToHit);
	file->writeLE<Uint32>(pPlayer->_pIBonusAC);
	file->writeLE<Uint32>(pPlayer->_pIBonusDamMod);
	file->skip(4); // Alignment

	file->writeLE<Uint64>(pPlayer->_pISpells);
	file->writeLE<Uint32>(pPlayer->_pIFlags);
	file->writeLE<Uint32>(pPlayer->_pIGetHit);

	file->writeByte(pPlayer->_pISplLvlAdd);
	file->writeByte(pPlayer->_pISplCost);
	file->skip(2); // Alignment
	file->writeLE<Uint32>(pPlayer->_pISplDur);
	file->writeLE<Uint32>(pPlayer->_pIEnAc);
	file->writeLE<Uint32>(pPlayer->_pIFMinDam);
	file->writeLE<Uint32>(pPlayer->_pIFMaxDam);
	file->writeLE<Uint32>(pPlayer->_pILMinDam);
	file->writeLE<Uint32>(pPlayer->_pILMaxDam);
	file->writeLE<Uint32>(pPlayer->_pOilType);
	file->writeByte(pPlayer->pTownWarps);
	file->writeByte(pPlayer->pDungMsgs);
	file->writeByte(pPlayer->pLvlLoad);
	if (gbIsHellfire)
		file->writeByte(pPlayer->pDungMsgs2);
	else
		file->writeByte(pPlayer->pBattleNet);
	file->writeByte(pPlayer->pManaShield);
	file->writeByte(pPlayer->pOriginalCathedral);
	file->skip(2); // Available bytes
	file->writeLE<Uint16>(pPlayer->wReflections);
	file->skip(14); // Available bytes

	file->writeLE<Uint32>(pPlayer->pDiabloKillLevel);
	file->writeLE<Uint32>(pPlayer->pDifficulty);
	file->writeLE<Uint32>(pPlayer->pDamAcFlags);
	file->skip(20); // Available bytes

	// Omit pointer _pNData
	// Omit pointer _pWData
	// Omit pointer _pAData
	// Omit pointer _pLData
	// Omit pointer _pFData
	// Omit pointer  _pTData
	// Omit pointer _pHData
	// Omit pointer _pDData
	// Omit pointer _pBData
	// Omit pointer pReserved
}

static void SaveMonster(SaveHelper *file, int i)
{
	MonsterStruct *pMonster = &monster[i];

	file->writeLE<Uint32>(pMonster->_mMTidx);
	file->writeLE<Uint32>(pMonster->_mmode);
	file->writeByte(pMonster->_mgoal);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pMonster->_mgoalvar1);
	file->writeLE<Uint32>(pMonster->_mgoalvar2);
	file->writeLE<Uint32>(pMonster->_mgoalvar3);
	file->skip(4); // Unused
	file->writeByte(pMonster->_pathcount);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pMonster->_mx);
	file->writeLE<Uint32>(pMonster->_my);
	file->writeLE<Uint32>(pMonster->_mfutx);
	file->writeLE<Uint32>(pMonster->_mfuty);
	file->writeLE<Uint32>(pMonster->_moldx);
	file->writeLE<Uint32>(pMonster->_moldy);
	file->writeLE<Uint32>(pMonster->_mxoff);
	file->writeLE<Uint32>(pMonster->_myoff);
	file->writeLE<Uint32>(pMonster->_mxvel);
	file->writeLE<Uint32>(pMonster->_myvel);
	file->writeLE<Uint32>(pMonster->_mdir);
	file->writeLE<Uint32>(pMonster->_menemy);
	file->writeByte(pMonster->_menemyx);
	file->writeByte(pMonster->_menemyy);
	file->skip(2); // Unused

	file->skip(4); // Skip pointer _mAnimData
	file->writeLE<Uint32>(pMonster->_mAnimDelay);
	file->writeLE<Uint32>(pMonster->_mAnimCnt);
	file->writeLE<Uint32>(pMonster->_mAnimLen);
	file->writeLE<Uint32>(pMonster->_mAnimFrame);
	file->skip(4); // Skip _meflag
	file->writeLE<Uint32>(pMonster->_mDelFlag);
	file->writeLE<Uint32>(pMonster->_mVar1);
	file->writeLE<Uint32>(pMonster->_mVar2);
	file->writeLE<Uint32>(pMonster->_mVar3);
	file->writeLE<Uint32>(pMonster->_mVar4);
	file->writeLE<Uint32>(pMonster->_mVar5);
	file->writeLE<Uint32>(pMonster->_mVar6);
	file->writeLE<Uint32>(pMonster->_mVar7);
	file->writeLE<Uint32>(pMonster->_mVar8);
	file->writeLE<Uint32>(pMonster->_mmaxhp);
	file->writeLE<Uint32>(pMonster->_mhitpoints);

	file->writeByte(pMonster->_mAi);
	file->writeByte(pMonster->_mint);
	file->skip(2); // Alignment
	file->writeLE<Uint32>(pMonster->_mFlags);
	file->writeByte(pMonster->_msquelch);
	file->skip(3); // Alignment
	file->skip(4); // Unused
	file->writeLE<Uint32>(pMonster->_lastx);
	file->writeLE<Uint32>(pMonster->_lasty);
	file->writeLE<Uint32>(pMonster->_mRndSeed);
	file->writeLE<Uint32>(pMonster->_mAISeed);
	file->skip(4); // Unused

	file->writeByte(pMonster->_uniqtype);
	file->writeByte(pMonster->_uniqtrans);
	file->writeByte(pMonster->_udeadval);

	file->writeByte(pMonster->mWhoHit);
	file->writeByte(pMonster->mLevel);
	file->skip(1); // Alignment
	file->writeLE<Uint16>(pMonster->mExp);

	file->writeByte(pMonster->mHit < SCHAR_MAX ? pMonster->mHit : SCHAR_MAX); // For backwards compatibility
	file->writeByte(pMonster->mMinDamage);
	file->writeByte(pMonster->mMaxDamage);
	file->writeByte(pMonster->mHit2 < SCHAR_MAX ? pMonster->mHit2 : SCHAR_MAX); // For backwards compatibility
	file->writeByte(pMonster->mMinDamage2);
	file->writeByte(pMonster->mMaxDamage2);
	file->writeByte(pMonster->mArmorClass);
	file->skip(1); // Alignment
	file->writeLE<Uint16>(pMonster->mMagicRes);
	file->skip(2); // Alignment

	file->writeLE<Uint32>(pMonster->mtalkmsg);
	file->writeByte(pMonster->leader);
	file->writeByte(pMonster->leaderflag);
	file->writeByte(pMonster->packsize);
	file->writeByte(pMonster->mlid);

	// Omit pointer mName;
	// Omit pointer MType;
	// Omit pointer MData;
}

static void SaveMissile(SaveHelper *file, int i)
{
	MissileStruct *pMissile = &missile[i];

	file->writeLE<Uint32>(pMissile->_mitype);
	file->writeLE<Uint32>(pMissile->_mix);
	file->writeLE<Uint32>(pMissile->_miy);
	file->writeLE<Uint32>(pMissile->_mixoff);
	file->writeLE<Uint32>(pMissile->_miyoff);
	file->writeLE<Uint32>(pMissile->_mixvel);
	file->writeLE<Uint32>(pMissile->_miyvel);
	file->writeLE<Uint32>(pMissile->_misx);
	file->writeLE<Uint32>(pMissile->_misy);
	file->writeLE<Uint32>(pMissile->_mitxoff);
	file->writeLE<Uint32>(pMissile->_mityoff);
	file->writeLE<Uint32>(pMissile->_mimfnum);
	file->writeLE<Uint32>(pMissile->_mispllvl);
	file->writeLE<Uint32>(pMissile->_miDelFlag);
	file->writeByte(pMissile->_miAnimType);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pMissile->_miAnimFlags);
	file->skip(4); // Skip pointer _miAnimData
	file->writeLE<Uint32>(pMissile->_miAnimDelay);
	file->writeLE<Uint32>(pMissile->_miAnimLen);
	file->writeLE<Uint32>(pMissile->_miAnimWidth);
	file->writeLE<Uint32>(pMissile->_miAnimWidth2);
	file->writeLE<Uint32>(pMissile->_miAnimCnt);
	file->writeLE<Uint32>(pMissile->_miAnimAdd);
	file->writeLE<Uint32>(pMissile->_miAnimFrame);
	file->writeLE<Uint32>(pMissile->_miDrawFlag);
	file->writeLE<Uint32>(pMissile->_miLightFlag);
	file->writeLE<Uint32>(pMissile->_miPreFlag);
	file->writeLE<Uint32>(pMissile->_miUniqTrans);
	file->writeLE<Uint32>(pMissile->_mirange);
	file->writeLE<Uint32>(pMissile->_misource);
	file->writeLE<Uint32>(pMissile->_micaster);
	file->writeLE<Uint32>(pMissile->_midam);
	file->writeLE<Uint32>(pMissile->_miHitFlag);
	file->writeLE<Uint32>(pMissile->_midist);
	file->writeLE<Uint32>(pMissile->_mlid);
	file->writeLE<Uint32>(pMissile->_mirnd);
	file->writeLE<Uint32>(pMissile->_miVar1);
	file->writeLE<Uint32>(pMissile->_miVar2);
	file->writeLE<Uint32>(pMissile->_miVar3);
	file->writeLE<Uint32>(pMissile->_miVar4);
	file->writeLE<Uint32>(pMissile->_miVar5);
	file->writeLE<Uint32>(pMissile->_miVar6);
	file->writeLE<Uint32>(pMissile->_miVar7);
	file->writeLE<Uint32>(pMissile->_miVar8);
}

static void SaveObject(SaveHelper *file, int i)
{
	ObjectStruct *pObject = &object[i];

	file->writeLE<Uint32>(pObject->_otype);
	file->writeLE<Uint32>(pObject->_ox);
	file->writeLE<Uint32>(pObject->_oy);
	file->writeLE<Uint32>(pObject->_oLight);
	file->writeLE<Uint32>(pObject->_oAnimFlag);
	file->skip(4); // Skip pointer _oAnimData
	file->writeLE<Uint32>(pObject->_oAnimDelay);
	file->writeLE<Uint32>(pObject->_oAnimCnt);
	file->writeLE<Uint32>(pObject->_oAnimLen);
	file->writeLE<Uint32>(pObject->_oAnimFrame);
	file->writeLE<Uint32>(pObject->_oAnimWidth);
	file->writeLE<Uint32>(pObject->_oAnimWidth2);
	file->writeLE<Uint32>(pObject->_oDelFlag);
	file->writeByte(pObject->_oBreak);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pObject->_oSolidFlag);
	file->writeLE<Uint32>(pObject->_oMissFlag);

	file->writeByte(pObject->_oSelFlag);
	file->skip(3); // Alignment
	file->writeLE<Uint32>(pObject->_oPreFlag);
	file->writeLE<Uint32>(pObject->_oTrapFlag);
	file->writeLE<Uint32>(pObject->_oDoorFlag);
	file->writeLE<Uint32>(pObject->_olid);
	file->writeLE<Uint32>(pObject->_oRndSeed);
	file->writeLE<Uint32>(pObject->_oVar1);
	file->writeLE<Uint32>(pObject->_oVar2);
	file->writeLE<Uint32>(pObject->_oVar3);
	file->writeLE<Uint32>(pObject->_oVar4);
	file->writeLE<Uint32>(pObject->_oVar5);
	file->writeLE<Uint32>(pObject->_oVar6);
	file->writeLE<Uint32>(pObject->_oVar7);
	file->writeLE<Uint32>(pObject->_oVar8);
}

static void SavePremium(SaveHelper *file, int i)
{
	SaveItem(file, &premiumitem[i]);
}

static void SaveQuest(SaveHelper *file, int i)
{
	QuestStruct *pQuest = &quests[i];

	file->writeByte(pQuest->_qlevel);
	file->writeByte(pQuest->_qtype);
	file->writeByte(pQuest->_qactive);
	file->writeByte(pQuest->_qlvltype);
	file->writeLE<Uint32>(pQuest->_qtx);
	file->writeLE<Uint32>(pQuest->_qty);
	file->writeByte(pQuest->_qslvl);
	file->writeByte(pQuest->_qidx);
	if (gbIsHellfire) {
		file->skip(2); // Alignment
		file->writeLE<Uint32>(pQuest->_qmsg);
	} else {
		file->writeByte(pQuest->_qmsg);
	}
	file->writeByte(pQuest->_qvar1);
	file->writeByte(pQuest->_qvar2);
	file->skip(2); // Alignment
	if (!gbIsHellfire)
		file->skip(1); // Alignment
	file->writeLE<Uint32>(pQuest->_qlog);

	file->writeBE<Uint32>(ReturnLvlX);
	file->writeBE<Uint32>(ReturnLvlY);
	file->writeBE<Uint32>(ReturnLvl);
	file->writeBE<Uint32>(ReturnLvlT);
	file->writeBE<Uint32>(DoomQuestState);
}

static void SaveLighting(SaveHelper *file, int i)
{
	LightListStruct *pLight = &LightList[i];

	file->writeLE<Uint32>(pLight->_lx);
	file->writeLE<Uint32>(pLight->_ly);
	file->writeLE<Uint32>(pLight->_lradius);
	file->writeLE<Uint32>(pLight->_lid);
	file->writeLE<Uint32>(pLight->_ldel);
	file->writeLE<Uint32>(pLight->_lunflag);
	file->skip(4); // Unused
	file->writeLE<Uint32>(pLight->_lunx);
	file->writeLE<Uint32>(pLight->_luny);
	file->writeLE<Uint32>(pLight->_lunr);
	file->writeLE<Uint32>(pLight->_xoff);
	file->writeLE<Uint32>(pLight->_yoff);
	file->writeLE<Uint32>(pLight->_lflags);
}

static void SaveVision(SaveHelper *file, int i)
{
	LightListStruct *pVision = &VisionList[i];

	file->writeLE<Uint32>(pVision->_lx);
	file->writeLE<Uint32>(pVision->_ly);
	file->writeLE<Uint32>(pVision->_lradius);
	file->writeLE<Uint32>(pVision->_lid);
	file->writeLE<Uint32>(pVision->_ldel);
	file->writeLE<Uint32>(pVision->_lunflag);
	file->skip(4); // Unused
	file->writeLE<Uint32>(pVision->_lunx);
	file->writeLE<Uint32>(pVision->_luny);
	file->writeLE<Uint32>(pVision->_lunr);
	file->writeLE<Uint32>(pVision->_xoff);
	file->writeLE<Uint32>(pVision->_yoff);
	file->writeLE<Uint32>(pVision->_lflags);
}

static void SavePortal(SaveHelper *file, int i)
{
	PortalStruct *pPortal = &portal[i];

	file->writeLE<Uint32>(pPortal->open);
	file->writeLE<Uint32>(pPortal->x);
	file->writeLE<Uint32>(pPortal->y);
	file->writeLE<Uint32>(pPortal->level);
	file->writeLE<Uint32>(pPortal->ltype);
	file->writeLE<Uint32>(pPortal->setlvl);
}

void SaveGame()
{
	SaveHelper file("game", FILEBUFF);

	if (gbIsSpawn && !gbIsHellfire)
		file.writeLE<Uint32>(LOAD_LE32("SHAR"));
	else if (gbIsSpawn && gbIsHellfire)
		file.writeLE<Uint32>(LOAD_LE32("SHLF"));
	else if (!gbIsSpawn && gbIsHellfire)
		file.writeLE<Uint32>(LOAD_LE32("HELF"));
	else if (!gbIsSpawn && !gbIsHellfire)
		file.writeLE<Uint32>(LOAD_LE32("RETL"));
	else
		app_fatal("Invalid game state");

	if (gbIsHellfire) {
		giNumberOfLevels = 25;
		giNumberQuests = 24;
		giNumberOfSmithPremiumItems = 15;
	} else {
		giNumberOfLevels = 17;
		giNumberQuests = 16;
		giNumberOfSmithPremiumItems = 6;
	}

	file.writeByte(setlevel);
	file.writeBE<Uint32>(setlvlnum);
	file.writeBE<Uint32>(currlevel);
	file.writeBE<Uint32>(leveltype);
	file.writeBE<Uint32>(ViewX);
	file.writeBE<Uint32>(ViewY);
	file.writeByte(invflag);
	file.writeByte(chrflag);
	file.writeBE<Uint32>(nummonsters);
	file.writeBE<Uint32>(numitems);
	file.writeBE<Uint32>(nummissiles);
	file.writeBE<Uint32>(nobjects);

	for (int i = 0; i < giNumberOfLevels; i++) {
		file.writeBE<Uint32>(glSeedTbl[i]);
		file.writeBE<Uint32>(gnLevelTypeTbl[i]);
	}

	plr[myplr].pDifficulty = gnDifficulty;
	SavePlayer(&file, myplr);

	for (int i = 0; i < giNumberQuests; i++)
		SaveQuest(&file, i);
	for (int i = 0; i < MAXPORTAL; i++)
		SavePortal(&file, i);
	for (int i = 0; i < MAXMONSTERS; i++)
		file.writeBE<Uint32>(monstkills[i]);

	if (leveltype != DTYPE_TOWN) {
		for (int i = 0; i < MAXMONSTERS; i++)
			file.writeBE<Uint32>(monstactive[i]);
		for (int i = 0; i < nummonsters; i++)
			SaveMonster(&file, monstactive[i]);
		for (int i = 0; i < MAXMISSILES; i++)
			file.writeByte(missileactive[i]);
		for (int i = 0; i < MAXMISSILES; i++)
			file.writeByte(missileavail[i]);
		for (int i = 0; i < nummissiles; i++)
			SaveMissile(&file, missileactive[i]);
		for (int i = 0; i < MAXOBJECTS; i++)
			file.writeByte(objectactive[i]);
		for (int i = 0; i < MAXOBJECTS; i++)
			file.writeByte(objectavail[i]);
		for (int i = 0; i < nobjects; i++)
			SaveObject(&file, objectactive[i]);

		file.writeBE<Uint32>(numlights);

		for (int i = 0; i < MAXLIGHTS; i++)
			file.writeByte(lightactive[i]);
		for (int i = 0; i < numlights; i++)
			SaveLighting(&file, lightactive[i]);

		file.writeBE<Uint32>(visionid);
		file.writeBE<Uint32>(numvision);

		for (int i = 0; i < numvision; i++)
			SaveVision(&file, i);
	}

	for (int i = 0; i < MAXITEMS; i++)
		file.writeByte(itemactive[i]);
	for (int i = 0; i < MAXITEMS; i++)
		file.writeByte(itemavail[i]);
	for (int i = 0; i < numitems; i++)
		SaveItem(&file, &item[itemactive[i]]);
	for (int i = 0; i < 128; i++)
		file.writeByte(UniqueItemFlag[i]);

	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			file.writeByte(dLight[i][j]);
	}
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			file.writeByte(dFlags[i][j] & ~(BFLAG_MISSILE | BFLAG_VISIBLE | BFLAG_DEAD_PLAYER));
	}
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			file.writeByte(dPlayer[i][j]);
	}
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			file.writeByte(dItem[i][j]);
	}

	if (leveltype != DTYPE_TOWN) {
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeBE<Uint32>(dMonster[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dDead[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dObject[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dLight[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dPreLight[i][j]);
		}
		for (int j = 0; j < DMAXY; j++) {
			for (int i = 0; i < DMAXX; i++)
				file.writeByte(automapview[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dMissile[i][j]);
		}
	}

	file.writeBE<Uint32>(numpremium);
	file.writeBE<Uint32>(premiumlevel);

	for (int i = 0; i < giNumberOfSmithPremiumItems; i++)
		SavePremium(&file, i);

	file.writeByte(automapflag);
	file.writeBE<Uint32>(AutoMapScale);

	file.flush();

	gbValidSaveFile = TRUE;
	pfile_rename_temp_to_perm();
	pfile_write_hero();
}

void SaveLevel()
{
	DoUnVision(plr[myplr]._px, plr[myplr]._py, plr[myplr]._pLightRad); // fix for vision staying on the level

	if (currlevel == 0)
		glSeedTbl[0] = AdvanceRndSeed();

	char szName[MAX_PATH];
	GetTempLevelNames(szName);
	SaveHelper file(szName, FILEBUFF);

	if (leveltype != DTYPE_TOWN) {
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dDead[i][j]);
		}
	}

	file.writeBE<Uint32>(nummonsters);
	file.writeBE<Uint32>(numitems);
	file.writeBE<Uint32>(nobjects);

	if (leveltype != DTYPE_TOWN) {
		for (int i = 0; i < MAXMONSTERS; i++)
			file.writeBE<Uint32>(monstactive[i]);
		for (int i = 0; i < nummonsters; i++)
			SaveMonster(&file, monstactive[i]);
		for (int i = 0; i < MAXOBJECTS; i++)
			file.writeByte(objectactive[i]);
		for (int i = 0; i < MAXOBJECTS; i++)
			file.writeByte(objectavail[i]);
		for (int i = 0; i < nobjects; i++)
			SaveObject(&file, objectactive[i]);
	}

	for (int i = 0; i < MAXITEMS; i++)
		file.writeByte(itemactive[i]);
	for (int i = 0; i < MAXITEMS; i++)
		file.writeByte(itemavail[i]);
	for (int i = 0; i < numitems; i++)
		SaveItem(&file, &item[itemactive[i]]);

	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			file.writeByte(dFlags[i][j] & ~(BFLAG_MISSILE | BFLAG_VISIBLE | BFLAG_DEAD_PLAYER));
	}
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			file.writeByte(dItem[i][j]);
	}

	if (leveltype != DTYPE_TOWN) {
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeBE<Uint32>(dMonster[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dObject[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dLight[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dPreLight[i][j]);
		}
		for (int j = 0; j < DMAXY; j++) {
			for (int i = 0; i < DMAXX; i++)
				file.writeByte(automapview[i][j]);
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				file.writeByte(dMissile[i][j]);
		}
	}

	if (!setlevel)
		plr[myplr]._pLvlVisited[currlevel] = TRUE;
	else
		plr[myplr]._pSLvlVisited[setlvlnum] = TRUE;
}

void LoadLevel()
{
	char szName[MAX_PATH];
	GetPermLevelNames(szName);
	LoadHelper file(szName);
	if (!file.isValid())
		app_fatal("Unable to open save file archive");

	if (leveltype != DTYPE_TOWN) {
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dDead[i][j] = file.next<Uint8>();
		}
		SetDead();
	}

	nummonsters = file.nextBE<Uint32>();
	numitems = file.nextBE<Uint32>();
	nobjects = file.nextBE<Uint32>();

	if (leveltype != DTYPE_TOWN) {
		for (int i = 0; i < MAXMONSTERS; i++)
			monstactive[i] = file.nextBE<Uint32>();
		for (int i = 0; i < nummonsters; i++)
			LoadMonster(&file, monstactive[i]);
		for (int i = 0; i < MAXOBJECTS; i++)
			objectactive[i] = file.next<Uint8>();
		for (int i = 0; i < MAXOBJECTS; i++)
			objectavail[i] = file.next<Uint8>();
		for (int i = 0; i < nobjects; i++)
			LoadObject(&file, objectactive[i]);
		if (!gbSkipSync) {
			for (int i = 0; i < nobjects; i++)
				SyncObjectAnim(objectactive[i]);
		}
	}

	for (int i = 0; i < MAXITEMS; i++)
		itemactive[i] = file.next<Uint8>();
	for (int i = 0; i < MAXITEMS; i++)
		itemavail[i] = file.next<Uint8>();
	for (int i = 0; i < numitems; i++)
		LoadItem(&file, itemactive[i]);

	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			dFlags[i][j] = file.next<Uint8>();
	}
	for (int j = 0; j < MAXDUNY; j++) {
		for (int i = 0; i < MAXDUNX; i++)
			dItem[i][j] = file.next<Uint8>();
	}

	if (leveltype != DTYPE_TOWN) {
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dMonster[i][j] = file.nextBE<Uint32>();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dObject[i][j] = file.next<Uint8>();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dLight[i][j] = file.next<Uint8>();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dPreLight[i][j] = file.next<Uint8>();
		}
		for (int j = 0; j < DMAXY; j++) {
			for (int i = 0; i < DMAXX; i++)
				automapview[i][j] = file.nextBool8();
		}
		for (int j = 0; j < MAXDUNY; j++) {
			for (int i = 0; i < MAXDUNX; i++)
				dMissile[i][j] = 0; /// BUGFIX: supposed to load saved missiles with "file.next<Uint8>()"?
		}
	}

	if (!gbSkipSync) {
		AutomapZoomReset();
		ResyncQuests();
		SyncPortals();
		dolighting = TRUE;
	}

	for (int i = 0; i < MAX_PLRS; i++) {
		if (plr[i].plractive && currlevel == plr[i].plrlevel)
			LightList[plr[i]._plid]._lunflag = TRUE;
	}
}

DEVILUTION_END_NAMESPACE
