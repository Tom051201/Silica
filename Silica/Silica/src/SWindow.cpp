#include "silicapch.h"
#include "SWindow.h"

#include "Renderer.h"

namespace Silica {

	void SWindow::construct(const Args& args) {
		m_title = args.title;
		m_position = args.initialPosition;
		m_size = args.initialSize;
		m_content = args.content;
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().Background_Panel);
		m_titleBarColor = args.titleBarColor.value_or(GetTheme().Background_Input);
		m_titleBarDraggingColor = args.titleBarDraggingColor.value_or(GetTheme().Accent_Primary);
		m_titleTextColor = args.titleTextColor.value_or(GetTheme().Text_Main);
	}

	void SWindow::computeDesiredSize() {
		m_desiredSize = m_size;
		if (m_content) m_content->computeDesiredSize();
	}

	void SWindow::arrangeChildren(const Geometry& allocatedGeometry) {
		m_allocatedGeometry.position = m_position;
		m_allocatedGeometry.size = m_size;

		if (m_content) {
			float scaledTitleHeight = 30.0f * m_renderScale;

			Geometry contentGeo;
			contentGeo.position.x = m_position.x;
			contentGeo.position.y = m_position.y + scaledTitleHeight;
			contentGeo.size.x = m_size.x;
			contentGeo.size.y = m_size.y - scaledTitleHeight;
			m_content->arrangeChildren(contentGeo);
		}
	}

	void SWindow::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		float scaledTitleHeight = 30.0f * m_renderScale;

		// -- Draw Window Background --
		outDrawList.addRect(m_allocatedGeometry, m_backgroundColor);

		// -- Draw Title Bar --
		Color titleColor = m_isDragging ? m_titleBarDraggingColor : m_titleBarColor;
		outDrawList.addRect({ m_position, {m_size.x, scaledTitleHeight} }, titleColor);

		if (m_font && !m_title.empty()) {
			Vec2 textPos = { m_position.x + (10.0f * m_renderScale), m_position.y + (20.0f * m_renderScale) };
			outDrawList.addText(m_font, m_title, textPos, m_titleTextColor, m_renderScale);
		}

		// -- Draw Content --
		if (m_content) {
			outDrawList.pushClipRect(Rect(m_position.x, m_position.x + m_size.x, m_position.y + scaledTitleHeight, m_position.y + m_size.y));
			m_content->onDraw(outDrawList, m_content->getAllocatedGeometry());
			outDrawList.popClipRect();
		}
	}

	void SWindow::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_content) {
			m_content->setRenderScale(scale);
		}
	}

	EventReply SWindow::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDragging) {
			m_position.x = mousePos.x - m_dragClickOffset.x;
			m_position.y = mousePos.y - m_dragClickOffset.y;
			if (onDragMove) onDragMove(mousePos);
			return EventReply::handled();
		}

		if (m_content) {
			EventReply reply = m_content->onMouseMove(m_content->getAllocatedGeometry(), mousePos);
			if (reply.isHandled) return reply;
		}

		if (m_allocatedGeometry.contains(mousePos)) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SWindow::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (getTitleBarRect().contains(mousePos) && button == MouseButton::Left) {
			m_isDragging = true;
			m_dragClickOffset = Vec2(mousePos.x - m_position.x, mousePos.y - m_position.y);
			SWidget::setCapturedWidget(this);
			return EventReply::handled();
		}

		if (m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseButtonDown(m_content->getAllocatedGeometry(), mousePos, button);
		}

		if (m_allocatedGeometry.contains(mousePos)) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SWindow::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_isDragging && button == MouseButton::Left) {
			m_isDragging = false;
			SWidget::setCapturedWidget(nullptr);
			if (onDragEnd) onDragEnd(mousePos);
			return EventReply::handled();
		}

		if (m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseButtonUp(m_content->getAllocatedGeometry(), mousePos, button);
		}

		return EventReply::unhandled();
	}

	void SWindow::startDragging(const Vec2& mousePos) {
		m_isDragging = true;
		m_dragClickOffset = Vec2(mousePos.x - m_position.x, mousePos.y - m_position.y);
		SWidget::setCapturedWidget(this);
	}

	Rect SWindow::getTitleBarRect() const {
		return Rect(m_position.x, m_position.x + m_size.x, m_position.y, m_position.y + (30.0f * m_renderScale));
	}

	bool SWindow::isDragging() const {
		return m_isDragging;
	}

	void SWindow::setContent(WidgetPtr content) {
		m_content = content;
	}

	WidgetPtr SWindow::getContent() const {
		return m_content;
	}

	const std::string& SWindow::getTitle() const {
		return m_title;
	}

	EventReply SWindow::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseWheel(m_content->getAllocatedGeometry(), mousePos, scrollDelta);
		}
		return EventReply::unhandled();
	}

	EventReply SWindow::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onDragOver(m_content->getAllocatedGeometry(), mousePos, payload);
		}
		return EventReply::unhandled();
	}

	EventReply SWindow::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onDrop(m_content->getAllocatedGeometry(), mousePos, payload);
		}
		return EventReply::unhandled();
	}

}
