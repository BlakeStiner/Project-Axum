[BaseContainerProps(category: "Autotest")]
class AX_PhoneSuite : SCR_AutotestSuiteBase
{
	override ResourceName GetWorldFile() { return ResourceName.Empty; }
}

class AX_PhoneTestCase : SCR_AutotestCaseBase
{
	ref AX_DrugAppSession m_App;
	void CreateApp()
	{
		m_App = new AX_DrugAppSession("test-phone", "alice", new AX_TestTradeEnvironment());
	}
	void Expect(bool condition, string message)
	{
		if (!condition) SetFailure(SCR_AutotestFailure.Create(message));
	}
}

[Test(suite: AX_PhoneSuite, timeoutS: 5)]
class AX_PhoneTest_OfferAndBooking : AX_PhoneTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateApp();
		bool ok = m_App.RequestOffer(0, 3, 0) == AX_ETradeResult.OK;
		ok = ok && m_App.GetAvailableUnits(0) == 20;
		ok = ok && m_App.GetOfferTotal() == 300;
		ok = ok && m_App.AcceptOffer(1, 0) == AX_ETradeResult.OK;
		AX_TradeOrder order = m_App.GetOrder(1);
		ok = ok && order && order.m_sCharacterId == "alice" && order.m_iUnits == 3;
		ok = ok && order.m_eState == AX_ETradeState.BOOKED && m_App.GetAvailableUnits(0) == 17;
		Expect(ok, "Offers must not reserve demand until accepted; accepted terms must match the service order.");
	}
}

[Test(suite: AX_PhoneSuite, timeoutS: 5)]
class AX_PhoneTest_InvalidAndExpiredOffer : AX_PhoneTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateApp();
		bool ok = m_App.RequestOffer(-1, 1, 0) == AX_ETradeResult.INVALID_REQUEST;
		ok = ok && m_App.RequestOffer(0, 6, 0) == AX_ETradeResult.INVALID_REQUEST;
		ok = ok && m_App.RequestOffer(0, 0, 0) == AX_ETradeResult.INVALID_REQUEST;
		ok = ok && m_App.RequestOffer(0, 2, 0) == AX_ETradeResult.OK;
		ok = ok && m_App.AcceptOffer(60, 0) == AX_ETradeResult.DEADLINE_PASSED;
		ok = ok && !m_App.GetOrder(60) && m_App.GetAvailableUnits(0) == 20;
		Expect(ok, "Invalid or expired quotes must never create an order or consume demand.");
	}
}

[Test(suite: AX_PhoneSuite, timeoutS: 5)]
class AX_PhoneTest_DuplicateAcceptance : AX_PhoneTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateApp();
		m_App.RequestOffer(0, 2, 0);
		bool ok = m_App.AcceptOffer(0, 0) == AX_ETradeResult.OK;
		string id = m_App.GetOrder(0).m_sId;
		ok = ok && m_App.AcceptOffer(0, 0) == AX_ETradeResult.INVALID_STATE;
		ok = ok && m_App.RequestOffer(1, 2, 1) == AX_ETradeResult.UNAVAILABLE;
		ok = ok && m_App.GetOrder(1).m_sId == id && m_App.GetAvailableUnits(0) == 18;
		Expect(ok, "Repeated acceptance or another contact must not reserve a second meeting.");
	}
}

[Test(suite: AX_PhoneSuite, timeoutS: 5)]
class AX_PhoneTest_CancelAndRotate : AX_PhoneTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateApp();
		m_App.RequestOffer(0, 2, 0);
		m_App.AcceptOffer(0, 0);
		string oldRegion = m_App.GetOrder(0).m_sRegionId;
		bool ok = m_App.CancelOrder(1) == AX_ETradeResult.OK;
		ok = ok && m_App.CancelOrder(1) == AX_ETradeResult.OK && m_App.GetAvailableUnits(0) == 20;
		m_App.RequestOffer(0, 2, 61);
		ok = ok && m_App.AcceptOffer(61, 0) == AX_ETradeResult.OK;
		ok = ok && m_App.GetOrder(61).m_sRegionId != oldRegion;
		Expect(ok, "Cancellation must refund once and a new meeting must avoid the cooled-down region.");
	}
}

[Test(suite: AX_PhoneSuite, timeoutS: 5)]
class AX_PhoneTest_ExpiryAndClearedQuote : AX_PhoneTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateApp();
		m_App.RequestOffer(0, 1, 0);
		m_App.ClearOffer();
		bool ok = m_App.AcceptOffer(0, 0) == AX_ETradeResult.INVALID_STATE;
		m_App.RequestOffer(0, 1, 0);
		m_App.AcceptOffer(0, 0);
		ok = ok && m_App.GetOrder(600).m_eState == AX_ETradeState.EXPIRED;
		ok = ok && m_App.GetAvailableUnits(0) == 20;
		ok = ok && m_App.GetConversation(0).Contains("Reserved.");
		Expect(ok, "Quantity/contact changes invalidate offers; appointment expiry restores demand.");
	}
}

[Test(suite: AX_PhoneSuite, timeoutS: 5)]
class AX_PhoneTest_Resources : AX_PhoneTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		ResourceName path = "{4E6001DE6EAF4601}ThirdParty/RPPhone/UI/AX_Phone.layout";
		Resource resource = Resource.Load(path);
		Expect(resource && resource.IsValid(), "Phone layout must resolve.");
		// Textures use the graphics loader, not Resource.Load. Actual widget
		// creation verifies the layout/controller contract on the dummy renderer;
		// texture appearance is checked separately in the graphical client.
		Widget root = GetGame().GetWorkspace().CreateWidgets(path);
		Expect(root != null, "Phone widget tree must instantiate.");
		if (!root) return;
		root.Update();
		Widget shell = root.FindAnyWidget("AX_PhoneShell");
		float rootW, rootH, shellX, shellY, shellW, shellH;
		root.GetScreenSize(rootW, rootH);
		shell.GetScreenPos(shellX, shellY);
		shell.GetScreenSize(shellW, shellH);
		Print(string.Format("Phone geometry: root %1 x %2; shell %3,%4 %5 x %6", rootW, rootH, shellX, shellY, shellW, shellH));
		Expect(rootW > 0 && rootH > 0 && shellW > 0 && shellH > 0 && shellX >= 0 && shellY >= 0 && shellX + shellW <= rootW && shellY + shellH <= rootH, "Phone shell must occupy visible space inside the workspace.");
		TextWidget conversation = TextWidget.Cast(root.FindAnyWidget("AX_Conversation"));
		Expect(conversation && conversation.GetTextWrapping() && root.FindAnyWidget("AX_Meeting"), "Conversation must wrap; appointment details must exist.");
		array<string> controls = {"AX_DrugApp", "AX_Contact0", "AX_Contact1", "AX_Minus", "AX_Plus", "AX_Request", "AX_Accept", "AX_Cancel", "AX_Home", "AX_Close"};
		foreach (string name : controls)
		{
			Widget button = root.FindAnyWidget(name);
			Expect(button && button.GetChildren() && !button.GetChildren().GetSibling(), name + " must have exactly one content child.");
			SCR_ButtonTextComponent component = SCR_ButtonTextComponent.GetButtonText(name, root);
			Expect(component && button.FindAnyWidget("Text"), name + " must bind and retain its visible label.");
		}
		root.RemoveFromHierarchy();
		// A path fallback can create widgets even when the resource database has
		// not registered the menu override. Exercise the real preset as well.
		MenuManager menus = GetGame().GetMenuManager();
		AX_PhoneMenu phone = AX_PhoneMenu.Cast(menus.OpenMenu(ChimeraMenuPreset.AX_Phone, 0, true));
		Expect(phone && phone.GetRootWidget(), "Phone preset must resolve to the Axum controller and layout.");
		if (phone && phone.GetRootWidget())
		{
			Widget menuRoot = phone.GetRootWidget();
			menuRoot.Update();
			menuRoot.GetScreenSize(rootW, rootH);
			shell = menuRoot.FindAnyWidget("AX_PhoneShell");
			shell.GetScreenPos(shellX, shellY);
			shell.GetScreenSize(shellW, shellH);
			Print(string.Format("Menu geometry: root %1 x %2; shell %3,%4 %5 x %6", rootW, rootH, shellX, shellY, shellW, shellH));
			Expect(rootW > 0 && rootH > 0 && shellX >= 0 && shellY >= 0 && shellX + shellW <= rootW && shellY + shellH <= rootH, "The actual menu must keep its phone inside the viewport too.");
		}
		menus.CloseMenuByPreset(ChimeraMenuPreset.AX_Phone);

		Resource inputResource = Resource.Load("{795184CF9AD764DB}Configs/System/chimeraInputCommon.conf");
		Expect(inputResource && inputResource.IsValid(), "Merged input config must resolve.");
		if (!inputResource || !inputResource.IsValid()) return;
		BaseContainer inputConfig = inputResource.GetResource().ToBaseContainer();
		Expect(FindContext(inputConfig, "AX_PhoneContext") == null, "Phone must not introduce an always-active context that masks gameplay input.");
		CheckBindings(inputConfig, "CharacterGeneralContext", "AX_OpenPhone", "Inventory");
		CheckBindings(inputConfig, "CharacterMovementContext", "CharacterForward", "CharacterRight");
		CheckBindings(inputConfig, "MenuContext", "AX_OpenPhone", "MenuBack");
		CheckBindings(inputConfig, "MenuWithEditorContext", "AX_OpenPhone", "MenuSelect");
	}
	protected BaseContainer FindContext(BaseContainer config, string name)
	{
		if (!config) return null;
		BaseContainerList contexts = config.GetObjectArray("Contexts");
		if (!contexts) return null;
		for (int i = 0; i < contexts.Count(); i++)
		{
			BaseContainer context = contexts.Get(i);
			if (context.GetName() == name) return context;
		}
		return null;
	}
	protected void CheckBindings(BaseContainer config, string name, string first, string second)
	{
		BaseContainer context = FindContext(config, name);
		array<string> refs = {};
		Expect(context && context.Get("ActionRefs", refs) && refs.Contains(first) && refs.Contains(second), name + " must preserve baseline actions alongside the phone binding.");
	}
}
