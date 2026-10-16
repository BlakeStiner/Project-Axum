modded enum ChimeraMenuPreset
{
	AX_Phone
}

class AX_PhoneMenu : ChimeraMenuBase
{
	protected Widget m_Root;
	protected ref AX_DrugAppSession m_Session;
	protected ref array<Widget> m_aFocus = {};
	protected int m_iPage;
	protected int m_iContact;
	protected int m_iQuantity = 1;
	protected int m_iFocus;
	protected float m_fRefresh;
	protected int m_iActionTick;
	protected bool m_bActionHandled;
	protected string m_sConversation;

	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_Root = GetRootWidget();
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller) m_Session = controller.AX_GetPhoneSession();
		BindButtons();
		ShowPage(0);
		Print("[Axum Phone] Opened");
	}

	override void OnMenuClose()
	{
		Print("[Axum Phone] Closed");
		// Release invoker subscriptions and widget references before the world or
		// script module is torn down. The phone may still be open when Play stops.
		if (m_Root) UnbindButtons();
		m_aFocus.Clear();
		m_Session = null;
		m_Root = null;
		super.OnMenuClose();
	}

	protected void UnbindButtons()
	{
		SCR_ButtonTextComponent b = SCR_ButtonTextComponent.GetButtonText("AX_DrugApp", m_Root);
		if (b) b.m_OnClicked.Remove(OnDrugApp);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Contact0", m_Root);
		if (b) b.m_OnClicked.Remove(OnContact0);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Contact1", m_Root);
		if (b) b.m_OnClicked.Remove(OnContact1);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Minus", m_Root);
		if (b) b.m_OnClicked.Remove(OnMinus);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Plus", m_Root);
		if (b) b.m_OnClicked.Remove(OnPlus);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Request", m_Root);
		if (b) b.m_OnClicked.Remove(OnRequest);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Accept", m_Root);
		if (b) b.m_OnClicked.Remove(OnAccept);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Cancel", m_Root);
		if (b) b.m_OnClicked.Remove(OnCancel);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Home", m_Root);
		if (b) b.m_OnClicked.Remove(OnHome);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Close", m_Root);
		if (b) b.m_OnClicked.Remove(OnClose);
	}

	protected void BindButtons()
	{
		SCR_ButtonTextComponent b = SCR_ButtonTextComponent.GetButtonText("AX_DrugApp", m_Root);
		if (b) b.m_OnClicked.Insert(OnDrugApp);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Contact0", m_Root);
		if (b) b.m_OnClicked.Insert(OnContact0);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Contact1", m_Root);
		if (b) b.m_OnClicked.Insert(OnContact1);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Minus", m_Root);
		if (b) b.m_OnClicked.Insert(OnMinus);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Plus", m_Root);
		if (b) b.m_OnClicked.Insert(OnPlus);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Request", m_Root);
		if (b) b.m_OnClicked.Insert(OnRequest);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Accept", m_Root);
		if (b) b.m_OnClicked.Insert(OnAccept);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Cancel", m_Root);
		if (b) b.m_OnClicked.Insert(OnCancel);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Home", m_Root);
		if (b) b.m_OnClicked.Insert(OnHome);
		b = SCR_ButtonTextComponent.GetButtonText("AX_Close", m_Root);
		if (b) b.m_OnClicked.Insert(OnClose);
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!GetGame().GetPlayerController() || !GetGame().GetPlayerController().GetControlledEntity()) { Close(); return; }
		InputManager input = GetGame().GetInputManager();
		input.ActivateContext("MenuContext");
		// AX_OpenPhone belongs to the baseline character/menu contexts. A
		// separate higher-priority context would suppress normal movement.
		if (input.GetActionTriggered("AX_OpenPhone"))
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (controller) controller.AX_OpenPhoneAction();
			return;
		}
		if (input.GetActionTriggered("MenuBack")) { OnBack(); return; }
		if (input.GetActionTriggered("MenuDown")) { MoveFocus(1); return; }
		if (input.GetActionTriggered("MenuUp")) { MoveFocus(-1); return; }
		if (input.GetActionTriggered("MenuSelect") && !m_aFocus.IsEmpty()) { Handle(m_aFocus[m_iFocus].GetName()); return; }
		m_fRefresh += tDelta;
		if (m_fRefresh >= 0.25)
		{
			m_fRefresh = 0;
			Render();
		}
	}
	protected int Now() { return GetGame().GetWorld().GetWorldTime() / 1000; }
	protected bool ActionReady()
	{
		if (m_bActionHandled && System.GetTickCount(m_iActionTick) < 150) return false;
		m_iActionTick = System.GetTickCount();
		m_bActionHandled = true;
		return true;
	}
	protected void Text(string name, string value)
	{
		TextWidget widget = TextWidget.Cast(m_Root.FindAnyWidget(name));
		if (widget) widget.SetText(value);
	}
	protected void Enabled(string name, bool enabled)
	{
		Widget widget = m_Root.FindAnyWidget(name);
		if (widget) widget.SetEnabled(enabled);
	}
	protected void AddFocus(string name)
	{
		Widget widget = m_Root.FindAnyWidget(name);
		if (widget && widget.IsEnabled()) m_aFocus.Insert(widget);
	}
	protected void RebuildFocus()
	{
		m_aFocus.Clear();
		if (m_iPage == 0) AddFocus("AX_DrugApp");
		if (m_iPage == 1) { AddFocus("AX_Contact0"); AddFocus("AX_Contact1"); }
		if (m_iPage == 2)
		{
			AddFocus("AX_Minus"); AddFocus("AX_Plus"); AddFocus("AX_Request"); AddFocus("AX_Accept"); AddFocus("AX_Cancel");
		}
		AddFocus("AX_Home"); AddFocus("AX_Close");
		if (m_iFocus >= m_aFocus.Count()) m_iFocus = 0;
	}
	protected void MoveFocus(int step)
	{
		if (!ActionReady()) return;
		RebuildFocus();
		m_iFocus = (m_iFocus + step + m_aFocus.Count()) % m_aFocus.Count();
		GetGame().GetWorkspace().SetFocusedWidget(m_aFocus[m_iFocus]);
	}
	protected void ShowPage(int page)
	{
		m_iPage = page;
		m_iFocus = 0;
		m_Root.FindAnyWidget("AX_HomePage").SetVisible(page == 0);
		m_Root.FindAnyWidget("AX_ContactsPage").SetVisible(page == 1);
		m_Root.FindAnyWidget("AX_ThreadPage").SetVisible(page == 2);
		Render();
		RebuildFocus();
		if (!m_aFocus.IsEmpty()) GetGame().GetWorkspace().SetFocusedWidget(m_aFocus[0]);
	}
	protected void Render()
	{
		if (!m_Root) return;
		Text("AX_PageTitle", "AXUM");
		if (m_iPage == 1) Text("AX_PageTitle", "Street trade");
		if (m_iPage == 2 && m_Session) Text("AX_PageTitle", m_Session.GetContactName(m_iContact));
		Enabled("AX_Contact0", m_Session != null);
		Enabled("AX_Contact1", m_Session != null);
		if (!m_Session)
		{
			Text("AX_ContactsInfo", "Contacts are available in the single-player debug world preview.");
			return;
		}
		Text("AX_ContactsInfo", "Arrange a private meeting. Each contact has limited demand.");
		if (m_iPage != 2) return;
		int now = Now();
		string conversation = m_Session.GetConversation(m_iContact);
		if (conversation != m_sConversation)
		{
			m_sConversation = conversation;
			Text("AX_Conversation", conversation);
			ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(m_Root.FindAnyWidget("AX_Scroll"));
			if (scroll) { scroll.Update(); scroll.SetSliderPos(0, 1); }
		}
		Text("AX_Quantity", m_iQuantity.ToString() + " packages");
		Text("AX_Price", "$" + m_Session.GetUnitPrice(m_iContact).ToString() + " per package");
		Text("AX_Notice", m_Session.GetNotice());
		Enabled("AX_Minus", m_iQuantity > 1);
		Enabled("AX_Plus", m_iQuantity < m_Session.GetMaxUnits(m_iContact));
		Enabled("AX_Accept", m_Session.HasOffer(m_iContact, now));
		AX_TradeOrder order = m_Session.GetOrder(now);
		bool active = order && (order.m_eState == AX_ETradeState.BOOKED || order.m_eState == AX_ETradeState.READY);
		Enabled("AX_Cancel", active);
		string meeting = "No active meeting.";
		if (active)
		{
			int remaining = Math.Max(0, order.m_iDeadline - now);
			meeting = AX_DrugAppSession.GetSiteName(order.m_sSiteId) + "  |  " + remaining.ToString() + "s left\n";
			meeting += "Position: " + order.m_vPosition.ToString() + "\nPreview meeting: buyer handoff is not enabled.";
		}
		else if (order && order.m_eState == AX_ETradeState.EXPIRED) meeting = "Your meeting expired. Request a new offer.";
		else if (order && order.m_eState == AX_ETradeState.CANCELLED) meeting = "Meeting cancelled. The area will rotate next time.";
		Text("AX_Meeting", meeting);
		RebuildFocus();
	}

	protected void Handle(string name)
	{
		if (!ActionReady()) return;
		if (name == "AX_Close") { Close(); return; }
		if (name == "AX_Home") { ShowPage(0); return; }
		if (name == "AX_DrugApp") { ShowPage(1); return; }
		if (!m_Session) return;
		if (name == "AX_Contact0" || name == "AX_Contact1")
		{
			m_iContact = 0;
			if (name == "AX_Contact1") m_iContact = 1;
			m_iQuantity = 1;
			m_Session.ClearOffer();
			ShowPage(2);
			return;
		}
		if (name == "AX_Minus") { m_iQuantity = Math.Max(1, m_iQuantity - 1); m_Session.ClearOffer(); }
		if (name == "AX_Plus") { m_iQuantity = Math.Min(m_Session.GetMaxUnits(m_iContact), m_iQuantity + 1); m_Session.ClearOffer(); }
		if (name == "AX_Request") m_Session.RequestOffer(m_iContact, m_iQuantity, Now());
		if (name == "AX_Accept") m_Session.AcceptOffer(Now(), Math.RandomInt(0, 1000000));
		if (name == "AX_Cancel") m_Session.CancelOrder(Now());
		Render();
	}
	protected void OnBack()
	{
		if (!ActionReady()) return;
		if (m_iPage == 2) ShowPage(1);
		else if (m_iPage == 1) ShowPage(0);
		else Close();
	}
	protected void OnDrugApp() { Handle("AX_DrugApp"); }
	protected void OnContact0() { Handle("AX_Contact0"); }
	protected void OnContact1() { Handle("AX_Contact1"); }
	protected void OnMinus() { Handle("AX_Minus"); }
	protected void OnPlus() { Handle("AX_Plus"); }
	protected void OnRequest() { Handle("AX_Request"); }
	protected void OnAccept() { Handle("AX_Accept"); }
	protected void OnCancel() { Handle("AX_Cancel"); }
	protected void OnHome() { Handle("AX_Home"); }
	protected void OnClose() { Handle("AX_Close"); }
}
