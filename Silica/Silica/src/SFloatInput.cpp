#include "SFloatInput.h"

#include <sstream>
#include <iomanip>

#include "SEditableText.h"

namespace Silica {

	void SFloatInput::construct(const Args& args) {
		m_currentValue = args.initialValue;
		m_onValueChanged = args.onValueChanged;

		std::stringstream ss;
		ss << std::fixed << std::setprecision(2) << m_currentValue;

		m_editableText = MakeWidget<SEditableText>({
			.initialText = ss.str(),
			.font = args.font,
			.onTextCommitted = [this](const std::string& newText) {
				try {
					m_currentValue = std::stof(newText);
					if (m_onValueChanged) {
						m_onValueChanged(m_currentValue);
					}
				}
				catch (...) {
				}
			}
		});
	}

	void SFloatInput::computeDesiredSize() {
		m_editableText->computeDesiredSize();
		m_desiredSize = m_editableText->getDesiredSize();
	}

	void SFloatInput::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		m_editableText->arrangeChildren(allocatedGeometry);
	}

	void SFloatInput::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		m_editableText->onDraw(outDrawList, allocatedGeometry);
	}

	EventReply SFloatInput::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		return m_editableText->onMouseMove(allocatedGeometry, mousePos);
	}

	EventReply SFloatInput::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		return m_editableText->onMouseButtonDown(allocatedGeometry, mousePos, button);
	}

	EventReply SFloatInput::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		return m_editableText->onMouseButtonUp(allocatedGeometry, mousePos, button);
	}

}
