[BaseContainerProps(category: "Autotest")]
class AX_ShopSystemProbeSuite : SCR_AutotestSuiteBase
{
	override ResourceName GetWorldFile() { return ResourceName.Empty; }
}

class AX_ShopProbeTestCase : SCR_AutotestCaseBase
{
	IEntity m_Buyer;
	AX_ShopProbeBuyerComponent m_Shop;

	void Expect(bool condition, string message)
	{
		if (!condition) SetFailure(SCR_AutotestFailure.Create(message));
	}

	bool SpawnBuyer()
	{
		Resource resource = Resource.Load("{4E6001DE6EAF4710}Prefabs/AX_ShopProbeBuyer.et");
		if (!resource || !resource.IsValid()) return false;
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = "0 1000 0";
		m_Buyer = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), parameters);
		if (m_Buyer) m_Shop = AX_ShopProbeBuyerComponent.Cast(m_Buyer.FindComponent(AX_ShopProbeBuyerComponent));
		return m_Buyer && m_Shop;
	}

	[TestStep(TestStage.TearDown)]
	void Cleanup()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.AX_ShopProbeMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.ADM_ShopMenu);
		if (m_Buyer) SCR_EntityHelper.DeleteEntityAndChildren(m_Buyer);
		m_Shop = null;
		m_Buyer = null;
	}
}

[Test(suite: AX_ShopSystemProbeSuite, timeoutS: 10)]
class AX_ShopProbeTest_Resources : AX_ShopProbeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		array<ResourceName> paths = {
			"{4E6001DE6EAF4710}Prefabs/AX_ShopProbeBuyer.et",
			"{DF4CC8BEDB0858A0}UI/Layouts/Menus/ShopMenu/ShopMain.layout",
			"{870C775F6E4107A1}UI/Layouts/Menus/ViewPayment/ViewPayment.layout",
			"{4E6001DE6EAF4601}ThirdParty/RPPhone/UI/AX_Phone.layout"
		};
		foreach (ResourceName path : paths)
		{
			Resource resource = Resource.Load(path);
			Expect(resource && resource.IsValid(), "Optional resource must resolve: " + path);
		}
	}
}

[Test(suite: AX_ShopSystemProbeSuite, timeoutS: 10)]
class AX_ShopProbeTest_NpcCatalog : AX_ShopProbeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		Expect(SpawnBuyer(), "Character prefab must spawn with the derived shop component.");
		if (!m_Shop) return;
		Expect(ChimeraCharacter.Cast(m_Buyer) != null, "Buyer must be an actual character.");
		ActionsManagerComponent actions = ActionsManagerComponent.Cast(m_Buyer.FindComponent(ActionsManagerComponent));
		Expect(actions != null, "Buyer must have an interaction manager.");
		if (!actions) return;
		array<BaseUserAction> registered = {};
		actions.GetActionsList(registered);
		AX_ShopProbeAction opener;
		foreach (BaseUserAction action : registered)
		{
			AX_ShopProbeAction candidate = AX_ShopProbeAction.Cast(action);
			if (candidate) opener = candidate;
		}
		Expect(actions.GetContext("axum-shop-probe") != null && opener != null, "Sell-only context and custom opener must register on the character.");
		if (opener) Expect(opener.CanBeShownScript(m_Buyer), "Sell-only opener must initialize and accept a catalog with no buy entries.");
		Expect(m_Shop.GetMerchandiseBuy().IsEmpty() && m_Shop.GetMerchandiseSell().Count() == 1, "Buyer must be sell-only with one fixture item.");
		Expect(m_Shop.GetMerchandiseSell()[0].GetType().GetMaxSellQuantity() == 1, "Fixture must retain its quantity limit.");
	}
}

[Test(suite: AX_ShopSystemProbeSuite, timeoutS: 10)]
class AX_ShopProbeTest_MenuBinding : AX_ShopProbeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		Expect(SpawnBuyer(), "Buyer must spawn before menu binding.");
		if (!m_Shop) return;
		MenuManager menus = GetGame().GetMenuManager();
		ADM_ShopUI stock = ADM_ShopUI.Cast(menus.OpenMenu(ChimeraMenuPreset.ADM_ShopMenu));
		Expect(stock != null, "Stock shop preset must remain registered with the phone.");
		menus.CloseMenuByPreset(ChimeraMenuPreset.ADM_ShopMenu);
		AX_ShopProbeMenu preview = AX_ShopProbeMenu.Cast(menus.OpenMenu(ChimeraMenuPreset.AX_ShopProbeMenu));
		Expect(preview != null, "Probe preset must instantiate the derived shop UI.");
		if (!preview) return;
		preview.SetShop(m_Shop);
		Expect(preview.HasBoundShop() && preview.HasShopControls(), "Shop UI must bind the spawned character's shop and build both tabs.");
		preview.TryPreviewCheckout();
		Expect(preview.WasCheckoutBlocked(), "Preview checkout must stay inside the blocking override.");
	}
}

[Test(suite: AX_ShopSystemProbeSuite, timeoutS: 10)]
class AX_ShopProbeTest_TransfersBlocked : AX_ShopProbeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		Expect(SpawnBuyer(), "Buyer must spawn for dispatch checks.");
		if (!m_Shop) return;
		ADM_ShopBaseComponent shop = m_Shop;
		ADM_ShopMerchandise item = m_Shop.GetMerchandiseSell()[0];
		// Null actors deliberately exercise virtual dispatch: the unsafe base
		// implementations would dereference them. Never call super here.
		Expect(!shop.AskSell(null, null, item, 1), "Base-typed sell dispatch must reject before stock transfer code.");
		Expect(!shop.AskPurchase(null, null, item, 1), "Base-typed purchase dispatch must reject before stock transfer code.");
		Expect(!shop.AskSell(null, null, item, -1), "Malformed quantities must also remain blocked.");
	}
}
