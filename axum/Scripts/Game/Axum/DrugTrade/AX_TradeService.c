// Session-only foundation. Not registered globally, exposed by RPC, or enabled
// in production. Durable storage + authenticated transport are separate gates.
class AX_TradeService
{
	protected ref map<string, ref AX_TradeContactState> m_mContacts = new map<string, ref AX_TradeContactState>();
	protected ref array<ref AX_TradeSiteDefinition> m_aSites = {};
	protected ref map<string, ref AX_TradeOrder> m_mOrders = new map<string, ref AX_TradeOrder>();
	protected ref map<string, string> m_mActiveCharacters = new map<string, string>();
	protected ref map<string, string> m_mReservedRegions = new map<string, string>();
	protected ref map<string, int> m_mRegionReadyAt = new map<string, int>();
	protected ref map<string, int> m_mCharacterReadyAt = new map<string, int>();
	protected ref AX_TradeEnvironment m_Environment;
	protected ref AX_TradeSettlementPort m_Settlement;
	protected string m_sSessionId;
	protected int m_iSequence;
	protected int m_iLastTime;
	protected int m_iAppointmentSeconds = 600;
	protected int m_iRegionCooldownSeconds = 300;
	protected int m_iRequestCooldownSeconds = 60;

	void AX_TradeService(string sessionId, AX_TradeEnvironment environment, AX_TradeSettlementPort settlement)
	{
		m_sSessionId = sessionId;
		m_Environment = environment;
		m_Settlement = settlement;
	}

	bool AddContact(AX_TradeContactDefinition definition)
	{
		if (!definition || definition.m_sId.IsEmpty() || definition.m_sProductId.IsEmpty()) return false;
		if (m_mContacts.Contains(definition.m_sId)) return false;
		if (definition.m_iUnitPrice <= 0 || definition.m_iMaxOrderUnits <= 0) return false;
		if (definition.m_iDemandUnits < 0 || definition.m_iBudget < 0) return false;
		if (definition.m_iMaxOrderUnits > int.MAX / definition.m_iUnitPrice) return false;

		AX_TradeContactState state = new AX_TradeContactState();
		// Copy configuration: changing a source definition cannot change a quote.
		state.m_Definition = new AX_TradeContactDefinition();
		state.m_Definition.m_sId = definition.m_sId;
		state.m_Definition.m_sProductId = definition.m_sProductId;
		state.m_Definition.m_iUnitPrice = definition.m_iUnitPrice;
		state.m_Definition.m_iMaxOrderUnits = definition.m_iMaxOrderUnits;
		state.m_iAvailableUnits = definition.m_iDemandUnits;
		state.m_iAvailableBudget = definition.m_iBudget;
		m_mContacts.Set(definition.m_sId, state);
		return true;
	}

	bool AddSite(AX_TradeSiteDefinition definition)
	{
		if (!definition || definition.m_sId.IsEmpty() || definition.m_sRegionId.IsEmpty()) return false;
		if (definition.m_fInteractionRadius <= 0 || definition.m_fInteractionRadius > 10) return false;
		foreach (AX_TradeSiteDefinition existing : m_aSites)
		{
			if (existing.m_sId == definition.m_sId) return false;
		}
		AX_TradeSiteDefinition site = new AX_TradeSiteDefinition();
		site.m_sId = definition.m_sId;
		site.m_sRegionId = definition.m_sRegionId;
		site.m_vPosition = definition.m_vPosition;
		site.m_fInteractionRadius = definition.m_fInteractionRadius;
		m_aSites.Insert(site);
		return true;
	}

	// now is monotonic server time in seconds; roll is server RNG output, not an RPC input.
	AX_ETradeResult Book(string characterId, string contactId, int units, int now, int roll, out string orderId)
	{
		orderId = string.Empty;
		if (m_sSessionId.IsEmpty() || !m_Environment || !m_Settlement) return AX_ETradeResult.INVALID_REQUEST;
		if (characterId.IsEmpty() || units <= 0 || roll < 0 || !Advance(now)) return AX_ETradeResult.INVALID_REQUEST;
		if (now > int.MAX - m_iAppointmentSeconds || m_iSequence == int.MAX) return AX_ETradeResult.INVALID_REQUEST;
		// Terminal receipts stay available for session idempotency; bound memory
		// until the persistent order repository replaces this in-memory store.
		if (m_mOrders.Count() >= 4096) return AX_ETradeResult.UNAVAILABLE;
		if (m_mActiveCharacters.Contains(characterId)) return AX_ETradeResult.UNAVAILABLE;
		int characterReady;
		if (m_mCharacterReadyAt.Find(characterId, characterReady) && now < characterReady) return AX_ETradeResult.UNAVAILABLE;
		AX_TradeContactState contact = m_mContacts.Get(contactId);
		if (!contact) return AX_ETradeResult.NOT_FOUND;
		if (units > contact.m_Definition.m_iMaxOrderUnits) return AX_ETradeResult.INVALID_REQUEST;
		int price = units * contact.m_Definition.m_iUnitPrice;
		if (units > contact.m_iAvailableUnits || price > contact.m_iAvailableBudget) return AX_ETradeResult.UNAVAILABLE;

		array<AX_TradeSiteDefinition> eligible = {};
		array<string> regions = {};
		foreach (AX_TradeSiteDefinition site : m_aSites)
		{
			if (m_mReservedRegions.Contains(site.m_sRegionId)) continue;
			int regionReady;
			if (m_mRegionReadyAt.Find(site.m_sRegionId, regionReady) && now < regionReady) continue;
			if (!m_Environment.IsSiteUsable(site)) continue;
			eligible.Insert(site);
			if (!regions.Contains(site.m_sRegionId)) regions.Insert(site.m_sRegionId);
		}
		if (eligible.IsEmpty()) return AX_ETradeResult.UNAVAILABLE;
		// Select a region first so dense clusters of authored sites do not bias
		// appointments toward one road or neighborhood.
		string chosenRegion = regions[roll % regions.Count()];
		array<AX_TradeSiteDefinition> regionalSites = {};
		foreach (AX_TradeSiteDefinition candidate : eligible)
		{
			if (candidate.m_sRegionId == chosenRegion) regionalSites.Insert(candidate);
		}
		int siteRoll = roll / regions.Count();
		AX_TradeSiteDefinition chosen = regionalSites[siteRoll % regionalSites.Count()];
		AX_TradeOrder order = new AX_TradeOrder();
		m_iSequence++;
		order.m_sId = m_sSessionId + ":" + m_iSequence.ToString();
		order.m_sCharacterId = characterId;
		order.m_sContactId = contactId;
		order.m_sProductId = contact.m_Definition.m_sProductId;
		order.m_sSiteId = chosen.m_sId;
		order.m_sRegionId = chosen.m_sRegionId;
		order.m_vPosition = chosen.m_vPosition;
		order.m_fInteractionRadius = chosen.m_fInteractionRadius;
		order.m_iUnits = units;
		order.m_iTotalPrice = price;
		order.m_iDeadline = now + m_iAppointmentSeconds;
		order.m_eState = AX_ETradeState.BOOKED;
		contact.m_iAvailableUnits -= units;
		contact.m_iAvailableBudget -= price;
		m_mOrders.Set(order.m_sId, order);
		m_mActiveCharacters.Set(characterId, order.m_sId);
		m_mReservedRegions.Set(order.m_sRegionId, order.m_sId);
		m_mRegionReadyAt.Set(order.m_sRegionId, now + m_iRegionCooldownSeconds);
		m_mCharacterReadyAt.Set(characterId, now + m_iRequestCooldownSeconds);
		orderId = order.m_sId;
		return AX_ETradeResult.OK;
	}

	// Only the encounter adapter calls this after the correct NPC exists and is usable.
	AX_ETradeResult MarkBuyerReady(string orderId, int now)
	{
		if (!Advance(now)) return AX_ETradeResult.INVALID_REQUEST;
		AX_TradeOrder order = m_mOrders.Get(orderId);
		if (!order) return AX_ETradeResult.NOT_FOUND;
		if (order.m_eState == AX_ETradeState.EXPIRED) return AX_ETradeResult.DEADLINE_PASSED;
		if (order.m_eState != AX_ETradeState.BOOKED) return AX_ETradeResult.INVALID_STATE;
		order.m_eState = AX_ETradeState.READY;
		return AX_ETradeResult.OK;
	}

	AX_ETradeResult Complete(string orderId, string characterId, vector position, int now)
	{
		if (!Advance(now)) return AX_ETradeResult.INVALID_REQUEST;
		AX_TradeOrder order = m_mOrders.Get(orderId);
		if (!order) return AX_ETradeResult.NOT_FOUND;
		if (order.m_sCharacterId != characterId) return AX_ETradeResult.NOT_OWNER;
		if (order.m_eState == AX_ETradeState.COMPLETED) return AX_ETradeResult.OK;
		if (order.m_eState == AX_ETradeState.RECOVERY_REQUIRED) return AX_ETradeResult.RECOVERY_REQUIRED;
		if (order.m_eState == AX_ETradeState.EXPIRED) return AX_ETradeResult.DEADLINE_PASSED;
		if (order.m_eState != AX_ETradeState.READY) return AX_ETradeResult.INVALID_STATE;
		if (vector.Distance(position, order.m_vPosition) > order.m_fInteractionRadius) return AX_ETradeResult.TOO_FAR;
		order.m_eState = AX_ETradeState.SETTLING;
		AX_ESettlementOutcome outcome = m_Settlement.Commit(order.Copy());
		if (outcome == AX_ESettlementOutcome.COMMITTED)
		{
			order.m_eState = AX_ETradeState.COMPLETED;
			ReleaseLocks(order);
			return AX_ETradeResult.OK;
		}
		if (outcome == AX_ESettlementOutcome.REJECTED)
		{
			order.m_eState = AX_ETradeState.READY;
			return AX_ETradeResult.SETTLEMENT_REJECTED;
		}
		order.m_eState = AX_ETradeState.RECOVERY_REQUIRED;
		return AX_ETradeResult.RECOVERY_REQUIRED;
	}

	AX_ETradeResult Cancel(string orderId, string characterId, int now)
	{
		if (!Advance(now)) return AX_ETradeResult.INVALID_REQUEST;
		AX_TradeOrder order = m_mOrders.Get(orderId);
		if (!order) return AX_ETradeResult.NOT_FOUND;
		if (order.m_sCharacterId != characterId) return AX_ETradeResult.NOT_OWNER;
		if (order.m_eState == AX_ETradeState.CANCELLED) return AX_ETradeResult.OK;
		if (order.m_eState != AX_ETradeState.BOOKED && order.m_eState != AX_ETradeState.READY) return AX_ETradeResult.INVALID_STATE;
		RefundReservation(order, AX_ETradeState.CANCELLED);
		return AX_ETradeResult.OK;
	}

	// Infrastructure tick, including when a seller is disconnected.
	bool Advance(int now)
	{
		if (now < 0 || now < m_iLastTime) return false;
		m_iLastTime = now;
		foreach (string id, AX_TradeOrder order : m_mOrders)
		{
			if (order.m_eState != AX_ETradeState.BOOKED && order.m_eState != AX_ETradeState.READY) continue;
			if (now >= order.m_iDeadline) RefundReservation(order, AX_ETradeState.EXPIRED);
		}
		return true;
	}

	AX_TradeOrder GetOrder(string orderId, string characterId)
	{
		AX_TradeOrder order = m_mOrders.Get(orderId);
		if (!order || order.m_sCharacterId != characterId) return null;
		return order.Copy();
	}

	int GetAvailableUnits(string contactId)
	{
		AX_TradeContactState contact = m_mContacts.Get(contactId);
		if (!contact) return -1;
		return contact.m_iAvailableUnits;
	}

	int GetAvailableBudget(string contactId)
	{
		AX_TradeContactState contact = m_mContacts.Get(contactId);
		if (!contact) return -1;
		return contact.m_iAvailableBudget;
	}

	protected void RefundReservation(AX_TradeOrder order, AX_ETradeState state)
	{
		AX_TradeContactState contact = m_mContacts.Get(order.m_sContactId);
		contact.m_iAvailableUnits += order.m_iUnits;
		contact.m_iAvailableBudget += order.m_iTotalPrice;
		order.m_eState = state;
		ReleaseLocks(order);
	}

	protected void ReleaseLocks(AX_TradeOrder order)
	{
		m_mActiveCharacters.Remove(order.m_sCharacterId);
		m_mReservedRegions.Remove(order.m_sRegionId);
		int readyAt = int.MAX;
		if (m_iLastTime <= int.MAX - m_iRegionCooldownSeconds) readyAt = m_iLastTime + m_iRegionCooldownSeconds;
		m_mRegionReadyAt.Set(order.m_sRegionId, readyAt);
	}
}
