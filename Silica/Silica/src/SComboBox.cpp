#include "silicapch.h"
#include "SComboBox.h"

#include "SButton.h"
#include "STextBlock.h"
#include "SVerticalBox.h"
#include "SScrollBox.h"
#include "SBox.h"
#include "SEditableText.h"
#include "Renderer.h"

namespace Silica {

	void SComboBox::construct(const Args& args) {
		m_options = args.options;
		m_font = args.font;
		m_onValueChanged = args.onValueChanged;
		m_currentValue = args.initialValue;

		m_textBlock = MakeWidget<STextBlock>({
			.text = m_currentValue,
			.font = m_font
		});

		// -- Main Button --
		m_mainButton = MakeWidget<SButton>({
			.padding = { 8.0f, 4.0f },
			.color = GetTheme().Background_Input,
			.onClick = [this]() {
				m_isOpen = !m_isOpen;
				if (m_isOpen && m_searchBox) {
					m_searchBox->setText("", true);
					rebuildOptions("");
					SWidget::setFocusedWidget(m_searchBox.get());
				}
				return EventReply::handled();
			},
			.child = m_textBlock
		});

		// -- List Of Options --
		m_optionsBox = MakeWidget<SVerticalBox>({ .spacing = 0.0f });
		rebuildOptions("");

		WidgetPtr popupContent;

		if (args.searchable) {
			// -- Searchable Mode --
			m_searchBox = MakeWidget<SEditableText>({
				.hintText = "Search...",
				.font = m_font,
				.onTextChanged = [this](const std::string& text) {
					rebuildOptions(text);
				}
			});

			popupContent = MakeWidget<SVerticalBox>({
				.spacing = 2.0f,
				.slots = {
					{ {4.0f, 4.0f}, m_searchBox },
					{ {0.0f, 0.0f}, MakeWidget<SScrollBox>({.child = m_optionsBox }) }
				}
			});
		}
		else {
			// -- Standard Mode --
			popupContent = MakeWidget<SScrollBox>({ .child = m_optionsBox });
		}

		// -- Popup Container --
		m_popupMenu = MakeWidget<SBox>({
			.borderThickness = 1.0f,
			.backgroundColor = GetTheme().Background_Panel,
			.borderColor = GetTheme().Border_Primary,
			.child = popupContent
		});
	}

	void SComboBox::computeDesiredSize() {
		m_mainButton->computeDesiredSize();
		m_desiredSize = m_mainButton->getDesiredSize();
	}

	void SComboBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		m_mainButton->arrangeChildren(allocatedGeometry);

		if (m_isOpen) {
			m_popupMenu->setRenderScale(1.0f);
			m_popupMenu->computeDesiredSize();

			float popupHeight = std::min(m_popupMenu->getDesiredSize().y, 250.0f);

			Geometry popupGeo = {
				{ allocatedGeometry.position.x, allocatedGeometry.position.y + allocatedGeometry.size.y },
				{ std::max(allocatedGeometry.size.x, 140.0f), popupHeight }
			};

			m_popupMenu->arrangeChildren(popupGeo);

			Renderer::pushPopup(m_popupMenu, popupGeo, [this]() {
				m_isOpen = false;
			});
		}
	}

	void SComboBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		m_mainButton->onDraw(outDrawList, allocatedGeometry);
	}

	void SComboBox::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_mainButton) m_mainButton->setRenderScale(scale);
	}

	EventReply SComboBox::onMouseMove(const Geometry& geo, const Vec2& pos) {
		return m_mainButton->onMouseMove(geo, pos);
	}

	EventReply SComboBox::onMouseButtonDown(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		return m_mainButton->onMouseButtonDown(geo, pos, btn);
	}

	EventReply SComboBox::onMouseButtonUp(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		return m_mainButton->onMouseButtonUp(geo, pos, btn);
	}

	EventReply SComboBox::onMouseWheel(const Geometry& geo, const Vec2& pos, float scroll) {
		if (geo.contains(pos)) {
			bool isCtrlHeld = Platform::isKeyDown(Key::LeftControl) || Platform::isKeyDown(Key::RightControl);

			if (isCtrlHeld && !m_options.empty()) {
				int currentIndex = 0;
				for (int i = 0; i < (int)m_options.size(); ++i) {
					if (m_options[i] == m_currentValue) {
						currentIndex = i;
						break;
					}
				}

				int step = (scroll > 0.0f) ? -1 : 1;
				int newIndex = std::clamp(currentIndex + step, 0, (int)m_options.size() - 1);

				if (newIndex != currentIndex) {
					m_currentValue = m_options[newIndex];
					m_textBlock->setText(m_currentValue);

					auto cb = m_onValueChanged;
					if (cb) cb(m_currentValue);
				}

				return EventReply::handled();
			}
		}

		return m_mainButton->onMouseWheel(geo, pos, scroll);
	}

	void SComboBox::rebuildOptions(const std::string& query) {
		m_optionsBox->clearSlots();

		std::string lowerQuery = query;
		std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

		for (const auto& opt : m_options) {
			std::string lowerOpt = opt;
			std::transform(lowerOpt.begin(), lowerOpt.end(), lowerOpt.begin(), ::tolower);

			if (lowerQuery.empty() || lowerOpt.find(lowerQuery) != std::string::npos) {
				m_optionsBox->addSlot({ {0,0}, MakeWidget<SButton>({
					.padding = { 8.0f, 4.0f },
					.color = Color::transparent(),
					.hoverColor = GetTheme().Element_Hover,
					.onClick = [this, opt]() {
						m_currentValue = opt;
						m_textBlock->setText(opt);
						m_isOpen = false;

						auto cb = m_onValueChanged;
						if (cb) cb(opt);

						return EventReply::handled();
					},
					.child = MakeWidget<STextBlock>({
						.text = opt,
						.font = m_font
					})
				}) });
			}
		}
	}

}
