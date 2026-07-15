#include "SVector3FloatInput.h"

#include "SHorizontalBox.h"
#include "STextBlock.h"
#include "SBox.h"
#include "SFloatInput.h"

namespace Silica {

	void SVector3FloatInput::construct(const Args& args) {
		m_currentValue = args.initialValue;

		auto makeAxis = [&](const std::string& axis, Color color, float val, std::function<void(float)> onChange) {
			return MakeWidget<SHorizontalBox>({
				.spacing = 2.0f,
				.slots = {
					{ {0,0}, MakeWidget<SBox>({
						.padding = { 6.0f, 2.0f },
						.backgroundColor = color,
						.child = MakeWidget<STextBlock>({.text = axis, .font = args.font})
					})},
					{ {0,0}, MakeWidget<SFloatInput>({
						.initialValue = val,
						.font = args.font,
						.onValueChanged = onChange
					})}
				}
			});
		};

		m_rootAssembly = MakeWidget<SHorizontalBox>({
			.spacing = 10.0f,
			.slots = {
				{ {0,0}, MakeWidget<SBox>({
					.explicitSize = Vec2(args.labelWidth, 0.0f),
					.backgroundColor = Color::transparent(),
					.child = MakeWidget<STextBlock>({.text = args.label, .font = args.font})
				})},

				// -- X Axis (Red) --
				{ {0,0}, makeAxis(args.firstText, args.firstColor, m_currentValue.x, [this, args](float v) {
					m_currentValue.x = v;
					if (args.onValueChanged) args.onValueChanged(m_currentValue);
				})},

				// -- Y Axis (Green) --
				{ {0,0}, makeAxis(args.secondText, args.secondColor, m_currentValue.y, [this, args](float v) {
					m_currentValue.y = v;
					if (args.onValueChanged) args.onValueChanged(m_currentValue);
				})},

				// -- Z Axis (Blue) --
				{ {0,0}, makeAxis(args.thirdText, args.thirdColor, m_currentValue.z, [this, args](float v) {
					m_currentValue.z = v;
					if (args.onValueChanged) args.onValueChanged(m_currentValue);
				})}
			}
		});
	}

	void SVector3FloatInput::computeDesiredSize() {
		m_rootAssembly->computeDesiredSize();
		m_desiredSize = m_rootAssembly->getDesiredSize();
	}

	void SVector3FloatInput::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		m_rootAssembly->arrangeChildren(allocatedGeometry);
	}

	void SVector3FloatInput::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		m_rootAssembly->onDraw(outDrawList, allocatedGeometry);
	}

	EventReply SVector3FloatInput::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		return m_rootAssembly->onMouseMove(allocatedGeometry, mousePos);
	}

	EventReply SVector3FloatInput::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		return m_rootAssembly->onMouseButtonDown(allocatedGeometry, mousePos, button);
	}

	EventReply SVector3FloatInput::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		return m_rootAssembly->onMouseButtonUp(allocatedGeometry, mousePos, button);
	}

	EventReply SVector3FloatInput::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		return m_rootAssembly->onMouseWheel(allocatedGeometry, mousePos, scrollDelta);
	}

	EventReply SVector3FloatInput::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		return m_rootAssembly->onDragOver(allocatedGeometry, mousePos, payload);
	}

	EventReply SVector3FloatInput::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		return m_rootAssembly->onDrop(allocatedGeometry, mousePos, payload);
	}

}
