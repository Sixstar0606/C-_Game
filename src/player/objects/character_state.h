#pragma once
#include <array>
#include <string>
#include <utils/color.h>
#include <utils/binary_reader.h>
#include <utils/binary_writer.h>

namespace GTServer {
    enum eStateFlags {
        STATEFLAG_NOCLIP,
        STATEFLAG_DOUBLE_JUMP,
        STATEFLAG_INVISIBLE,
        STATEFLAG_NO_HAND,
        STATEFLAG_NO_EYE,
        STATEFLAG_NO_BODY,
        STATEFLAG_DEVIL_HORN,
        STATEFLAG_GOLDEN_HALO,
        STATEFLAG_UNKNOWN_8,
        STATEFLAG_UNKNOWN_9,
        STATEFLAG_UNKNOWN_10,
        STATEFLAG_FROZEN,
        STATEFLAG_CURSED,
        STATEFLAG_DUCT_TAPE,
        STATEFLAG_CIGAR,
        STATEFLAG_SHINING,
        STATEFLAG_ZOMBIE,
        STATEFLAG_RED_BODY,
        STATEFLAG_HAUNTED_SHADOWS,
        STATEFLAG_GEIGER_RADIATION,
        STATEFLAG_SPOTLIGHT,
        STATEFLAG_YELLOW_BODY,
        STATEFLAG_PINEAPPLE_FLAG,
        STATEFLAG_FLYING_PINEAPPLE,
        STATEFLAG_SUPER_SUPPORTER_NAME,
        STATEFLAG_SUPER_PINEAPPLE,
        NUM_STATE_FLAGS
    };
    class CharacterState {
    public:
        CharacterState() {
            for (uint8_t index = 0; index < NUM_STATE_FLAGS; index++)
                this->RemoveFlag((eStateFlags)index);
        }
        ~CharacterState() = default;

        bool IsFlagOn(const eStateFlags& flag) const {
            if (m_flags[flag])
                return true;
            return false;
        }
        void SetFlag(const eStateFlags& flag) { m_flags[flag] = true; }
        void RemoveFlag(const eStateFlags& flag) { m_flags[flag] = false; }

        uint8_t GetPunchID() const { return m_punch_id; }
        uint8_t GetPunchRange() const { return m_punch_range; }
        uint8_t GetBuildRange() const { return m_build_range; }
        float GetWaterSpeed() const { return m_water_speed; }
        float GetSpeed() const { return m_speed; }
        float GetGravity() const { return m_gravity; }
        float GetAcceleration() const { return m_acceleration; }
        float GetPunchStrength() const { return m_punch_strength; }
        uint32_t GetFlags() const {
            uint32_t flags = 0;
            for (auto index = 0; index < 26; index++)
                flags |= m_flags[index] << index;
            return flags;
        }
    
        Color GetPupilColor() const { return m_pupil_color; }
        Color GetHairColor() const { return m_hair_color; }
        Color GetEyesColor() const { return m_eyes_color; }
    
    public:
        void SetPunchID(uint8_t val) { m_punch_id = val; }
        void SetPunchRange(uint8_t val) { m_punch_range = val; }
        void SetBuildRange(uint8_t val) { m_build_range = val; }
        void SetWaterSpeed(float val) { m_water_speed = val; }
        void SetSpeed(float val) { m_speed = val; }
        void SetGravity(float val) { m_gravity = val; }
        void SetAcceleration(float val) { m_acceleration = val; }
        void SetPunchStrength(float val) { m_punch_strength = val; }
        void SetPupilColor(Color val) { m_pupil_color = val; }
        void SetHairColor(Color val) { m_hair_color = val; }
        void SetEyesColor(Color val) { m_eyes_color = val; }
        
    private:
        uint8_t m_punch_id = 0;
        uint8_t m_punch_range = 128;
        uint8_t m_build_range = 128;

        float m_water_speed = 150.0f;
        float m_speed = 260.0f;
        float m_gravity = 1000.0f;
        float m_acceleration = 1000.0f;
        float m_punch_strength = 350.0f;
        std::array<bool, NUM_STATE_FLAGS> m_flags;

        Color m_pupil_color{ 0, 0, 0, 0xFF };
        Color m_hair_color{ 0xFF, 0xFF, 0xFF, 0xFF };
        Color m_eyes_color{ 0xFF, 0xFF, 0xFF, 0xFF };

    public:
        std::vector<uint8_t> Pack() const {
            std::vector<uint8_t> ret{};
            ret.resize(sizeof(CharacterState));
            
            BinaryWriter buffer{ ret.data() };
            buffer.write<uint8_t>(this->GetPunchID());
            buffer.write<uint8_t>(this->GetPunchRange());
            buffer.write<uint8_t>(this->GetBuildRange());

            buffer.write<float>(this->GetWaterSpeed());
            buffer.write<float>(this->GetSpeed());
            buffer.write<float>(this->GetGravity());
            buffer.write<float>(this->GetAcceleration());
            buffer.write<float>(this->GetPunchStrength());
            
            for (auto index = 0; index < NUM_STATE_FLAGS; index++)
                buffer.write<bool>(this->m_flags[index]);

            buffer.write<uint32_t>(this->GetPupilColor().GetInt());
            buffer.write<uint32_t>(this->GetHairColor().GetInt());
            buffer.write<uint32_t>(this->GetEyesColor().GetInt());

            return ret;
        }
        void Serialize(const std::vector<uint8_t>& data) {
            BinaryReader br{ data };
            this->m_punch_id = br.read<uint8_t>();
            this->m_punch_range = br.read<uint8_t>();
            this->m_build_range = br.read<uint8_t>();

            this->m_water_speed = br.read<float>();
            this->m_speed = br.read<float>();
            this->m_gravity = br.read<float>();
            this->SetAcceleration(br.read<float>());
            this->SetPunchStrength(br.read<float>());

            for (auto index = 0; index < NUM_STATE_FLAGS; index++)
                this->m_flags[index] = br.read<bool>();

            this->SetPupilColor(Color{ br.read<uint32_t>() });
            this->SetHairColor(Color{ br.read<uint32_t>() });
            this->SetEyesColor(Color{ br.read<uint32_t>() });
        }
    };

    enum ePunchEffectType {
        PUNCH_EFFECT_NONE,
        PUNCH_EFFECT_CYCLOPEAN_VISOR,
        PUNCH_EFFECT_HEARTBOW,
        PUNCH_EFFECT_TOMMYGUN,
        PUNCH_EFFECT_ELVISH_LONGBOW,
        PUNCH_EFFECT_SAWED_OFF_SHOTGUN,
        PUNCH_EFFECT_DRAGON_HAND,
        PUNCH_EFFECT_REANIMATOR_REMOTE,
        PUNCH_EFFECT_DEATH_RAY,
        PUNCH_EFFECT_SIX_SHOOTER,
        PUNCH_EFFECT_FOCUSED_EYES,
        PUNCH_EFFECT_ICE_DRAGON_HAND,
        PUNCH_EFFECT_EVIL_SPACE_HELMET,
        PUNCH_EFFECT_ATOMIC_SHADOW_SCYTHE,
        PUNCH_EFFECT_PET_LEPRECHAUN,
        PUNCH_EFFECT_BATTLE_TROUT,
        PUNCH_EFFECT_FIESTA_DRAGON,
        PUNCH_EFFECT_SQUIRT_GUN,
        PUNCH_EFFECT_GUITAR,
        PUNCH_EFFECT_FLAMETHROWER,
        PUNCH_EFFECT_LEGENDBOT,
        PUNCH_EFFECT_DRAGON_OF_LEGEND,
        PUNCH_EFFECT_ZEUS_LIGHTNING_BOLT,
        PUNCH_EFFECT_VIOLET_PROTODRAKE,
        PUNCH_EFFECT_RING_OF_FORCE,
        PUNCH_EFFECT_ICE_CALF,
        PUNCH_EFFECT_OWLBEARD,
        PUNCH_EFFECT_CHAOS_CURSED_WAND,
        PUNCH_EFFECT_FLAMETHROWER_2,
        PUNCH_EFFECT_SWORD,
        PUNCH_EFFECT_CLAW_GLOVE,
        PUNCH_EFFECT_COSMIC_UNICORN,
        PUNCH_EFFECT_BLACK_CRYSTAL_DRAGON,
        PUNCH_EFFECT_MIGHTY_SNOW_ROD,
        PUNCH_EFFECT_TINY_TANK,
        PUNCH_EFFECT_ICICLES
    };
}