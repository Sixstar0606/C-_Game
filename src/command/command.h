#pragma once
#include <command/command_context.h>

namespace GTServer {
    class Command {
    public:
        Command(const std::string& name, const std::vector<std::string>& aliases, const std::string& description, const ePlayerRole& role,
            std::function<void(const CommandContext&)> func) : 
            m_name{ name },
            m_aliases{ aliases },
            m_description{ description },
            m_role{ role },
            m_function{ func } {
        }
        ~Command() {
            m_aliases.clear();
        }

        [[nodiscard]] std::string GetName() const { return m_name; }
        void SetName(const std::string& name) { m_name = name; }

        [[nodiscard]] std::vector<std::string> GetAliases() const { return m_aliases; }
        void SetAliases(const std::vector<std::string>& aliases) { m_aliases = aliases; }

        [[nodiscard]] std::string GetDescription() const { return m_description; }
        void SetDescription(const std::string& description) { m_description = description; }

        [[nodiscard]] ePlayerRole GetRole() const { return m_role; }
        void SetRole(const ePlayerRole& role) { m_role = role; }

        [[nodiscard]] std::function<void(const CommandContext&)> GetFunction() const { return m_function; }
        void SetFunction(const std::function<void(const CommandContext&)>& function) { m_function = function; }

    private:
        std::string m_name;
        std::vector<std::string> m_aliases;
        std::string m_description;
        ePlayerRole m_role;

        std::function<void(const CommandContext&)> m_function;
    };
}