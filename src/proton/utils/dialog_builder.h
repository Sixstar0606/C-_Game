#pragma once
#include <string>
#include <fmt/core.h>
#include <proton/utils/common.h>

namespace GTServer
{
	class DialogBuilder {
	public:
		enum size_type {
			SMALL,
			BIG
		};
		enum direction {
			NONE,
			LEFT,
			RIGHT,
			STATIC_BLUE_FRAME
		};
	private:
		[[nodiscard]] std::string get_size(const int& size) const {
			switch (size) {
				case size_type::SMALL:
					return "small";
				default:
					return "big";
			}
		}
		[[nodiscard]] std::string get_direction(const int& direction) const {
            switch (direction) {
            case direction::LEFT:
                return "left";
            case direction::RIGHT:
                return "right";
            case direction::STATIC_BLUE_FRAME:
                return "staticBlueFrame";
            default:
                return "";
            }
            return "";
        }

	public:
		DialogBuilder() { m_result.clear(); }
		~DialogBuilder() {}

		[[nodiscard]] std::string get() const { return m_result; }
		void clear() {
            m_result.clear();
        }
		
		DialogBuilder* set_default_color(const char& color) {
            m_result.append(fmt::format("\nset_default_color|`{}", color));
            return this;
        }
		DialogBuilder* text_scaling_string(const std::string& scale){
            m_result.append(fmt::format("\ntext_scaling_string|{}", scale));
            return this;
        }

		DialogBuilder* embed_data(const std::string& name, const std::string& embed_value) {
			m_result.append(fmt::format("\nembed_data|{}|{}", name, embed_value));
			return this;
		}
        template<typename T, typename std::enable_if_t<std::is_integral_v<T>, bool> = true>
		DialogBuilder* embed_data(const std::string& name, const T& embed_value) {
			m_result.append(fmt::format("\nembed_data|{}|{}", name, std::to_string((int32_t)embed_value)));
			return this;
		}
        template<typename T, typename std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
		DialogBuilder* embed_data(const std::string& name, const T& embed_value) {
			if (std::is_same_v<T, double>)
				m_result.append(fmt::format("\nembed_data|{}|{}", name, std::stod(embed_value)));
            else if (std::is_same_v<T, long double>)
                m_result.append(fmt::format("\nembed_data|{}|{}", name, std::stold(embed_value)));
            else
				m_result.append(fmt::format("\nembed_data|{}|{}", name, std::stof(embed_value)));
			return this;
		}

		DialogBuilder* end_dialog(const std::string& name, const std::string& cancel, const std::string& ok) {
			m_result.append(fmt::format("\nend_dialog|{}|{}|{}|", name, cancel, ok));
			return this;
		}

		DialogBuilder* add_custom(const std::string& value);
		DialogBuilder* add_custom_break() {
			m_result.append("\nadd_custom_break|");
			return this;
		}
		DialogBuilder* add_spacer(const size_type& label_size = SMALL) {
			m_result.append(fmt::format("\nadd_spacer|{}|", this->get_size(label_size)));
			return this;
		}
		DialogBuilder* set_custom_spacing(const int& x, const int& y) {
			m_result.append(fmt::format("\nset_custom_spacing|x:{};y:{}|", x, y));
			return this;
		}
		DialogBuilder* add_label(const std::string& label, const int& start_direction = LEFT, const size_type& label_size = SMALL) {
			m_result.append(fmt::format("\nadd_label|{}|{}|{}|0|", this->get_size(label_size), label, this->get_direction(start_direction)));
        	return this;
		}
		DialogBuilder* add_custom_label(const std::string& label, const std::string& target, const double& top, const double& left, const size_type& size = SMALL) {
			m_result.append(fmt::format("\nadd_custom_label|{}|target:{};top:{};left:{};size:{}|", label, target, top, left, this->get_size(size)));
        	return this;
		}
		DialogBuilder* add_textbox(const std::string& label) {
			m_result.append(fmt::format("\nadd_textbox|{}|", label));
        	return this;
		}
		DialogBuilder* add_smalltext(const std::string& label) {
			m_result.append(fmt::format("\nadd_smalltext|{}|", label));
			return this;
		}
		DialogBuilder* add_text_input(const std::string& name, const std::string& label, const std::string& label_inside, const int& max_length) {
			m_result.append(fmt::format("\nadd_text_input|{}|{}|{}|{}|", name, label, label_inside, max_length));
        	return this;
		}
		DialogBuilder* add_text_input_password(const std::string& name, const std::string& label, const std::string& label_inside, const int& max_length) {
			m_result.append(fmt::format("\nadd_text_input_password|{}|{}|{}|{}|", name, label, label_inside, max_length));
        	return this;
		}
		DialogBuilder* add_text_box_input(const std::string& name, const std::string& label, std::string text_inside, const int& max_length, const int& lines);
		DialogBuilder* add_button(const std::string& name, const std::string& label, const std::string& btn_flag = "noflags") {
			m_result.append(fmt::format("\nadd_button|{}|{}|{}|0|0|", name, label, btn_flag));
        	return this;
		}
		DialogBuilder* add_button_with_icon(const std::string& name, const std::string& label, const int& item_id, const int& start_direction = LEFT) {
			m_result.append(fmt::format("\nadd_button_with_icon|{}|{}|{}|{}|", name, label, this->get_direction(start_direction), item_id));
        	return this;
		}
		DialogBuilder* add_button_with_icon(const std::string& name, const std::string& label, const int& item_id, const std::string& description, const int& start_direction = LEFT) {
			m_result.append(fmt::format("\nadd_button_with_icon|{}|{}|{}|{}|{}|", name, label, this->get_direction(start_direction), item_id, description));
        	return this;
		}
		DialogBuilder* add_custom_button(const std::string& name, const std::string& image, const int& image_x, const int& image_y, const double& width) {
			m_result.append(fmt::format("\nadd_custom_button|{}|image:{};image_size:{},{};width:{};|", name, image, image_x, image_y, width));
        	return this;
		}
		DialogBuilder* add_friend_image_label_button(const std::string& name, const std::string& label, const std::string& texture_path, const double& size, const CL_Vec2i& texture) {
			m_result.append(fmt::format("\nadd_friend_image_label_button|{}|{}|{}|{}|{}|{}|32|false|false|", name, label, texture_path, size, texture.m_x, texture.m_y));
			return this;
		}
		DialogBuilder* add_label_with_icon(const std::string& label, const int& itemId, const int& start_direction = LEFT, const size_type& label_size = SMALL) {
			m_result.append(fmt::format("\nadd_label_with_icon|{}|{}|{}|{}|", this->get_size(label_size), label, this->get_direction(start_direction), itemId));
			return this;
		}
		DialogBuilder* add_label_with_icon_button(const std::string& name, const std::string& label, const int& itemId, const int& start_direction = LEFT, const size_type& label_size = SMALL);
		DialogBuilder* add_dual_layer_icon_label(const std::string& label, const std::pair<int, int>& icon, const double& size, const int& start_direction = LEFT, const size_type& label_size = SMALL) {
			m_result.append(fmt::format("\nadd_dual_layer_icon_label|{}|{}|{}|{}|{}|{}|0|", this->get_size(size), label, this->get_direction(start_direction), icon.second, icon.first, size));
        	return this;
		}
		DialogBuilder* add_player_info(const std::string& name, const int& level, const int& exp, const int& math_exp) {
			m_result.append(fmt::format("\nadd_player_info|{}|{}|{}|{}", name, level, exp, math_exp));
        	return this;
		}
		DialogBuilder* add_item_picker(const std::string& name, const std::string& label, const std::string& floating_text) {
			m_result.append(fmt::format("\nadd_item_picker|{}|{}|{}|", name, label, floating_text));
        	return this;
		}
		DialogBuilder* add_player_picker(const std::string& name, const std::string& label) {
			m_result.append(fmt::format("\nadd_player_picker|{}|{}", name, label));
        	return this;
		}
		DialogBuilder* add_checkbox(const std::string& name, const std::string& label, const bool& active) {
			m_result.append(fmt::format("\nadd_checkbox|{}|{}|{}", name, label, active ? "1" : "0"));
        	return this;
		}
		DialogBuilder* add_checkicon(const std::string& name, const std::string& label, const int& id, const std::string& frame_message, const bool& enabled) {
			m_result.append(fmt::format("\nadd_checkicon|{}|{}|staticframe|{}|{}|{}", name, label, id, frame_message, enabled ? "1" : "0"));
        	return this;
		}
		DialogBuilder* add_quick_exit() {
			m_result.append("\nadd_quick_exit|");
			return this;
		}
	private:
		std::string m_result{};
	};
}