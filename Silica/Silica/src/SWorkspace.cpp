#include "SWorkspace.h"

#include <algorithm>

namespace Silica {

	void SWorkspace::construct(const Args& args) {
		m_font = args.font;

		m_dockSpace = MakeWidget<SDockSpace>({
			.initialContent = args.initialContent,
			.onUndockWindow = [this](WidgetPtr content, Vec2 mousePos) {
				auto newWin = MakeWidget<SWindow>({
					.title = "Panel",
					.initialPosition = { mousePos.x - 150.0f, mousePos.y - 15.0f },
					.initialSize = { 300.0f, 200.0f },
					.font = m_font,
					.content = content
				});
				newWin->startDragging(mousePos);
				addFloatingWindow(newWin);
			}
		});
	}

	void SWorkspace::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		m_dockSpace->computeDesiredSize();
		for (auto& win : m_floatingWindows) win->computeDesiredSize();
	}

	void SWorkspace::arrangeChildren(const Geometry & allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		m_dockSpace->arrangeChildren(allocatedGeometry);
		for (auto& win : m_floatingWindows) win->arrangeChildren(allocatedGeometry);
	}

	void SWorkspace::onDraw(DrawList & outDrawList, const Geometry & allocatedGeometry) const {
		// -- Draw DockSpace First (Background) --
		m_dockSpace->onDraw(outDrawList, allocatedGeometry);

		// -- Draw Floating Windows On Top --
		for (auto& win : m_floatingWindows) {
			win->onDraw(outDrawList, win->getAllocatedGeometry());
		}
	}

	EventReply SWorkspace::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		EventReply reply = EventReply::unhandled();
		for (auto it = m_floatingWindows.rbegin(); it != m_floatingWindows.rend(); ++it) {
			if ((*it)->onMouseMove((*it)->getAllocatedGeometry(), mousePos).isHandled) {
				reply = EventReply::handled();
				break;
			}
		}
		if (!reply.isHandled) {
			reply = m_dockSpace->onMouseMove(m_dockSpace->getAllocatedGeometry(), mousePos);
		}
		return reply;
	}

	EventReply SWorkspace::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		// -- Check Floating Windows from Top to Bottom --
		for (auto it = m_floatingWindows.rbegin(); it != m_floatingWindows.rend(); ++it) {
			if ((*it)->onMouseButtonDown((*it)->getAllocatedGeometry(), mousePos).isHandled) {
				auto win = *it;

				auto eraseIt = std::find(m_floatingWindows.begin(), m_floatingWindows.end(), win);
				m_floatingWindows.erase(eraseIt);
				m_floatingWindows.push_back(win);
				return EventReply::handled();
			}
		}

		// -- Fallback to DockSpace --
		return m_dockSpace->onMouseButtonDown(m_dockSpace->getAllocatedGeometry(), mousePos);
	}

	EventReply SWorkspace::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		EventReply reply = EventReply::unhandled();
		for (auto it = m_floatingWindows.rbegin(); it != m_floatingWindows.rend(); ++it) {
			if ((*it)->onMouseButtonUp((*it)->getAllocatedGeometry(), mousePos).isHandled) {
				reply = EventReply::handled();
				break;
			}
		}
		if (!reply.isHandled) reply = m_dockSpace->onMouseButtonUp(m_dockSpace->getAllocatedGeometry(), mousePos);
		return reply;
	}

	EventReply SWorkspace::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (auto it = m_floatingWindows.rbegin(); it != m_floatingWindows.rend(); ++it) {
			if ((*it)->onMouseWheel((*it)->getAllocatedGeometry(), mousePos, scrollDelta).isHandled) return EventReply::handled();
		}
		return m_dockSpace->onMouseWheel(m_dockSpace->getAllocatedGeometry(), mousePos, scrollDelta);
	}

	void SWorkspace::addFloatingWindow(std::shared_ptr<SWindow> window) {
		window->onDragMove = [this](Vec2 mousePos) {
			m_dockSpace->updateDragDropPreview(mousePos, true);
		};

		window->onDragEnd = [this, window](Vec2 mousePos) {
			if (m_dockSpace->processDrop(window->getContent())) {
				window->setContent(nullptr);
				auto it = std::find(m_floatingWindows.begin(), m_floatingWindows.end(), window);
				if (it != m_floatingWindows.end()) m_floatingWindows.erase(it);
			}
			m_dockSpace->updateDragDropPreview(mousePos, false);
		};

		m_floatingWindows.push_back(window);
	}

}
