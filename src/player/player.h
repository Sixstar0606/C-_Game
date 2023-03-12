#pragma once
#include <array>
#include <enet/enet.h>
#include <player/player_component.h>
#include <player/objects/enums.h>
#include <player/objects/roles.h>
#include <player/objects/character_state.h>
#include <player/objects/inventory.h>
#include <player/objects/login_information.h>
#include <player/objects/packet_sender.h>
#include <player/objects/playmod.h>
#include <player/objects/variantlist_sender.h>
#include <server/server.h>
#include <proton/packet.h>
#include <proton/utils/common.h>
#include <proton/utils/text_scanner.h>
#include <utils/timing_clock.h>

namespace GTServer {
    class World;
    class Player : public PacketSender, public PlayerComponent, public CharacterState {
    public:
        explicit Player(ENetPeer* peer);
        ~Player();

        bool IsFlagOn(const ePlayerFlags& flag) const;
        void SetFlag(const ePlayerFlags& flag);
        void RemoveFlag(const ePlayerFlags& flag);

        [[nodiscard]] ENetPeer* GetPeer() const { return m_peer; }
        [[nodiscard]] const char* GetIPAddress() const { return m_ip_address.data(); }
        void Disconnect(const enet_uint32& data) { enet_peer_disconnect_later(this->GetPeer(), data); }

        void SetUserId(const uint32_t& uid) { m_user_id = uid; }
        [[nodiscard]] uint32_t GetUserId() const { return m_user_id; }
        void SetConnectID(const uint32_t& cid) { m_connect_id = cid; }
        [[nodiscard]] uint32_t GetConnectID() const { return m_connect_id; }
        void SetRawName(const std::string& name) { m_raw_name = name; }
        [[nodiscard]] std::string GetRawName() const { return m_raw_name; }
        [[nodiscard]] std::string GetDisplayName(std::shared_ptr<World> world) const;
        void SetDisplayName(const std::string& name) { m_display_name = name; }
        [[nodiscard]] std::string GetDisplayName() const { return m_display_name; }
        
        void SetEmail(const std::string& email) { m_email = email; }
        [[nodiscard]] std::string GetEmail() const { return m_email; } 
        void SetDiscord(const uint64_t& discord) { m_discord = discord; }
        [[nodiscard]] uint64_t GetDiscord() const { return m_discord; }

        void SetRole(const uint32_t& role) { m_role = role; }
        [[nodiscard]] uint32_t GetRole() const { return m_role; }

        void SetWorld(const std::string& name) { m_world = name; }
        [[nodiscard]] std::string GetWorld() const { return m_world; }
        void SetNetId(const uint32_t& net_id) { m_net_id = net_id; }
        [[nodiscard]] uint32_t GetNetId() const { return m_net_id; }
        void SetPosition(const int& x, const int& y) { m_position = CL_Vec2i{ x, y }; }
        [[nodiscard]] CL_Vec2i GetPosition() const { return m_position; }

        void SetGems(const int32_t& gems) { m_gems = gems; }
        int32_t GetGems() const { return m_gems; }
        void SetBGTTokens(const int32_t& tokens) { m_bgt_tokens = tokens; }
        int32_t GetBGTTokens() const { return m_bgt_tokens; }
        void SetCloth(const uint8_t& body_part, const uint16_t& id, const bool& add_playmod = false);
        uint16_t& GetCloth(const uint8_t& body_part) { return m_clothes[body_part]; }
        std::array<uint16_t, NUM_BODY_PARTS>& GetClothes() { return m_clothes; }
        Color GetSkinColor() const { return m_skin_color; }
        void SetSkinColor(Color skin) { m_skin_color = skin; } // change skin
        std::vector<uint8_t> Pack(const ePlayerData& type) const;
        void Serialize(const ePlayerData& type, const std::vector<uint8_t>& data);
        
        TextScanner GetSpawnData(const bool& local = false) const;
        std::shared_ptr<LoginInformation> GetLoginDetail() { return m_login_info; }

        void SendCharacterState(std::shared_ptr<Player> player);
        void SendExperience(std::shared_ptr<World> world, const int32_t& amount);

        bool HasAccess(std::shared_ptr<World> world, const CL_Vec2i position, uint32_t item);
    
    public:
        bool RemoveItemSafe(const uint16_t& item_id, const uint8_t& count, const bool& send_packet = false);

    public:
        enum eDialogType {
            DIALOG_TYPE_REGISTRATION,
            DIALOG_TYPE_ACCOUNT_VERIFY,
            DIALOG_TYPE_NEWS,
            DIALOG_TYPE_POPUP, 
            DIALOG_TYPE_PUNISHMENT,
            DIALOG_TYPE_SOCIAL_PORTAL
        };
        void SendDialog(const eDialogType& type, TextScanner parser, std::shared_ptr<World> world = nullptr, std::shared_ptr<Player> target = nullptr);

    public:
        void AddPlaymod(ePlaymodType type, uint16_t icon_id, steady_clock::time_point apply_mod, std::chrono::seconds time);
        void RemovePlaymod(ePlaymodType type);
        void SetBan(std::string& reason);
        bool HasPlaymod(ePlaymodType type);
        bool GetPlaymod(ePlaymodType type, Playmod& val);
        uint8_t GetActivePunchID();
        std::vector<Playmod>& GetPlaymods() { return m_playmods; }

    public:
        std::shared_ptr<LoginInformation> m_login_info;
        
        Inventory m_inventory;
        VariantListSender v_sender;

    private:
        ENetPeer* m_peer;

        uint32_t m_flags{ 0 };
        uint32_t m_user_id{ 0 };
        uint32_t m_connect_id{ 0 };
        uint8_t m_role{ 0 };

        std::string m_raw_name{};
        std::string m_display_name{};
        std::string m_ip_address{};
        std::string m_ban_reason{};

        std::string m_email{};
        uint64_t m_discord{ 0 };

        std::string m_world{ "EXIT" };
        uint32_t m_net_id{ 0 };
        CL_Vec2i m_position{ 0, 0 };

        int32_t m_gems{ 0 };
        int32_t m_bgt_tokens = { 0 };
        std::array<uint16_t, NUM_BODY_PARTS> m_clothes{};
        Color m_skin_color = Color{ 0xB4, 0x8A, 0x78, 0xFF };

        std::vector<Playmod> m_playmods{};
    };
}