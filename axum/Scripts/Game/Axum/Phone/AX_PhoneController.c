// Original Axum input/preview adapter. No RPPhone banking, jobs or player RPCs.
class AX_PhonePreviewEnvironment : AX_TradeEnvironment
{
	override bool IsSiteUsable(AX_TradeSiteDefinition site) { return site != null; }
}

modded class SCR_PlayerController
{
	protected bool m_bAXPhoneBound;
	protected ref AX_DrugAppSession m_AXPhoneSession;
	protected string m_sAXPhoneCharacter;
	protected int m_iAXPhoneToggleTick;
	protected bool m_bAXPhoneToggled;

	override void OnUpdate(float timeSlice)
	{
		super.OnUpdate(timeSlice);
		if (GetGame().GetPlayerController() != this || !GetControlledEntity()) return;
		InputManager input = GetGame().GetInputManager();
		if (!input) return;
		if (!m_bAXPhoneBound)
		{
			input.AddActionListener("AX_OpenPhone", EActionTrigger.DOWN, AX_OpenPhoneAction);
			m_bAXPhoneBound = true;
		}
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		if (m_bAXPhoneBound && GetGame().GetInputManager())
			GetGame().GetInputManager().RemoveActionListener("AX_OpenPhone", EActionTrigger.DOWN, AX_OpenPhoneAction);
		m_bAXPhoneBound = false;
		if (GetGame().GetPlayerController() == this && GetGame().GetMenuManager())
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.AX_Phone);
	}

	void ~SCR_PlayerController()
	{
		if (m_bAXPhoneBound && GetGame() && GetGame().GetInputManager())
			GetGame().GetInputManager().RemoveActionListener("AX_OpenPhone", EActionTrigger.DOWN, AX_OpenPhoneAction);
	}

	void AX_OpenPhoneAction()
	{
		if (GetGame().GetPlayerController() != this || !GetControlledEntity() || !GetGame().GetWorld()) return;
		// Menus can suspend world updates in Workbench. UI debounce must use
		// real elapsed time so the same F6 action can always close the menu.
		if (m_bAXPhoneToggled && System.GetTickCount(m_iAXPhoneToggleTick) < 250) return;
		m_iAXPhoneToggleTick = System.GetTickCount();
		m_bAXPhoneToggled = true;
		MenuManager menus = GetGame().GetMenuManager();
		if (menus.FindMenuByPreset(ChimeraMenuPreset.AX_Phone))
			menus.CloseMenuByPreset(ChimeraMenuPreset.AX_Phone);
		else if (!menus.IsAnyMenuOpen())
			menus.OpenMenu(ChimeraMenuPreset.AX_Phone, 0, true);
	}

	AX_DrugAppSession AX_GetPhoneSession()
	{
		// Explicit preview boundary: remote clients and published servers cannot
		// create pretend appointments or reset buyer budgets through this adapter.
		#ifndef WORKBENCH
		return null;
		#else
		if (GetGame().GetPlayerController() != this || !GetControlledEntity() || !Replication.IsServer()) return null;
		if (!GetGame().GetWorldFile().Contains("Worlds/DebugWorld/DebugWorld.ent")) return null;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		if (players.Count() > 1) return null;
		string characterId;
		EL_PlayerAccount account = EL_PlayerAccountManager.GetInstance().GetFromCache(GetPlayerId());
		if (account && account.GetActiveCharacter()) characterId = account.GetActiveCharacter().GetId();
		if (characterId.IsEmpty()) characterId = "workbench-preview-" + GetPlayerId().ToString();
		if (!m_AXPhoneSession || m_sAXPhoneCharacter != characterId)
		{
			m_sAXPhoneCharacter = characterId;
			m_AXPhoneSession = new AX_DrugAppSession("phone-preview", characterId, new AX_PhonePreviewEnvironment());
		}
		return m_AXPhoneSession;
		#endif
	}
}
