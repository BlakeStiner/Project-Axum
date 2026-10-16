// Project Axum: server-side appointment domain. No inventory or money mutations.
enum AX_ETradeState
{
	BOOKED,
	READY,
	SETTLING,
	COMPLETED,
	CANCELLED,
	EXPIRED,
	RECOVERY_REQUIRED
}

enum AX_ETradeResult
{
	OK,
	INVALID_REQUEST,
	NOT_FOUND,
	NOT_OWNER,
	UNAVAILABLE,
	INVALID_STATE,
	TOO_FAR,
	DEADLINE_PASSED,
	SETTLEMENT_REJECTED,
	RECOVERY_REQUIRED
}

enum AX_ESettlementOutcome
{
	REJECTED,
	COMMITTED,
	UNKNOWN
}

[BaseContainerProps()]
class AX_TradeContactDefinition
{
	[Attribute()] string m_sId;
	[Attribute()] string m_sProductId;
	[Attribute("100")] int m_iUnitPrice;
	[Attribute("10")] int m_iMaxOrderUnits;
	[Attribute("50")] int m_iDemandUnits;
	[Attribute("5000")] int m_iBudget;
}

[BaseContainerProps()]
class AX_TradeSiteDefinition
{
	[Attribute()] string m_sId;
	[Attribute()] string m_sRegionId;
	[Attribute("0 0 0")] vector m_vPosition;
	[Attribute("4")] float m_fInteractionRadius;
}

class AX_TradeContactState
{
	ref AX_TradeContactDefinition m_Definition;
	int m_iAvailableUnits;
	int m_iAvailableBudget;
}

// Returned by value-copy: callers cannot mutate the service's internal order.
class AX_TradeOrder
{
	string m_sId;
	string m_sCharacterId;
	string m_sContactId;
	string m_sProductId;
	string m_sSiteId;
	string m_sRegionId;
	vector m_vPosition;
	float m_fInteractionRadius;
	int m_iUnits;
	int m_iTotalPrice;
	int m_iDeadline;
	AX_ETradeState m_eState;

	AX_TradeOrder Copy()
	{
		AX_TradeOrder copy = new AX_TradeOrder();
		copy.m_sId = m_sId;
		copy.m_sCharacterId = m_sCharacterId;
		copy.m_sContactId = m_sContactId;
		copy.m_sProductId = m_sProductId;
		copy.m_sSiteId = m_sSiteId;
		copy.m_sRegionId = m_sRegionId;
		copy.m_vPosition = m_vPosition;
		copy.m_fInteractionRadius = m_fInteractionRadius;
		copy.m_iUnits = m_iUnits;
		copy.m_iTotalPrice = m_iTotalPrice;
		copy.m_iDeadline = m_iDeadline;
		copy.m_eState = m_eState;
		return copy;
	}
}

// Implementations belong on the server. A transport must resolve identity and
// position from the authenticated player, never forward client-supplied values.
class AX_TradeEnvironment
{
	bool IsSiteUsable(AX_TradeSiteDefinition site) { return false; }
}

// Commit MUST durably settle the operation ID exactly once before COMMITTED.
// UNKNOWN means a timeout/ambiguous result: do not release goods or pay again.
// The base implementation intentionally cannot move goods or grant money.
class AX_TradeSettlementPort
{
	AX_ESettlementOutcome Commit(AX_TradeOrder order) { return AX_ESettlementOutcome.REJECTED; }
}
