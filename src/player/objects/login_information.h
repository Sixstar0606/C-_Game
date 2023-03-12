#pragma once
#include <string>
#include <proton/utils/text_scanner.h>

namespace GTServer {
    class LoginInformation
    {
    public:
        LoginInformation() = default;
        ~LoginInformation() = default;
        
    public:
        int32_t m_platform{ PLATFORM_ID_UNKNOWN };

        std::string m_tank_id_name{};
        std::string m_tank_id_pass{};
        std::string m_email{};
        
        std::string m_fz{};
        std::string m_wk{};
        std::string m_zf{};
        std::string m_tr{};
        std::string m_aid{};
        std::string m_vid{};
        std::string m_gid{};
        std::string m_rid{};
        std::string m_mac{};

        std::string m_country{ "us" };

        bool Serialize(TextScanner parser) {
            this->m_mac = parser.Get("mac", 1);
            this->m_rid = parser.Get("rid", 1);
            switch (this->m_platform) {
                case PLATFORM_ID_WINDOWS: {
                    if (!(
                        parser.TryGet("fz", this->m_fz) &&
                        parser.TryGet("wk", this->m_wk) &&
                        parser.TryGet("zf", this->m_zf)
                    )) return false;
                } break;
                case PLATFORM_ID_IOS: {
                    if (!(
                        parser.TryGet("vid", this->m_vid)
                    )) return false;
                } break;
                case PLATFORM_ID_OSX:
                    break;
                case PLATFORM_ID_ANDROID:
                    break;
                default: //todo more platform checks
                    return true;
            }
            return true;
        }
    };
}
