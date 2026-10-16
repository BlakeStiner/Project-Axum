// Isolated compatibility addon. Never loaded by ProjectAxum.gproj.
// Original Axum adapters; Shop System remains an external APL dependency.
class AX_ShopProbeBuyerComponentClass : ADM_ShopComponentClass {}
class AX_ShopProbeBuyerComponent : ADM_ShopComponent
{
	override bool AskPurchase(IEntity player, ADM_PlayerShopManagerComponent playerManager, ADM_ShopMerchandise merchandise, int quantity)
	{
		if (playerManager) playerManager.SetTransactionMessage("Compatibility preview: purchases disabled.");
		return false;
	}

	override bool AskSell(IEntity player, ADM_PlayerShopManagerComponent playerManager, ADM_ShopMerchandise merchandise, int quantity)
	{
		if (playerManager) playerManager.SetTransactionMessage("Compatibility preview: sales disabled.");
		return false;
	}
}

modded enum ChimeraMenuPreset
{
	AX_ShopProbeMenu
}

// This UI is deliberately a preview, not an authenticated appointment adapter.
// Display the fixture even without inventory and never invoke stock checkout.
class AX_ShopProbeMenu : ADM_ShopUI
{
	protected bool m_bCheckoutBlocked;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		// The installed layout's inherited heading is absent on 1.8.0.13.
		if (!m_wHeaderText)
		{
			Widget heading = GetGame().GetWorkspace().CreateWidgets("{4E6001DE6EAF4711}UI/AX_ShopProbeHeading.layout", w);
			if (heading) m_wHeaderText = TextWidget.Cast(heading.FindAnyWidget("AX_ProbeHeading"));
		}
	}

	override protected void SelectCategorySell(string search = "")
	{
		// The diagnostic server has no graphical item-preview service.
		// Bind tabs there; exercise actual item rows in the graphical client.
		if (m_Shop && m_wSellTabView && m_ItemPreviewManager)
			PopulateTab(m_wSellTabView, m_Shop.GetMerchandiseSell(), false);
	}

	override protected void Checkout(SCR_InputButtonComponent btn)
	{
		m_bCheckoutBlocked = true;
		if (m_wHeaderText) m_wHeaderText.SetText("PREVIEW: no goods or money exchanged");
	}

	bool HasBoundShop() { return m_Shop != null; }
	bool HasShopControls()
	{
		return m_wBuyTabView && m_wSellTabView && m_wHeaderText;
	}
	bool WasCheckoutBlocked() { return m_bCheckoutBlocked; }
	void TryPreviewCheckout() { Checkout(null); }
}

// A sell-only opener must not use ADM_ShopAction's buy-catalog gate.
class AX_ShopProbeAction : ScriptedUserAction
{
	protected AX_ShopProbeBuyerComponent m_Shop;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Shop = AX_ShopProbeBuyerComponent.Cast(pOwnerEntity.FindComponent(AX_ShopProbeBuyerComponent));
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_Shop && m_Shop.GetMerchandiseSell().Count() > 0;
	}

	override bool CanBePerformedScript(IEntity user) { return CanBeShownScript(user); }
	override bool GetActionNameScript(out string outName) { outName = "Preview buyer shop (no transfers)"; return true; }
	override bool HasLocalEffectOnlyScript() { return true; }
	override bool CanBroadcastScript() { return false; }

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!m_Shop || !controller || controller.GetControlledEntity() != pUserEntity) return;
		AX_ShopProbeMenu menu = AX_ShopProbeMenu.Cast(GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.AX_ShopProbeMenu));
		if (menu) menu.SetShop(m_Shop);
	}
}
