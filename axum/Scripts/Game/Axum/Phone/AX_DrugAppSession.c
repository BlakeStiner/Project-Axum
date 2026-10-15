// Phone application state. Uses the existing order service; never moves goods
// or money. The initial game adapter exposes this only in single-player Workbench.
class AX_PhoneContact
{
	string m_sId;
	string m_sName;
	int m_iUnitPrice;
	int m_iMaxUnits;
	ref array<string> m_aMessages = {};
}

class AX_DrugAppSession
{
	protected ref AX_TradeService m_Service;
	protected ref array<ref AX_PhoneContact> m_aContacts = {};
	protected string m_sCharacterId;
	protected string m_sOrderId;
	protected int m_iOfferContact = -1;
	protected int m_iOfferUnits;
	protected int m_iOfferUntil;
	protected string m_sNotice;

	void AX_DrugAppSession(string sessionId, string characterId, AX_TradeEnvironment environment)
	{
		m_sCharacterId = characterId;
		m_Service = new AX_TradeService(sessionId, environment, new AX_TradeSettlementPort());
		AddContact("eli", "Eli", 100, 5, 20, 2000);
		AddContact("morgan", "Morgan", 125, 8, 24, 3000);
		AddSite("orchard-lane", "orchard", "118 0 144");
		AddSite("east-turnout", "east", "161 0 113");
		AddSite("old-yard", "yard", "90 0 170");
	}

	protected void AddContact(string id, string name, int price, int maxUnits, int demand, int budget)
	{
		AX_TradeContactDefinition definition = new AX_TradeContactDefinition();
		definition.m_sId = id;
		definition.m_sProductId = "sealed-test-package";
		definition.m_iUnitPrice = price;
		definition.m_iMaxOrderUnits = maxUnits;
		definition.m_iDemandUnits = demand;
		definition.m_iBudget = budget;
		m_Service.AddContact(definition);
		AX_PhoneContact contact = new AX_PhoneContact();
		contact.m_sId = id;
		contact.m_sName = name;
		contact.m_iUnitPrice = price;
		contact.m_iMaxUnits = maxUnits;
		contact.m_aMessages.Insert(name + "\nText me when you have sealed packages to move.");
		m_aContacts.Insert(contact);
	}

	protected void AddSite(string id, string region, vector position)
	{
		AX_TradeSiteDefinition site = new AX_TradeSiteDefinition();
		site.m_sId = id;
		site.m_sRegionId = region;
		site.m_vPosition = position;
		site.m_fInteractionRadius = 4;
		m_Service.AddSite(site);
	}

	int GetContactCount() { return m_aContacts.Count(); }
	string GetNotice() { return m_sNotice; }
	bool HasContact(int index) { return index >= 0 && index < m_aContacts.Count(); }
	string GetContactName(int index)
	{
		if (!HasContact(index)) return string.Empty;
		return m_aContacts[index].m_sName;
	}
	int GetMaxUnits(int index)
	{
		if (!HasContact(index)) return 0;
		return m_aContacts[index].m_iMaxUnits;
	}
	int GetUnitPrice(int index)
	{
		if (!HasContact(index)) return 0;
		return m_aContacts[index].m_iUnitPrice;
	}
	int GetAvailableUnits(int index)
	{
		if (!HasContact(index)) return -1;
		return m_Service.GetAvailableUnits(m_aContacts[index].m_sId);
	}
	string GetConversation(int index)
	{
		if (!HasContact(index)) return string.Empty;
		string result;
		foreach (string line : m_aContacts[index].m_aMessages)
		{
			if (!result.IsEmpty()) result += "\n\n";
			result += line;
		}
		return result;
	}
	protected void Message(int index, string text)
	{
		if (!HasContact(index)) return;
		array<string> messages = m_aContacts[index].m_aMessages;
		while (messages.Count() >= 8) messages.RemoveOrdered(0);
		messages.Insert(text);
	}
	void ClearOffer() { m_iOfferContact = -1; }
	bool HasOffer(int index, int now)
	{
		return m_iOfferContact == index && now < m_iOfferUntil;
	}
	int GetOfferTotal() { return m_iOfferUnits * GetUnitPrice(m_iOfferContact); }

	AX_ETradeResult RequestOffer(int index, int units, int now)
	{
		ClearOffer();
		if (!HasContact(index) || units <= 0 || units > GetMaxUnits(index) || !m_Service.Advance(now))
			return Fail(AX_ETradeResult.INVALID_REQUEST, "Choose a valid package quantity.");
		AX_TradeOrder current = GetOrder(now);
		if (current && current.m_eState != AX_ETradeState.CANCELLED && current.m_eState != AX_ETradeState.EXPIRED && current.m_eState != AX_ETradeState.COMPLETED)
			return Fail(AX_ETradeResult.UNAVAILABLE, "You already have a meeting booked.");
		AX_PhoneContact contact = m_aContacts[index];
		Message(index, "You\nLooking to move " + units.ToString() + " sealed packages. Interested?");
		if (units > m_Service.GetAvailableUnits(contact.m_sId) || units * contact.m_iUnitPrice > m_Service.GetAvailableBudget(contact.m_sId))
		{
			Message(index, contact.m_sName + "\nI cannot take that order right now.");
			return Fail(AX_ETradeResult.UNAVAILABLE, "This contact cannot take that quantity right now.");
		}
		if (now > int.MAX - 60) return Fail(AX_ETradeResult.INVALID_REQUEST, "Offer unavailable.");
		m_iOfferContact = index;
		m_iOfferUnits = units;
		m_iOfferUntil = now + 60;
		Message(index, contact.m_sName + "\nI can take " + units.ToString() + " for $" + GetOfferTotal().ToString() + ". Confirm within a minute and I will send the meeting details.");
		m_sNotice = "Offer received. Confirm to reserve the order.";
		return AX_ETradeResult.OK;
	}

	AX_ETradeResult AcceptOffer(int now, int roll)
	{
		if (m_iOfferContact < 0) return Fail(AX_ETradeResult.INVALID_STATE, "Request an offer first.");
		int index = m_iOfferContact;
		int units = m_iOfferUnits;
		bool expired = now >= m_iOfferUntil;
		ClearOffer();
		if (expired) return Fail(AX_ETradeResult.DEADLINE_PASSED, "That offer expired. Request a new one.");
		string id;
		AX_ETradeResult result = m_Service.Book(m_sCharacterId, m_aContacts[index].m_sId, units, now, roll, id);
		if (result != AX_ETradeResult.OK)
			return Fail(result, "No meeting available yet. Give it a moment and request another offer.");
		m_sOrderId = id;
		AX_TradeOrder order = GetOrder(now);
		Message(index, "You\nAgreed. Send me the meeting details.");
		Message(index, m_aContacts[index].m_sName + "\nReserved. Meet at " + GetSiteName(order.m_sSiteId) + ". You have ten minutes.");
		m_sNotice = "Meeting reserved. Details are below.";
		return AX_ETradeResult.OK;
	}

	AX_ETradeResult CancelOrder(int now)
	{
		AX_TradeOrder order = GetOrder(now);
		if (!order) return Fail(AX_ETradeResult.NOT_FOUND, "No meeting to cancel.");
		if (order.m_eState == AX_ETradeState.CANCELLED)
		{
			m_sNotice = "Meeting cancelled.";
			return AX_ETradeResult.OK;
		}
		AX_ETradeResult result = m_Service.Cancel(order.m_sId, m_sCharacterId, now);
		if (result != AX_ETradeResult.OK) return Fail(result, "This meeting can no longer be cancelled.");
		for (int i = 0; i < m_aContacts.Count(); i++)
		{
			if (m_aContacts[i].m_sId == order.m_sContactId)
				Message(i, "You\nCancel this meeting.\n\n" + m_aContacts[i].m_sName + "\nUnderstood. We will arrange another location next time.");
		}
		m_sNotice = "Meeting cancelled.";
		return result;
	}

	AX_TradeOrder GetOrder(int now)
	{
		m_Service.Advance(now);
		if (m_sOrderId.IsEmpty()) return null;
		return m_Service.GetOrder(m_sOrderId, m_sCharacterId);
	}
	static string GetSiteName(string id)
	{
		if (id == "orchard-lane") return "Orchard lane";
		if (id == "east-turnout") return "East turnout";
		if (id == "old-yard") return "Old yard";
		return id;
	}
	protected AX_ETradeResult Fail(AX_ETradeResult result, string notice)
	{
		m_sNotice = notice;
		return result;
	}
}
