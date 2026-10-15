// Test-only ports. These classes are never loaded by the gameplay addon.
class AX_TestTradeEnvironment : AX_TradeEnvironment
{
	bool m_bUsable = true;
	override bool IsSiteUsable(AX_TradeSiteDefinition site) { return m_bUsable; }
}

class AX_TestTradeSettlement : AX_TradeSettlementPort
{
	int m_iCalls;
	AX_ESettlementOutcome m_eOutcome = AX_ESettlementOutcome.COMMITTED;
	AX_TradeService m_ReentrantService;
	AX_ETradeResult m_eReentrantResult;
	override AX_ESettlementOutcome Commit(AX_TradeOrder order)
	{
		m_iCalls++;
		if (m_ReentrantService) m_eReentrantResult = m_ReentrantService.Complete(order.m_sId, order.m_sCharacterId, order.m_vPosition, 2);
		return m_eOutcome;
	}
}

[BaseContainerProps(category: "Autotest")]
class AX_TradeCoreSuite : SCR_AutotestSuiteBase
{
	// Pure domain tests do not load the client-oriented default MpTest world.
	override ResourceName GetWorldFile() { return ResourceName.Empty; }
}

class AX_TradeTestCase : SCR_AutotestCaseBase
{
	ref AX_TestTradeEnvironment m_Environment;
	ref AX_TestTradeSettlement m_Settlement;
	ref AX_TradeService m_Service;

	void CreateService()
	{
		m_Environment = new AX_TestTradeEnvironment();
		m_Settlement = new AX_TestTradeSettlement();
		m_Service = new AX_TradeService("test-session", m_Environment, m_Settlement);
		AX_TradeContactDefinition contact = new AX_TradeContactDefinition();
		contact.m_sId = "buyer";
		contact.m_sProductId = "sealed-test-package";
		contact.m_iUnitPrice = 100;
		contact.m_iMaxOrderUnits = 10;
		contact.m_iDemandUnits = 10;
		contact.m_iBudget = 1000;
		m_Service.AddContact(contact);
		AddSite("clearing-a", "north", "0 0 0");
		AddSite("clearing-b", "south", "100 0 0");
	}

	void AddSite(string id, string region, vector position)
	{
		AX_TradeSiteDefinition site = new AX_TradeSiteDefinition();
		site.m_sId = id;
		site.m_sRegionId = region;
		site.m_vPosition = position;
		site.m_fInteractionRadius = 4;
		m_Service.AddSite(site);
	}

	void Expect(bool condition, string message)
	{
		if (condition) return;
		SetFailure(SCR_AutotestFailure.Create(message));
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_OneSettlement : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		string id;
		bool ok = m_Service.Book("alice", "buyer", 4, 0, 0, id) == AX_ETradeResult.OK;
		ok = ok && m_Service.GetAvailableUnits("buyer") == 6;
		ok = ok && m_Service.MarkBuyerReady(id, 1) == AX_ETradeResult.OK;
		ok = ok && m_Service.Complete(id, "alice", "0 0 0", 2) == AX_ETradeResult.OK;
		ok = ok && m_Service.Complete(id, "alice", "0 0 0", 3) == AX_ETradeResult.OK;
		Expect(ok && m_Settlement.m_iCalls == 1 && m_Service.GetAvailableBudget("buyer") == 600, "A completed order must settle exactly once and consume its reserved demand/budget.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_CancelAndCooldown : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		string id, second;
		m_Service.Book("alice", "buyer", 4, 0, 0, id);
		m_Service.Cancel(id, "alice", 1);
		m_Service.Cancel(id, "alice", 2);
		bool ok = m_Service.GetAvailableUnits("buyer") == 10 && m_Service.GetAvailableBudget("buyer") == 1000;
		ok = ok && m_Service.Book("alice", "buyer", 4, 3, 0, second) == AX_ETradeResult.UNAVAILABLE;
		ok = ok && m_Service.Book("alice", "buyer", 4, 60, 0, second) == AX_ETradeResult.OK;
		Expect(ok && m_Service.GetOrder(second, "alice").m_sSiteId == "clearing-b", "Cancellation must refund once, retain request cooldown, and rotate away from the recent region.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_OwnershipAndDistance : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		string id;
		m_Service.Book("alice", "buyer", 4, 0, 0, id);
		m_Service.MarkBuyerReady(id, 1);
		bool ok = m_Service.Complete(id, "bob", "0 0 0", 2) == AX_ETradeResult.NOT_OWNER;
		ok = ok && m_Service.Complete(id, "alice", "20 0 0", 3) == AX_ETradeResult.TOO_FAR;
		ok = ok && !m_Service.GetOrder(id, "bob");
		Expect(ok && m_Settlement.m_iCalls == 0, "Wrong owners and distant callers must never reach settlement or read private appointments.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_Expiry : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		string id;
		m_Service.Book("alice", "buyer", 4, 0, 0, id);
		m_Service.MarkBuyerReady(id, 1);
		m_Service.Advance(600);
		m_Service.Advance(601);
		bool ok = m_Service.Complete(id, "alice", "0 0 0", 601) == AX_ETradeResult.DEADLINE_PASSED;
		Expect(ok && m_Service.GetAvailableBudget("buyer") == 1000 && m_Service.GetAvailableUnits("buyer") == 10 && m_Settlement.m_iCalls == 0, "Expiry must refund unused reservations once and never pay a late order.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_UnknownSettlement : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		m_Settlement.m_eOutcome = AX_ESettlementOutcome.UNKNOWN;
		string id, second;
		m_Service.Book("alice", "buyer", 4, 0, 0, id);
		m_Service.MarkBuyerReady(id, 1);
		bool ok = m_Service.Complete(id, "alice", "0 0 0", 2) == AX_ETradeResult.RECOVERY_REQUIRED;
		ok = ok && m_Service.Complete(id, "alice", "0 0 0", 3) == AX_ETradeResult.RECOVERY_REQUIRED;
		ok = ok && m_Service.Cancel(id, "alice", 4) == AX_ETradeResult.INVALID_STATE;
		m_Service.Advance(900);
		ok = ok && m_Service.Book("alice", "buyer", 1, 901, 0, second) == AX_ETradeResult.UNAVAILABLE;
		Expect(ok && m_Settlement.m_iCalls == 1 && m_Service.GetAvailableBudget("buyer") == 600, "An ambiguous settlement must retain reservations and block retries, cancellation, and expiry refunds.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_NoSuitableSite : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		m_Environment.m_bUsable = false;
		string id;
		bool ok = m_Service.Book("alice", "buyer", 4, 0, 0, id) == AX_ETradeResult.UNAVAILABLE;
		Expect(ok && id.IsEmpty() && m_Service.GetAvailableUnits("buyer") == 10 && m_Service.GetAvailableBudget("buyer") == 1000, "No usable site must decline without consuming demand or silently falling back to a fixed dealer.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_DemandAndReservations : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		string first, second, third;
		bool ok = m_Service.Book("alice", "buyer", 7, 0, 0, first) == AX_ETradeResult.OK;
		ok = ok && m_Service.Book("bob", "buyer", 4, 1, 0, second) == AX_ETradeResult.UNAVAILABLE;
		ok = ok && m_Service.Book("bob", "buyer", 3, 2, 0, second) == AX_ETradeResult.OK;
		ok = ok && m_Service.GetOrder(first, "alice").m_sRegionId != m_Service.GetOrder(second, "bob").m_sRegionId;
		ok = ok && m_Service.Book("charlie", "buyer", 1, 3, 0, third) == AX_ETradeResult.UNAVAILABLE;
		Expect(ok && m_Service.GetAvailableBudget("buyer") == 0, "Concurrent appointments cannot oversubscribe a buyer or reserve the same region.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_RejectedSettlement : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		m_Settlement.m_eOutcome = AX_ESettlementOutcome.REJECTED;
		string id;
		m_Service.Book("alice", "buyer", 4, 0, 0, id);
		m_Service.MarkBuyerReady(id, 1);
		bool ok = m_Service.Complete(id, "alice", "0 0 0", 2) == AX_ETradeResult.SETTLEMENT_REJECTED;
		ok = ok && m_Service.GetOrder(id, "alice").m_eState == AX_ETradeState.READY;
		m_Settlement.m_eOutcome = AX_ESettlementOutcome.COMMITTED;
		ok = ok && m_Service.Complete(id, "alice", "0 0 0", 3) == AX_ETradeResult.OK;
		Expect(ok && m_Settlement.m_iCalls == 2 && m_Service.GetAvailableBudget("buyer") == 600, "A definite inventory/capacity rejection permits a corrected retry without a second reservation.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_InvalidAndCopiedState : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		string id;
		bool ok = m_Service.Book("alice", "buyer", -1, 0, 0, id) == AX_ETradeResult.INVALID_REQUEST;
		ok = ok && m_Service.Book("alice", "buyer", 11, 0, 0, id) == AX_ETradeResult.INVALID_REQUEST;
		ok = ok && m_Service.Book("alice", "buyer", 4, 10, 0, id) == AX_ETradeResult.OK;
		AX_TradeOrder copy = m_Service.GetOrder(id, "alice");
		copy.m_iTotalPrice = 999999;
		ok = ok && m_Service.GetOrder(id, "alice").m_iTotalPrice == 400;
		ok = ok && !m_Service.Advance(9);
		Expect(ok, "Invalid quantities, backwards clocks, and caller edits must not change server-owned terms.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_RegionSelection : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		AddSite("clearing-c", "north", "10 0 0");
		AddSite("clearing-d", "north", "20 0 0");
		string id;
		bool ok = m_Service.Book("alice", "buyer", 1, 0, 1, id) == AX_ETradeResult.OK;
		Expect(ok && m_Service.GetOrder(id, "alice").m_sRegionId == "south", "A region with more sites must not receive extra region-selection slots.");
	}
}

[Test(suite: AX_TradeCoreSuite, timeoutS: 5)]
class AX_TradeTest_ReentrantSettlement : AX_TradeTestCase
{
	[TestStep(TestStage.Main)]
	void Execute()
	{
		CreateService();
		m_Settlement.m_ReentrantService = m_Service;
		string id;
		m_Service.Book("alice", "buyer", 1, 0, 0, id);
		bool ok = m_Service.Complete(id, "alice", "0 0 0", 1) == AX_ETradeResult.INVALID_STATE;
		m_Service.MarkBuyerReady(id, 1);
		ok = ok && m_Service.Complete(id, "alice", "0 0 0", 2) == AX_ETradeResult.OK;
		Expect(ok && m_Settlement.m_iCalls == 1 && m_Settlement.m_eReentrantResult == AX_ETradeResult.INVALID_STATE, "Settlement must lock the order before a callback can submit another completion.");
	}
}
