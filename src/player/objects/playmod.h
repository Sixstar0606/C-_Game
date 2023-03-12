#pragma once
#include <unordered_map>
#include <utils/timing_clock.h>

namespace GTServer {
    enum ePlaymodType : uint16_t {
        PLAYMOD_TYPE_DOUBLE_JUMP,
        PLAYMOD_TYPE_HIGH_JUMP,
        PLAYMOD_TYPE_FISTS_O_FURY,
        PLAYMOD_TYPE_ARMED_AND_DANGEROUS,
        PLAYMOD_TYPE_SPEEDY,
        PLAYMOD_TYPE_TANK_DRIVER,
        PLAYMOD_TYPE_SOCCER_SKILLS,
        PLAYMOD_TYPE_PUTT_PUTT_PUTT,
        PLAYMOD_TYPE_FIREPROOF,
        PLAYMOD_TYPE_EYE_BEAM,
        PLAYMOD_TYPE_FOCUSED_EYES,
        PLAYMOD_TYPE_ENCHANCED_DIGGING,
        PLAYMOD_TYPE_DEATH_TO_BLOCKS,
        PLAYMOD_TYPE_DRAGONBORN,
        PLAYMOD_TYPE_DRAGONBORN_WITH_A_SLIVER_SPOON,
        PLAYMOD_TYPE_CUPIDS_FIREPOWER,
        PLAYMOD_TYPE_DEVOURING_SOULS,
        PLAYMOD_TYPE_NINJA_STEALTH,
        PLAYMOD_TYPE_RAINBOWS,
        PLAYMOD_TYPE_ZEPHYR_HELM,
        PLAYMOD_TYPE_PICKIN_AND_CLIMBIN,
        PLAYMOD_TYPE_MUSICAL,
        PLAYMOD_TYPE_BOW_AND_ARROW,
        PLAYMOD_TYPE_FIERY_PET,
        PLAYMOD_TYPE_SLASHER,
        PLAYMOD_TYPE_CLAWS,
        PLAYMOD_TYPE_ICY_PET,
        PLAYMOD_TYPE_CHEERFUL_GIVER,
        PLAYMOD_TYPE_WIZARDLY_FORCE,
        PLAYMOD_TYPE_FLAMES_OF_AJANTI,
        PLAYMOD_TYPE_THE_ONE_RING,
        PLAYMOD_TYPE_A_LITTLE_FISHY,
        PLAYMOD_TYPE_FLAMING_ARROWS,
        PLAYMOD_TYPE_PRECISION_TOOL,
        PLAYMOD_TYPE_CHAOS,
        PLAYMOD_TYPE_FLAMETHROWIN,
        PLAYMOD_TYPE_BUNNYESQUE,
        PLAYMOD_TYPE_GEIGER_COUNTING,
        PLAYMOD_TYPE_CHARGING_GEIGER_COUNTER,
        PLAYMOD_TYPE_IRRADIATED,
        PLAYMOD_TYPE_LASERBEAST,
        PLAYMOD_TYPE_FLYING_REINDEER,
        PLAYMOD_TYPE_FLAME_SCYTHE,
        PLAYMOD_TYPE_FIRE_HOSE,
        PLAYMOD_TYPE_HUNTING_GHOSTS,
        PLAYMOD_TYPE_RING_OF_FORCE,
        PLAYMOD_TYPE_IRON_MMMFF,
        PLAYMOD_TYPE_PRISMATIC_AURA,
        PLAYMOD_TYPE_CONFETTI,
        PLAYMOD_TYPE_DRAGOSCARF,
        PLAYMOD_TYPE_FLAMING_HOT,
        PLAYMOD_TYPE_CLOAK_OF_FALLING_WATER,
        PLAYMOD_TYPE_SOAKED,
        PLAYMOD_TYPE_ENERGIZED_HORN,
        PLAYMOD_TYPE_IM_ON_A_SHARK,
        PLAYMOD_TYPE_ETHEREAL_RAINBOW_DRAGON,
        PLAYMOD_TYPE_LEGENDARY_DRAGON,
        PLAYMOD_TYPE_LEGENDARY_BOT,
        PLAYMOD_TYPE_LEGENDARY_SWORDSMASTER,
        PLAYMOD_TYPE_WHIP_OF_TRUTH,
        PLAYMOD_TYPE_CURSE,
        PLAYMOD_TYPE_BAN,
        PLAYMOD_TYPE_DUCT_TAPE,
        PLAYMOD_TYPE_FROZEN,
        PLAYMOD_TYPE_FLOATING,
        PLAYMOD_TYPE_SAUCY,
        PLAYMOD_TYPE_EGGED,
        PLAYMOD_TYPE_FEELIN_BLUE,
        PLAYMOD_TYPE_ENVIOUS,
        PLAYMOD_TYPE_DOCTOR_REPLUSION,
        PLAYMOD_TYPE_CAFFEINATED,
        PLAYMOD_TYPE_DEVIL_HORNS,
        PLAYMOD_TYPE_LUCKY,
        PLAYMOD_TYPE_ON_FIRE,
        PLAYMOD_TYPE_WINTERFEST_CROWN,
        PLAYMOD_TYPE_INFECTED,
        PLAYMOD_TYPE_SLIMED,
        PLAYMOD_TYPE_MIND_CONTROL,
        PLAYMOD_TYPE_MALPRACTICE,
        PLAYMOD_TYPE_RECENTLY_NAME_CHANGED,
        PLAYMOD_TYPE_RECOVERING,
        PLAYMOD_TYPE_NOOB,
        PLAYMOD_TYPE_DISHONOURED,
        PLAYMOD_TYPE_VALENTINE,
        PLAYMOD_TYPE_BALLOON_IMMUNITY,
        PLAYMOD_TYPE_MEGAPHONE,
        PLAYMOD_TYPE_XP_BUFF,
        PLAYMOD_TYPE_COFFEE,
        PLAYMOD_TYPE_GHOST_IN_THE_SHELL,
        PLAYMOD_TYPE_NICK,
        PLAYMOD_TYPE_INVISIBILITY,
        PLAYMOD_TYPE_IN_THE_SPOTLIGHT,
        PLAYMOD_TYPE_G_VIRUS_IMMUNITY,
        PLAYMOD_TYPE_MARK_OF_GROWGANOTH,
        PLAYMOD_TYPE_HELICOPTER_HAIR,
        PLAYMOD_TYPE_RAYMAN_SFIST,
        PLAYMOD_TYPE_INFLUENCER,
        PLAYMOD_TYPE_SLIGHTLY_DAMP,
        PLAYMOD_TYPE_FREEZE,
        PLAYMOD_TYPE_SNOWBALL,
        PLAYMOD_TYPE_GOLDEN_HALO,
        PLAYMOD_TYPE_HAUNTED,
        PLAYMOD_TYPE_INVISIBLE,
        PLAYMOD_TYPE_1HIT,
        PLAYMOD_TYPE_RAISE_THE_FLAG__BANNER_BANDOLIER
    };

    struct Playmod {
        ePlaymodType m_type;
        uint16_t m_icon_id;
        TimingClock m_time;

        std::string GetTabString() const {
            std::chrono::seconds time = std::chrono::duration_cast<std::chrono::seconds>(m_time.GetTime() - std::chrono::steady_clock::now()) + m_time.GetTimeout();

            static int day_count = 24 * 3600;
            static int hour_count = 3600;
            static int min_count = 60;

            int n = time.count();

            int day = n / day_count;
            n %= day_count;
            int hour = n / hour_count;
            n %= hour_count;
            int min = n / min_count;
            n %= min_count;
            int sec = n;

            std::string result;

            if (day != 0)
                result.append(fmt::format("{} days, ", day));
            if (hour != 0)
                result.append(fmt::format("{} hours, ", hour));
            if (min != 0)
                result.append(fmt::format("{} mins, ", min));

            result.append(fmt::format("{} secs", sec));
            return result;
        }
    };

    struct PlaymodData {
        ePlaymodType m_type;
        std::string m_name;

        std::string m_adding;
        std::string m_removing;
    };

    namespace PlaymodManager {
        inline std::vector<PlaymodData> g_playmods = {
            {
                PLAYMOD_TYPE_DOUBLE_JUMP, "Double Jump",
                "You can jump mid-air! Its like flying but less.",
                "Gravity - It's the law."
            }, {
                PLAYMOD_TYPE_HIGH_JUMP, "High Jump",
                "You can fly!! Well, you can jump higher than normal.",
                "Gravity - It's the law."
            }, {
                PLAYMOD_TYPE_FISTS_O_FURY, "Fists O' Fury",
                "Float like a butterfly, sting like a punch in the face.",
                "Boxing time over."
            }, {
                PLAYMOD_TYPE_ARMED_AND_DANGEROUS, "Armed and Dangerous",
                "You shoot bullets instead of punching!",
                "You are disarmed."
            }, {
                PLAYMOD_TYPE_SPEEDY, "Speedy",
                "You can move faster!",
                "You are slow again."
            }, {
                PLAYMOD_TYPE_GHOST_IN_THE_SHELL, "Ghost",
                "You a ghost now!",
                "You not a ghost."
            }, {
                PLAYMOD_TYPE_NICK, "Undercover",
                "You are now undercover!",
                "You are no longer undercover."
            }, {
                PLAYMOD_TYPE_DUCT_TAPE, "Duct Tape",
                "Duct tape has covered your mouth!",
                "Duct tape removed. OUCH!"
            }, {
                PLAYMOD_TYPE_FROZEN, "Frozen",
                "Your body has turned to ice. You can't move!",
                "You've thawed out."
            }, {
                PLAYMOD_TYPE_CURSE, "Cursed",
                "You have broken the rules and now can only go to the world HELL",
                "You no longer have to limit yourself to that world."
            }, {
                PLAYMOD_TYPE_INVISIBLE, "Invisible",
                "You are now invisible to other players!",
                "You are now visible to other players."
            }, {
                PLAYMOD_TYPE_1HIT, "1 Hit",
                "You can break anything in 1 hit!",
                "You now break stuff like normal."
            }, {
                PLAYMOD_TYPE_BAN, "Ban",
                "It appears that you want to take a break from BetterGrowtopia.",
                "Welcome back to BetterGrowtopia."
            }, {
                PLAYMOD_TYPE_TANK_DRIVER, "Tank Driver",
                "You're driving a tank!",
                "Back on foot."
            }, {
                PLAYMOD_TYPE_IN_THE_SPOTLIGHT, "In the Spotlight",
                "All eyes are on you!",
                "Back to anonymity."
            }
        };
        inline PlaymodData Get(ePlaymodType type) {
            auto iterator = std::find_if(g_playmods.begin(), g_playmods.end(), 
                [&](const PlaymodData& it) { return it.m_type == type; });
            if (iterator == g_playmods.end())
                return g_playmods[0];
            return *iterator;
        }
    };
}