#pragma once

class Player;

class Party
{
public:
	explicit Party(Player* owner) {
		setHost(owner);
	}

	Player* getHost() const {
		return host;
	}
	void setHost(Player* new_host) {
		host = new_host;
	}

	const std::vector<Player*>& getMembers() const {
		return members;
	}

	bool isEmpty() const {
		return members.size() == 1 && invited_players.empty();
	}
	bool isInvited(const Player* player);
	bool isMember(const Player* player);

	void addInvitation(Player* player);
	void removeInvitation(Player* player);

	void addMember(Player* player);
	void removeMember(Player* player);

	void passLeadership(Player* player);
	void joinParty(Player* player);
	void leaveParty(Player* player);

	void disband();
private:
	Player* host = nullptr;

	std::vector<Player*> invited_players{};
	std::vector<Player*> members{};
};