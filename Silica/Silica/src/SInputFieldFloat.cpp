#include "SInputFieldFloat.h"

#include <sstream>

#include "SEditableText.h"

namespace Silica {

	void SInputFieldFloat::construct(const Args& args) {
		m_currentValue = args.initialValue;
		m_onValueChanged = args.onValueChanged;

		std::string initialText = std::to_string(m_currentValue);

		std::stringstream ss;
		ss << std::defaultfloat << m_currentValue;
		initialText = ss.str();

		m_editableText = MakeWidget<SEditableText>({
			.initialText = initialText,
			.font = args.font,
			.charFilter = [](char c) {
				return std::isdigit(c) || c == '.' || c == '-';
			},
			.onTextCommitted = [this](const std::string& newText) {
				try {
					if (newText.empty()) {
						setValue(m_currentValue);
						return;
					}

					float newValue = std::stof(newText);
					m_currentValue = newValue;

					setValue(m_currentValue);

					if (m_onValueChanged) {
						m_onValueChanged(m_currentValue);
					}
				}
				catch (...) {
					setValue(m_currentValue);
				}
			}
		});
	}

	void SInputFieldFloat::computeDesiredSize() {
		m_editableText->computeDesiredSize();
		m_desiredSize = m_editableText->getDesiredSize();
	}

	void SInputFieldFloat::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		m_editableText->arrangeChildren(allocatedGeometry);
	}

	void SInputFieldFloat::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		m_editableText->onDraw(outDrawList, allocatedGeometry);
	}

	void SInputFieldFloat::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_editableText) m_editableText->setRenderScale(scale);
	}

	void SInputFieldFloat::setValue(float newValue, bool moveCursorToEnd) {
		m_currentValue = std::round(newValue * 1000000.0f) / 1000000.0f;

		char buffer[32];
		std::snprintf(buffer, sizeof(buffer), "%.6g", m_currentValue);

		if (auto textWidget = std::dynamic_pointer_cast<SEditableText>(m_editableText)) {
			textWidget->setText(std::string(buffer), moveCursorToEnd);
		}
	}

	EventReply SInputFieldFloat::onMouseMove(const Geometry& geo, const Vec2& pos) {
		return m_editableText->onMouseMove(geo, pos);
	}

	EventReply SInputFieldFloat::onMouseButtonDown(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		return m_editableText->onMouseButtonDown(geo, pos, btn);
	}

	EventReply SInputFieldFloat::onMouseButtonUp(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		return m_editableText->onMouseButtonUp(geo, pos, btn);
	}

	EventReply SInputFieldFloat::onMouseWheel(const Geometry& geo, const Vec2& pos, float scrollDelta) {
		if (geo.contains(pos)) {
			bool isCtrlHeld = Platform::isKeyDown(Key::LeftControl) || Platform::isKeyDown(Key::RightControl);

			if (isCtrlHeld) {
				bool isShiftHeld = Platform::isKeyDown(Key::LeftShift) || Platform::isKeyDown(Key::RightShift);
				float step = isShiftHeld ? 1.0f : 0.1f;
				float rawValue = m_currentValue + (scrollDelta * step);

				setValue(rawValue, true);

				if (m_onValueChanged) {
					m_onValueChanged(m_currentValue);
				}

				return EventReply::handled();
			}
		}
		return m_editableText->onMouseWheel(geo, pos, scrollDelta);
	}

	EventReply SInputFieldFloat::onDragOver(const Geometry& geo, const Vec2& pos, const DragDropPayload& p) {
		return m_editableText->onDragOver(geo, pos, p);
	}

	EventReply SInputFieldFloat::onDrop(const Geometry& geo, const Vec2& pos, const DragDropPayload& p) {
		return m_editableText->onDrop(geo, pos, p);
	}

}
