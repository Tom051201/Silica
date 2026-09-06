#include "silicapch.h"
#include "SInputFieldVec3Float.h"

#include "SHorizontalBox.h"
#include "STextBlock.h"
#include "SBox.h"
#include "SButton.h"
#include "SInputFieldFloat.h"

namespace Silica {

	void SInputFieldVec3Float::construct(const Args& args) {
		m_currentValue = args.initialValue;
		m_resetValue = args.resetValue;

		m_inputX = MakeWidget<SInputFieldFloat>({
			.initialValue = m_currentValue.x,
			.font = args.font,
			.onValueChanged = [this, args](float v) {
				m_currentValue.x = v;
				if (args.onValueChanged) args.onValueChanged(m_currentValue);
			},
			.onEditBegin = args.onEditBegin,
			.onEditComplete = args.onEditComplete,
		});

		m_inputY = MakeWidget<SInputFieldFloat>({
			.initialValue = m_currentValue.y,
			.font = args.font,
			.onValueChanged = [this, args](float v) {
				m_currentValue.y = v;
				if (args.onValueChanged) args.onValueChanged(m_currentValue);
			},
			.onEditBegin = args.onEditBegin,
			.onEditComplete = args.onEditComplete,
		});

		m_inputZ = MakeWidget<SInputFieldFloat>({
			.initialValue = m_currentValue.z,
			.font = args.font,
			.onValueChanged = [this, args](float v) {
				m_currentValue.z = v;
				if (args.onValueChanged) args.onValueChanged(m_currentValue);
			},
			.onEditBegin = args.onEditBegin,
			.onEditComplete = args.onEditComplete,
		});

		auto makeAxis = [&](const std::string& axis, Color color, std::shared_ptr<SInputFieldFloat> inputWidget, std::function<void()> onReset) {
			return MakeWidget<SHorizontalBox>({
				.slots = {
					{ {0,0}, MakeWidget<SButton>({
						.padding = { 6.0f, 5.0f },
						.color = color,
						.hoverColor = color,
						.pressedColor = color,
						.onClick = [onReset, args]() {
							if (args.onEditBegin) args.onEditBegin();
							onReset();
							if (args.onEditComplete) args.onEditComplete();
							return EventReply::handled();
						},
						.child = MakeWidget<STextBlock>({
							.text = axis,
							.font = args.font
						})
					})},
					{ {0,0}, inputWidget }
				}
			});
		};

		std::vector<Slot> rootSlots;
		if (!args.label.empty()) {
			rootSlots.push_back({ {0,0}, MakeWidget<SBox>({
				.explicitSize = Vec2(args.labelWidth, 0.0f),
				.backgroundColor = Color::transparent(),
				.child = MakeWidget<STextBlock>({.text = args.label, .font = args.font})
			}) });
		}

		rootSlots.push_back({ {0,0}, MakeWidget<SHorizontalBox>({
			.spacing = 0.0f,
			.slots = {
				{ {0,0}, makeAxis(args.firstText, args.firstColor, m_inputX, [this, args]() {
					m_currentValue.x = m_resetValue.x;
					m_inputX->setValue(m_currentValue.x);
					if (args.onValueChanged) args.onValueChanged(m_currentValue);
				})},
				{ {0,0}, makeAxis(args.secondText, args.secondColor, m_inputY, [this, args]() {
					m_currentValue.y = m_resetValue.y;
					m_inputY->setValue(m_currentValue.y);
					if (args.onValueChanged) args.onValueChanged(m_currentValue);
				})},
				{ {0,0}, makeAxis(args.thirdText, args.thirdColor, m_inputZ, [this, args]() {
					m_currentValue.z = m_resetValue.z;
					m_inputZ->setValue(m_currentValue.z);
					if (args.onValueChanged) args.onValueChanged(m_currentValue);
				})}
			}
		}) });

		m_rootAssembly = MakeWidget<SHorizontalBox>({
			.spacing = args.label.empty() ? 0.0f : 10.0f,
			.slots = rootSlots
		});
	}

	void SInputFieldVec3Float::computeDesiredSize() {
		m_rootAssembly->computeDesiredSize();
		m_desiredSize = m_rootAssembly->getDesiredSize();
	}

	void SInputFieldVec3Float::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		m_rootAssembly->arrangeChildren(allocatedGeometry);
	}

	void SInputFieldVec3Float::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		m_rootAssembly->onDraw(outDrawList, allocatedGeometry);
	}

	void SInputFieldVec3Float::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_rootAssembly) {
			m_rootAssembly->setRenderScale(scale);
		}
	}

	void SInputFieldVec3Float::setValue(const Vec3& newValue) {
		m_currentValue = newValue;
		if (m_inputX) m_inputX->setValue(newValue.x);
		if (m_inputY) m_inputY->setValue(newValue.y);
		if (m_inputZ) m_inputZ->setValue(newValue.z);
	}

	EventReply SInputFieldVec3Float::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		return m_rootAssembly->onMouseMove(allocatedGeometry, mousePos);
	}

	EventReply SInputFieldVec3Float::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		return m_rootAssembly->onMouseButtonDown(allocatedGeometry, mousePos, button);
	}

	EventReply SInputFieldVec3Float::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		return m_rootAssembly->onMouseButtonUp(allocatedGeometry, mousePos, button);
	}

	EventReply SInputFieldVec3Float::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		return m_rootAssembly->onMouseWheel(allocatedGeometry, mousePos, scrollDelta);
	}

	EventReply SInputFieldVec3Float::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		return m_rootAssembly->onDragOver(allocatedGeometry, mousePos, payload);
	}

	EventReply SInputFieldVec3Float::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		return m_rootAssembly->onDrop(allocatedGeometry, mousePos, payload);
	}

}
