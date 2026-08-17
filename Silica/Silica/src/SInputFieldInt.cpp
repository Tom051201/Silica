#include "SInputFieldInt.h"

#include <algorithm>

#include "SEditableText.h"

namespace Silica {

	void SInputFieldInt::construct(const Args& args) {
		m_currentValue = args.initialValue;
		m_onValueChanged = args.onValueChanged;

		m_editableText = MakeWidget<SEditableText>({
			.initialText = std::to_string(m_currentValue),
			.font = args.font,
			.charFilter = [](char c) {
				return std::isdigit(c) || c == '-';
			},
			.onTextCommitted = [this](const std::string& newText) {
				try {
					if (newText.empty()) {
						setValue(m_currentValue);
						return;
					}

					int newValue = std::stoi(newText);
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

	void SInputFieldInt::setValue(int newValue, bool moveCursorToEnd) {
		m_currentValue = newValue;
		if (auto textWidget = std::dynamic_pointer_cast<SEditableText>(m_editableText)) {
			textWidget->setText(std::to_string(m_currentValue), moveCursorToEnd);
		}
	}

	void SInputFieldInt::computeDesiredSize() {
		m_editableText->computeDesiredSize();
		m_desiredSize = m_editableText->getDesiredSize();
	}

	void SInputFieldInt::arrangeChildren(const Geometry& geo) {
		SWidget::arrangeChildren(geo);
		m_editableText->arrangeChildren(geo);
	}

	void SInputFieldInt::onDraw(DrawList& out, const Geometry& geo) const {
		m_editableText->onDraw(out, geo);
	}

	void SInputFieldInt::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_editableText) m_editableText->setRenderScale(scale);
	}
	
	EventReply SInputFieldInt::onMouseMove(const Geometry& g, const Vec2& p) {
		return m_editableText->onMouseMove(g, p);
	}

	EventReply SInputFieldInt::onMouseButtonDown(const Geometry& g, const Vec2& p, MouseButton b) {
		return m_editableText->onMouseButtonDown(g, p, b);
	}

	EventReply SInputFieldInt::onMouseButtonUp(const Geometry& g, const Vec2& p, MouseButton b) {
		return m_editableText->onMouseButtonUp(g, p, b);
	}

	EventReply SInputFieldInt::onMouseWheel(const Geometry& geo, const Vec2& pos, float scrollDelta) {
		if (geo.contains(pos)) {
			bool isCtrlHeld = Platform::isKeyDown(Key::LeftControl) || Platform::isKeyDown(Key::RightControl);

			if (isCtrlHeld) {
				bool isShiftHeld = Platform::isKeyDown(Key::LeftShift) || Platform::isKeyDown(Key::RightShift);

				int step = isShiftHeld ? 10 : 1;
				int newValue = m_currentValue + (static_cast<int>(scrollDelta) * step);

				setValue(newValue, true);

				if (m_onValueChanged) {
					m_onValueChanged(m_currentValue);
				}

				return EventReply::handled();
			}
		}
		return m_editableText->onMouseWheel(geo, pos, scrollDelta);
	}

	EventReply SInputFieldInt::onDragOver(const Geometry& g, const Vec2& p, const DragDropPayload& pl) {
		return m_editableText->onDragOver(g, p, pl);
	}

	EventReply SInputFieldInt::onDrop(const Geometry& g, const Vec2& p, const DragDropPayload& pl) {
		return m_editableText->onDrop(g, p, pl);
	}

}
