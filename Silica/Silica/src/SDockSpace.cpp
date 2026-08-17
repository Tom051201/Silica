#include "SDockSpace.h"

#include <functional>
#include <algorithm>
#include <sstream>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SDockSpace::construct(const Args& args) {
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_rootNode = std::make_shared<SDockNode>();
		m_rootNode->tabs = args.initialTabs;
		m_onUndockWindow = args.onUndockWindow;
		m_titleBarColor = args.titleBarColor.value_or(GetTheme().Background_Panel);
	}

	void SDockSpace::computeDesiredSize() {
		m_desiredSize = Vec2::zero();

		std::function<void(DockNodePtr)> computeNode = [&](DockNodePtr node) {
			if (!node) return;

			if (node->splitDirection == SplitDirection::None) {
				for (auto& tab : node->tabs) {
					if (tab.content) tab.content->computeDesiredSize();
				}
			}
			else {
				computeNode(node->child[0]);
				computeNode(node->child[1]);
			}
		};

		computeNode(m_rootNode);
	}

	void SDockSpace::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		if (m_rootNode) {
			arrangeNode(m_rootNode, allocatedGeometry);
		}
	}

	void SDockSpace::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		outDrawList.addRect(allocatedGeometry, GetTheme().Surface_Tertiary);

		if (m_rootNode) {
			drawNode(m_rootNode, outDrawList);
		}

		// -- Draw Drag & Drop Preview Overlay --
		if (m_previewNode && m_previewZone != DockZone::None) {
			Geometry pGeo = m_previewNode->allocatedGeometry;

			if (m_previewZone == DockZone::Left) pGeo.size.x /= 2.0f;
			else if (m_previewZone == DockZone::Right) { pGeo.size.x /= 2.0f; pGeo.position.x += pGeo.size.x; }
			else if (m_previewZone == DockZone::Top) pGeo.size.y /= 2.0f;
			else if (m_previewZone == DockZone::Bottom) { pGeo.size.y /= 2.0f; pGeo.position.y += pGeo.size.y; }

			outDrawList.addRect(pGeo, Color(70, 130, 180, 128));
		}
	}

	void SDockSpace::setRenderScale(float scale) {
		m_renderScale = scale;

		std::function<void(DockNodePtr)> propagateScale = [&](DockNodePtr node) {
			if (!node) return;
			if (node->splitDirection == SplitDirection::None) {
				for (auto& tab : node->tabs) {
					if (tab.content) tab.content->setRenderScale(scale);
				}
			}
			else {
				propagateScale(node->child[0]);
				propagateScale(node->child[1]);
			}
		};
		propagateScale(m_rootNode);
	}

	EventReply SDockSpace::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		// -- Handle Tab Tearing --
		if (m_pressedTabNode) {
			if (std::abs(mousePos.x - m_pressedMousePos.x) > 5.0f || std::abs(mousePos.y - m_pressedMousePos.y) > 5.0f) {
				SWidget::setCapturedWidget(nullptr);
				undockNode(m_pressedTabNode, m_pressedTabIndex, mousePos);
				m_pressedTabNode = nullptr;
			}
			return EventReply::handled();
		}

		// -- Handle Splitter Dragging --
		if (m_draggingNode) {
			if (m_draggingNode->splitDirection == SplitDirection::Horizontal) {
				Platform::setCursor(Platform::Cursor::ResizeEW);
				float localX = mousePos.x - m_draggingNode->allocatedGeometry.position.x;
				float totalWidth = m_draggingNode->allocatedGeometry.size.x - m_splitterThickness;
				m_draggingNode->splitRatio = std::clamp(localX / totalWidth, 0.05f, 0.95f);
			}
			else if (m_draggingNode->splitDirection == SplitDirection::Vertical) {
				Platform::setCursor(Platform::Cursor::ResizeNS);
				float localY = mousePos.y - m_draggingNode->allocatedGeometry.position.y;
				float totalHeight = m_draggingNode->allocatedGeometry.size.y - m_splitterThickness;
				m_draggingNode->splitRatio = std::clamp(localY / totalHeight, 0.05f, 0.95f);
			}
			return EventReply::handled();
		}

		// -- Check Hovering Splitter --
		m_hoveredNode = hitTestSplitter(m_rootNode, mousePos);
		if (m_hoveredNode) {
			if (m_hoveredNode->splitterRect.contains(mousePos)) {
				Platform::setCursor(m_hoveredNode->splitDirection == SplitDirection::Horizontal ? Platform::Cursor::ResizeEW : Platform::Cursor::ResizeNS);
			}

			return EventReply::handled();
		}

		// -- Route Hover Events to UI Widgets --
		std::function<EventReply(DockNodePtr)> routeMove = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();
			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content) {
					return node->tabs[node->activeTab].content->onMouseMove(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos);
				}
			}
			else {
				EventReply rep = routeMove(node->child[1]);
				if (rep.isHandled) return rep;
				return routeMove(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeMove(m_rootNode);
	}

	EventReply SDockSpace::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button == MouseButton::Left) {
			// -- Check if Hit a Tab --
			auto hitTab = hitTestTab(m_rootNode, mousePos);
			if (hitTab.first) {
				DockNodePtr hitNode = hitTab.first;
				int hitIndex = hitTab.second;
				m_focusedNode = hitNode;

				// -- Check If X Clicked --
				if (hitNode->tabs[hitIndex].closeRect.contains(mousePos)) {
					closeTab(hitNode, hitIndex);
					return EventReply::handled();
				}

				// -- Tab Body Clicked --
				m_pressedTabNode = hitNode;
				m_pressedTabIndex = hitIndex;
				m_pressedMousePos = mousePos;
				SWidget::setCapturedWidget(this);
				return EventReply::handled();
			}

			// -- Check if Grabbed a Splitter --
			m_draggingNode = hitTestSplitter(m_rootNode, mousePos);
			if (m_draggingNode) {
				SWidget::setCapturedWidget(this);
				return EventReply::handled();
			}
		}

		// -- Route The Click to UI Widgets --
		std::function<EventReply(DockNodePtr)> routeDown = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();
			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content && node->tabs[node->activeTab].content->getAllocatedGeometry().contains(mousePos)) {
					m_focusedNode = node;
					return node->tabs[node->activeTab].content->onMouseButtonDown(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos, button);
				}
			}
			else {
				EventReply rep = routeDown(node->child[1]);
				if (rep.isHandled) return rep;
				return routeDown(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeDown(m_rootNode);
	}

	EventReply SDockSpace::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button == MouseButton::Left) {
			// -- Tab Hit --
			if (m_pressedTabNode) {
				m_pressedTabNode->activeTab = m_pressedTabIndex;
				m_pressedTabNode = nullptr;
				SWidget::setCapturedWidget(nullptr);
				return EventReply::handled();
			}

			// -- Dragging Node --
			if (m_draggingNode) {
				m_draggingNode = nullptr;
				SWidget::setCapturedWidget(nullptr);
				return EventReply::handled();
			}
		}

		// -- Route The Click to UI Widgets --
		std::function<EventReply(DockNodePtr)> routeUp = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();
			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content && node->tabs[node->activeTab].content->getAllocatedGeometry().contains(mousePos)) {
					return node->tabs[node->activeTab].content->onMouseButtonUp(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos, button);
				}
			}
			else {
				EventReply rep = routeUp(node->child[1]);
				if (rep.isHandled) return rep;
				return routeUp(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeUp(m_rootNode);
	}

	EventReply SDockSpace::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		// -- Route The Wheel Input to UI Widgets --
		std::function<EventReply(DockNodePtr)> routeWheel = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();
			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content && node->tabs[node->activeTab].content->getAllocatedGeometry().contains(mousePos)) {
					return node->tabs[node->activeTab].content->onMouseWheel(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos, scrollDelta);
				}
			}
			else {
				EventReply rep = routeWheel(node->child[1]);
				if (rep.isHandled) return rep;
				return routeWheel(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeWheel(m_rootNode);
	}

	void SDockSpace::splitNode(DockNodePtr node, SplitDirection dir, float ratio, std::string title, WidgetPtr newContent, bool insertFirst) {
		if (!node || node->splitDirection != SplitDirection::None) return;

		node->splitDirection = dir;
		node->splitRatio = ratio;
		node->child[0] = std::make_shared<SDockNode>();
		node->child[1] = std::make_shared<SDockNode>();

		if (insertFirst) {
			node->child[0]->tabs.push_back({ title, newContent, Rect() });
			node->child[1]->tabs = std::move(node->tabs);
			node->child[1]->activeTab = node->activeTab;
		}
		else {
			node->child[0]->tabs = std::move(node->tabs);
			node->child[0]->activeTab = node->activeTab;
			node->child[1]->tabs.push_back({ title, newContent, Rect() });
		}
		node->tabs.clear();
	}

	DockNodePtr SDockSpace::getRootNode() const {
		return m_rootNode; 
	}

	void SDockSpace::updateDragDropPreview(const Vec2& mousePos, bool isDragging) {
		if (!isDragging) {
			m_previewNode = nullptr;
			m_previewZone = DockZone::None;
			return;
		}

		m_previewNode = hitTestContentNode(m_rootNode, mousePos);
		if (m_previewNode) {
			float localX = (mousePos.x - m_previewNode->allocatedGeometry.position.x) / m_previewNode->allocatedGeometry.size.x;
			float localY = (mousePos.y - m_previewNode->allocatedGeometry.position.y) / m_previewNode->allocatedGeometry.size.y;

			if (localX < 0.25f) m_previewZone = DockZone::Left;
			else if (localX > 0.75f) m_previewZone = DockZone::Right;
			else if (localY < 0.25f) m_previewZone = DockZone::Top;
			else if (localY > 0.75f) m_previewZone = DockZone::Bottom;
			else m_previewZone = DockZone::Center;
		}
	}

	bool SDockSpace::processDrop(std::string title, WidgetPtr draggedContent) {
		if (m_previewNode && m_previewZone != DockZone::None) {

			if (m_previewZone == DockZone::Center || (m_previewNode->tabs.empty() && m_previewNode->splitDirection == SplitDirection::None)) {
				m_previewNode->tabs.push_back({ title, draggedContent, Rect() });
				m_previewNode->activeTab = (int)m_previewNode->tabs.size() - 1;
				m_previewNode = nullptr;
				m_previewZone = DockZone::None;
				return true;
			}

			SplitDirection dir = (m_previewZone == DockZone::Left || m_previewZone == DockZone::Right) ? SplitDirection::Horizontal : SplitDirection::Vertical;
			bool insertFirst = (m_previewZone == DockZone::Left || m_previewZone == DockZone::Top);

			splitNode(m_previewNode, dir, 0.5f, title, draggedContent, insertFirst);

			m_previewNode = nullptr;
			m_previewZone = DockZone::None;
			return true;
		}
		return false;
	}

	void SDockSpace::arrangeNode(DockNodePtr node, const Geometry& geo) {
		if (!node) return;

		node->allocatedGeometry = geo;

		if (node->splitDirection == SplitDirection::None) {
			float titleBarHeight = 20.0f * m_renderScale;
			node->titleBarRect = Rect(geo.position.x, geo.position.x + geo.size.x, geo.position.y, geo.position.y + titleBarHeight);

			std::vector<float> desiredWidths(node->tabs.size());
			float totalDesiredWidth = 0.0f;
			for (size_t i = 0; i < node->tabs.size(); ++i) {
				std::string displayTitle = node->tabs[i].title;
				size_t hashPos = displayTitle.find("##");
				if (hashPos != std::string::npos) {
					displayTitle = displayTitle.substr(0, hashPos);
				}

				float textWidth = 0.0f;
				if (m_font) {
					for (char c : displayTitle) textWidth += m_font->getGlyph(c).advanceX;
				}

				desiredWidths[i] = std::max(100.0f * m_renderScale, textWidth + (46.0f * m_renderScale));
				totalDesiredWidth += desiredWidths[i];
			}

			float availableWidth = geo.size.x;
			float scale = 1.0f;
			if (totalDesiredWidth > availableWidth && availableWidth > 0.0f) {
				scale = availableWidth / totalDesiredWidth;
			}

			float currentX = geo.position.x;
			for (size_t i = 0; i < node->tabs.size(); ++i) {
				float finalWidth = std::max(40.0f * m_renderScale, desiredWidths[i] * scale);
				node->tabs[i].hitRect = Rect(currentX, currentX + finalWidth, geo.position.y, geo.position.y + 20.0f);
				currentX += finalWidth;
			}

			Geometry contentGeo;
			contentGeo.position = { geo.position.x, geo.position.y + 20.0f };
			contentGeo.size = { geo.size.x, geo.size.y - 20.0f };
			if (contentGeo.size.y < 0) contentGeo.size.y = 0;

			for (size_t i = 0; i < node->tabs.size(); ++i) {
				if (node->tabs[i].content) {
					if (i == node->activeTab) {
						node->tabs[i].content->arrangeChildren(contentGeo);
					}
					else {
						node->tabs[i].content->arrangeChildren({ {0.0f, 0.0f}, {0.0f, 0.0f} });
					}
				}
			}
		}
		// -- Left Right --
		else if (node->splitDirection == SplitDirection::Horizontal) {
			float totalWidth = geo.size.x - m_splitterThickness;
			float leftWidth = totalWidth * node->splitRatio;
			float rightWidth = totalWidth - leftWidth;

			Geometry leftGeo = { geo.position, {leftWidth, geo.size.y} };
			Geometry rightGeo = { {geo.position.x + leftWidth + m_splitterThickness, geo.position.y}, {rightWidth, geo.size.y} };

			node->splitterRect = Rect(leftGeo.position.x + leftWidth, leftGeo.position.x + leftWidth + m_splitterThickness, geo.position.y, geo.position.y + geo.size.y);

			if (node->child[0]) arrangeNode(node->child[0], leftGeo);
			if (node->child[1]) arrangeNode(node->child[1], rightGeo);
		}
		// -- Top Bottom --
		else if (node->splitDirection == SplitDirection::Vertical) {
			float totalHeight = geo.size.y - m_splitterThickness;
			float topHeight = totalHeight * node->splitRatio;
			float bottomHeight = totalHeight - topHeight;

			Geometry topGeo = { geo.position, {geo.size.x, topHeight} };
			Geometry bottomGeo = { {geo.position.x, geo.position.y + topHeight + m_splitterThickness}, {geo.size.x, bottomHeight} };

			node->splitterRect = Rect(geo.position.x, geo.position.x + geo.size.x, topGeo.position.y + topHeight, topGeo.position.y + topHeight + m_splitterThickness);

			if (node->child[0]) arrangeNode(node->child[0], topGeo);
			if (node->child[1]) arrangeNode(node->child[1], bottomGeo);
		}
	}

	void SDockSpace::drawNode(const DockNodePtr& node, DrawList& drawList) const {
		if (!node) return;

		if (node->splitDirection == SplitDirection::None) {
			if (!node->tabs.empty()) {
				// -- Draw Entire Title Bar Background --
				Geometry tbGeo = { {node->titleBarRect.left, node->titleBarRect.top}, {node->titleBarRect.right - node->titleBarRect.left, node->titleBarRect.bottom - node->titleBarRect.top} };
				drawList.addRect(tbGeo, GetTheme().Surface_Tertiary);

				// -- Draw Each Tab --
				for (size_t i = 0; i < node->tabs.size(); ++i) {
					Geometry tGeo = { {node->tabs[i].hitRect.left, node->tabs[i].hitRect.top}, {node->tabs[i].hitRect.right - node->tabs[i].hitRect.left, node->tabs[i].hitRect.bottom - node->tabs[i].hitRect.top} };
					Color inactiveTabColor = GetTheme().Background_Panel;
					Color activeTabColor = GetTheme().Surface_Primary;
					Color tColor = (i == node->activeTab) ? activeTabColor : inactiveTabColor;

					if (i != node->activeTab) {
						tColor = Color(
							std::max(0, (int)tColor.r() - 20),
							std::max(0, (int)tColor.g() - 20),
							std::max(0, (int)tColor.b() - 20),
							tColor.a()
						);
					}

					drawList.addRect(tGeo, tColor);

					// -- Draw Tab Title --
					if (m_font && !node->tabs[i].title.empty()) {
						Vec2 textPos = { tGeo.position.x + (10.0f * m_renderScale), tGeo.position.y + (15.0f * m_renderScale) };

						std::string displayTitle = node->tabs[i].title;
						size_t hashPos = displayTitle.find("##");
						if (hashPos != std::string::npos) {
							displayTitle = displayTitle.substr(0, hashPos);
						}

						Rect textClip = Rect(tGeo.position.x, tGeo.position.x + tGeo.size.x - 25.0f, tGeo.position.y, tGeo.position.y + tGeo.size.y);
						drawList.pushClipRect(textClip);
						drawList.addText(m_font, displayTitle, textPos, GetTheme().Text_Main);

						drawList.popClipRect();
					}

					// -- Close X --
					float closeBoxSize = 16.0f * m_renderScale;
					Rect closeRect = Rect(
						tGeo.position.x + tGeo.size.x - closeBoxSize - (5.0f * m_renderScale),
						tGeo.position.x + tGeo.size.x - (5.0f * m_renderScale),
						tGeo.position.y + (2.0f * m_renderScale),
						tGeo.position.y + (2.0f * m_renderScale) + closeBoxSize
					);
					node->tabs[i].closeRect = closeRect;

					bool isHoveringX = closeRect.contains(Renderer::getMousePosition());

					if (isHoveringX) {
						Geometry highlightGeo = { {closeRect.left, closeRect.top}, {closeBoxSize, closeBoxSize} };
						drawList.addRect(highlightGeo, Color(255, 255, 255, 30));
					}

					Vec2 center = { std::round(closeRect.left + (closeBoxSize * 0.5f)), std::round(closeRect.top + (closeBoxSize * 0.5f)) };
					float crossSize = 3.0f;

					Color crossColor = isHoveringX ? GetTheme().Accent_Danger : GetTheme().Text_Dim;
					if (m_font) {
						const Glyph& g = m_font->getGlyph('x');
						Vec2 xPos = {
							std::round(closeRect.left + (closeBoxSize * 0.5f) - (g.advanceX * 0.5f)),
							std::round(closeRect.top + (closeBoxSize * 0.5f) - (g.size.y * 0.5f) - g.offset.y)
						};
						drawList.addText(m_font, "x", xPos, crossColor);
					}

					// -- Draw Focus Outline Around Active Tab --
					if (node == m_focusedNode && i == node->activeTab) {
						float t = 1.0f;
						Color c = GetTheme().Border_Selected;

						drawList.addRect({ {tGeo.position.x, tGeo.position.y}, {tGeo.size.x, t} }, c); // Top
						drawList.addRect({ {tGeo.position.x, tGeo.position.y + t}, {t, tGeo.size.y - t} }, c); // Left
						drawList.addRect({ {tGeo.position.x + tGeo.size.x - t, tGeo.position.y + t}, {t, tGeo.size.y - t} }, c); // Right
					}
				}

				// -- Draw Content --
				if (node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content) {
					node->tabs[node->activeTab].content->onDraw(drawList, node->tabs[node->activeTab].content->getAllocatedGeometry());
				}

				// -- Draw The Content Border & Split Top Line --
				if (node == m_focusedNode) {
					Geometry geo = node->allocatedGeometry;
					float t = 1.0f;
					Color c = GetTheme().Border_Selected;

					float contentY = node->titleBarRect.bottom;
					float contentHeight = geo.size.y - (contentY - geo.position.y);

					drawList.addRect({ {geo.position.x, contentY}, {t, contentHeight} }, c); // Left
					drawList.addRect({ {geo.position.x + geo.size.x - t, contentY}, {t, contentHeight} }, c); // Right
					drawList.addRect({ {geo.position.x, geo.position.y + geo.size.y - t}, {geo.size.x, t} }, c); // Bottom

					if (node->activeTab < node->tabs.size()) {
						float activeTabLeft = node->tabs[node->activeTab].hitRect.left;
						float activeTabRight = node->tabs[node->activeTab].hitRect.right;

						// -- Left Side Of Gap --
						if (activeTabLeft > geo.position.x) {
							float width = activeTabLeft - geo.position.x;
							drawList.addRect({ {geo.position.x, contentY}, {width, t} }, c);
						}

						// -- Right Side Of Gap --
						if (activeTabRight < geo.position.x + geo.size.x) {
							float width = (geo.position.x + geo.size.x) - activeTabRight;
							drawList.addRect({ {activeTabRight, contentY}, {width, t} }, c);
						}
					}
				}
			}
		}
		else {
			Geometry splitterGeo = { {node->splitterRect.left, node->splitterRect.top}, {node->splitterRect.right - node->splitterRect.left, node->splitterRect.bottom - node->splitterRect.top} };
			Color sColor = GetTheme().Background_Panel;
			if (m_draggingNode == node) sColor = GetTheme().Accent_Primary;
			else if (m_hoveredNode == node) sColor = Color(120, 120, 120, 255);
			drawList.addRect(splitterGeo, sColor);

			// -- Recurse --
			if (node->child[0]) drawNode(node->child[0], drawList);
			if (node->child[1]) drawNode(node->child[1], drawList);
		}
	}

	DockNodePtr SDockSpace::hitTestSplitter(const DockNodePtr& node, const Vec2& mousePos) {
		if (!node || node->splitDirection == SplitDirection::None) return nullptr;

		Rect hitRect = node->splitterRect;
		if (node->splitDirection == SplitDirection::Horizontal) {
			hitRect.left -= 6.0f;
			hitRect.right += 6.0f;
		}
		else {
			hitRect.top -= 6.0f;
			hitRect.bottom += 6.0f;
		}

		if (hitRect.contains(mousePos)) return node;

		auto leftHit = hitTestSplitter(node->child[0], mousePos);
		if (leftHit) return leftHit;

		return hitTestSplitter(node->child[1], mousePos);
	}

	DockNodePtr SDockSpace::hitTestContentNode(const DockNodePtr& node, const Vec2& mousePos) {
		if (!node || !node->allocatedGeometry.contains(mousePos)) return nullptr;

		if (node->splitDirection == SplitDirection::None) return node;

		auto leftHit = hitTestContentNode(node->child[0], mousePos);
		if (leftHit) return leftHit;

		return hitTestContentNode(node->child[1], mousePos);
	}

	std::pair<DockNodePtr, int> SDockSpace::hitTestTab(const DockNodePtr& node, const Vec2& mousePos) {
		if (!node) return { nullptr, -1 };
		if (node->splitDirection == SplitDirection::None) {
			for (size_t i = 0; i < node->tabs.size(); ++i) {
				if (node->tabs[i].hitRect.contains(mousePos)) return { node, (int)i };
			}
			return { nullptr, -1 };
		}
		auto leftHit = hitTestTab(node->child[0], mousePos);
		if (leftHit.first) return leftHit;
		return hitTestTab(node->child[1], mousePos);
	}

	bool SDockSpace::removeLeafNode(DockNodePtr parent, DockNodePtr target) {
		if (!parent || parent->splitDirection == SplitDirection::None) return false;

		if (parent->child[0] == target) {
			DockNodePtr survivor = parent->child[1];
			*parent = *survivor;
			return true;
		}
		if (parent->child[1] == target) {
			DockNodePtr survivor = parent->child[0];
			*parent = *survivor;
			return true;
		}

		if (removeLeafNode(parent->child[0], target)) return true;
		if (removeLeafNode(parent->child[1], target)) return true;
		return false;
	}

	void SDockSpace::undockNode(DockNodePtr node, int tabIndex, const Vec2& mousePos) {
		if (!node || tabIndex < 0 || tabIndex >= node->tabs.size()) return;

		DockTab savedTab = node->tabs[tabIndex];
		node->tabs.erase(node->tabs.begin() + tabIndex);
		if (node->activeTab >= node->tabs.size()) node->activeTab = std::max(0, (int)node->tabs.size() - 1);

		if (node->tabs.empty()) {
			if (node != m_rootNode) { removeLeafNode(m_rootNode, node); }
		}

		if (m_onUndockWindow) m_onUndockWindow(savedTab.title, savedTab.content, mousePos);
	}

	void SDockSpace::saveLayout(const std::filesystem::path& filepath) {
		std::ofstream out(filepath);
		if (!out.is_open()) return;

		out << "[Silica_Layout_v1]\n";
		serializeNode(out, m_rootNode, 0);
		out.close();
	}

	void SDockSpace::loadLayout(const std::filesystem::path& filepath) {
		std::ifstream in(filepath);
		if (!in.is_open()) return;

		std::string header;
		std::getline(in, header);
		if (header != "[Silica_Layout_v1]") return;

		m_focusedNode = nullptr;

		m_rootNode = deserializeNode(in);
		in.close();
	}

	void SDockSpace::serializeNode(std::ofstream& out, DockNodePtr node, int depth) {
		if (!node) return;

		std::string indent(depth * 2, ' ');

		if (node->splitDirection != SplitDirection::None) {
			out << indent << "Split "
				<< (node->splitDirection == SplitDirection::Horizontal ? "H " : "V ")
				<< node->splitRatio << "\n";

			serializeNode(out, node->child[0], depth + 1);
			serializeNode(out, node->child[1], depth + 1);
		}
		else {
			out << indent << "Tabs " << node->tabs.size() << " " << node->activeTab << "\n";
			for (const auto& tab : node->tabs) {
				out << indent << "  Tab " << tab.title << "\n";
			}
		}
	}

	DockNodePtr SDockSpace::deserializeNode(std::ifstream& in) {
		std::string line;
		while (std::getline(in, line)) {
			line = trim(line);
			if (!line.empty()) break;
		}

		if (line.empty()) return nullptr;

		DockNodePtr node = std::make_shared<SDockNode>();

		if (line.substr(0, 5) == "Split") {
			char dir;
			std::istringstream iss(line.substr(6));
			iss >> dir >> node->splitRatio;

			node->splitDirection = (dir == 'H') ? SplitDirection::Horizontal : SplitDirection::Vertical;
			node->child[0] = deserializeNode(in);
			node->child[1] = deserializeNode(in);
		}
		else if (line.substr(0, 4) == "Tabs") {
			node->splitDirection = SplitDirection::None;

			int tabCount = 0;
			std::istringstream iss(line.substr(5));
			iss >> tabCount >> node->activeTab;

			for (int i = 0; i < tabCount; ++i) {
				std::string tabLine;
				std::getline(in, tabLine);
				tabLine = trim(tabLine);

				if (tabLine.substr(0, 4) == "Tab ") {
					std::string title = tabLine.substr(4);

					WidgetPtr content = nullptr;
					auto it = m_widgetRegistry.find(title);
					if (it != m_widgetRegistry.end()) {
						content = it->second;
					}

					node->tabs.push_back({ title, content, Rect() });
				}
			}
		}

		return node;
	}

	std::string SDockSpace::trim(const std::string& str) {
		size_t first = str.find_first_not_of(' ');
		if (std::string::npos == first) return str;
		size_t last = str.find_last_not_of(' ');
		return str.substr(first, (last - first + 1));
	}

	void SDockSpace::registerTab(const std::string& title, WidgetPtr content) {
		m_widgetRegistry[title] = content;
	}

	void SDockSpace::closeTab(DockNodePtr node, int tabIndex) {
		if (!node || tabIndex < 0 || tabIndex >= node->tabs.size()) return;

		node->tabs.erase(node->tabs.begin() + tabIndex);

		if (node->activeTab >= node->tabs.size()) {
			node->activeTab = std::max(0, (int)node->tabs.size() - 1);
		}

		if (node->tabs.empty()) {
			if (node == m_rootNode) {}
			else {
				removeLeafNode(m_rootNode, node);
			}
		}
	}

	void SDockSpace::openTab(const std::string& title) {
		// -- Check If Already Open --
		std::function<bool(DockNodePtr)> isTabOpen = [&](DockNodePtr node) -> bool {
			if (!node) return false;
			if (node->splitDirection == SplitDirection::None) {
				for (const auto& tab : node->tabs) {
					if (tab.title == title) return true;
				}
				return false;
			}
			return isTabOpen(node->child[0]) || isTabOpen(node->child[1]);
			};

		if (isTabOpen(m_rootNode)) return;

		// -- Fetch From Registry --
		auto it = m_widgetRegistry.find(title);
		if (it == m_widgetRegistry.end()) return;

		WidgetPtr content = it->second;

		// -- Smart Placement --
		DockNodePtr targetNode = nullptr;
		float maxArea = -1.0f;

		std::function<void(DockNodePtr)> findBestNode = [&](DockNodePtr n) {
			if (!n) return;
			if (n->splitDirection == SplitDirection::None) {
				float area = n->allocatedGeometry.size.x * n->allocatedGeometry.size.y;
				if (area > maxArea) {
					maxArea = area;
					targetNode = n;
				}
			}
			else {
				findBestNode(n->child[0]);
				findBestNode(n->child[1]);
			}
		};

		findBestNode(m_rootNode);
		if (!targetNode) targetNode = m_rootNode;

		targetNode->tabs.push_back({ title, content, Rect(), Rect() });
		targetNode->activeTab = (int)targetNode->tabs.size() - 1;
	}

	std::vector<std::string> SDockSpace::getRegisteredTabNames() const {
		std::vector<std::string> names;
		for (const auto& pair : m_widgetRegistry) {
			names.push_back(pair.first);
		}
		std::sort(names.begin(), names.end());
		return names;
	}

	EventReply SDockSpace::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		// -- Route The Drag Hover to UI Widgets --
		std::function<EventReply(DockNodePtr)> routeDrag = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();

			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content && node->tabs[node->activeTab].content->getAllocatedGeometry().contains(mousePos)) {
					return node->tabs[node->activeTab].content->onDragOver(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos, payload);
				}
			}
			else {
				EventReply rep = routeDrag(node->child[1]);
				if (rep.isHandled) return rep;
				return routeDrag(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeDrag(m_rootNode);
	}

	EventReply SDockSpace::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		// -- Route The Drop to UI Widgets --
		std::function<EventReply(DockNodePtr)> routeDrop = [&](DockNodePtr node) -> EventReply {
			if (!node) return EventReply::unhandled();

			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab < node->tabs.size() && node->tabs[node->activeTab].content && node->tabs[node->activeTab].content->getAllocatedGeometry().contains(mousePos)) {
					return node->tabs[node->activeTab].content->onDrop(node->tabs[node->activeTab].content->getAllocatedGeometry(), mousePos, payload);
				}
			}
			else {
				EventReply rep = routeDrop(node->child[1]);
				if (rep.isHandled) return rep;
				return routeDrop(node->child[0]);
			}
			return EventReply::unhandled();
		};

		return routeDrop(m_rootNode);
	}

	void SDockSpace::focusTab(const std::string& title) {
		std::function<bool(DockNodePtr)> findAndFocus = [&](DockNodePtr node) -> bool {
			if (!node) return false;

			if (node->splitDirection == SplitDirection::None) {
				for (size_t i = 0; i < node->tabs.size(); ++i) {
					if (node->tabs[i].title == title) {
						node->activeTab = (int)i;
						m_focusedNode = node;
						return true;
					}
				}
				return false;
			}

			if (findAndFocus(node->child[0])) return true;
			return findAndFocus(node->child[1]);
		};

		findAndFocus(m_rootNode);
	}

	bool SDockSpace::isTabVisible(const std::string& title) const {
		std::function<bool(DockNodePtr)> checkVisible = [&](DockNodePtr node) -> bool {
			if (!node) return false;

			if (node->splitDirection == SplitDirection::None) {
				if (!node->tabs.empty() && node->activeTab >= 0 && node->activeTab < node->tabs.size()) {
					if (node->tabs[node->activeTab].title == title) {
						return true;
					}
				}
				return false;
			}

			return checkVisible(node->child[0]) || checkVisible(node->child[1]);
		};

		return checkVisible(m_rootNode);
	}

}
