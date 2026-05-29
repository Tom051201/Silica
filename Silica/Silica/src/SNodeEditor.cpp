#include "SNodeEditor.h"

#include <algorithm>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SNodeEditor::construct(const Args& args) {
		m_font = args.font;
		m_onBackgroundContextClick = args.onBackgroundContextClick;
		m_onNodeContextClick = args.onNodeContextClick;
	}

	void SNodeEditor::computeDesiredSize() {
		m_desiredSize = Vec2(400, 400);
	}

	void SNodeEditor::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		if (m_activeContextMenu) {
			m_activeContextMenu->arrangeChildren(m_contextMenuGeometry);

			Renderer::pushPopup(m_activeContextMenu, m_contextMenuGeometry, [this]() {
				m_activeContextMenu = nullptr;
			});
		}
	}

	void SNodeEditor::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Draw Background And Grid --
		addRectToDrawList(outDrawList, allocatedGeometry, Color(30, 30, 30, 255));

		outDrawList.pushClipRect(Rect(
			allocatedGeometry.position.x, allocatedGeometry.position.x + allocatedGeometry.size.x,
			allocatedGeometry.position.y, allocatedGeometry.position.y + allocatedGeometry.size.y
		));

		float gridSize = 50.0f * m_zoom;
		float offsetX = fmod(m_panOffset.x, gridSize);
		float offsetY = fmod(m_panOffset.y, gridSize);

		for (float x = offsetX; x < allocatedGeometry.size.x; x += gridSize) {
			float snapX = std::round(allocatedGeometry.position.x + x);
			outDrawList.addThickLine(
				{ snapX, allocatedGeometry.position.y },
				{ snapX, allocatedGeometry.position.y + allocatedGeometry.size.y },
				1.0f, Color(50, 50, 50, 255)
			);
		}
		for (float y = offsetY; y < allocatedGeometry.size.y; y += gridSize) {
			float snapY = std::round(allocatedGeometry.position.y + y);
			outDrawList.addThickLine(
				{ allocatedGeometry.position.x, snapY },
				{ allocatedGeometry.position.x + allocatedGeometry.size.x, snapY },
				1.0f, Color(50, 50, 50, 255)
			);
		}

		// -- Pre-Compute Pin Screen Positions --
		for (auto& node : const_cast<SNodeEditor*>(this)->m_nodes) {
			Vec2 screenPos = {
				allocatedGeometry.position.x + m_panOffset.x + (node.position.x * m_zoom),
				allocatedGeometry.position.y + m_panOffset.y + (node.position.y * m_zoom)
			};

			float pinY = screenPos.y + (40.0f * m_zoom);
			for (auto& pin : node.inputs) {
				pin.screenPosition = { screenPos.x, pinY };
				pinY += (20.0f * m_zoom);
			}

			pinY = screenPos.y + (40.0f * m_zoom);
			for (auto& pin : node.outputs) {
				pin.screenPosition = { screenPos.x + (node.size.x * m_zoom), pinY };
				pinY += (20.0f * m_zoom);
			}
		}

		// -- Draw Established Links --
		for (const auto& link : m_links) {
			NodePin* p1 = const_cast<SNodeEditor*>(this)->findPin(link.startPin);
			NodePin* p2 = const_cast<SNodeEditor*>(this)->findPin(link.endPin);
			if (p1 && p2) {
				Vec2 cp1 = { p1->screenPosition.x + (50.0f * m_zoom), p1->screenPosition.y };
				Vec2 cp2 = { p2->screenPosition.x - (50.0f * m_zoom), p2->screenPosition.y };

				float targetThickness = (link.id == m_selectedLinkID ? 5.0f : 3.0f) * m_zoom;
				float renderThickness = std::max(2.0f, targetThickness);

				Color wireColor = (link.id == m_selectedLinkID) ? Color(255, 165, 0, 255) : link.color;
				if (targetThickness < 2.0f) {
					float alphaScale = targetThickness / 2.0f;
					wireColor = Color(wireColor.r(), wireColor.g(), wireColor.b(), static_cast<uint8_t>(wireColor.a() * alphaScale));
				}

				outDrawList.addBezierCurve(p1->screenPosition, cp1, cp2, p2->screenPosition, renderThickness, wireColor);
			}
		}

		// -- Draw Nodes --
		for (auto& node : const_cast<SNodeEditor*>(this)->m_nodes) {
			Vec2 screenPos = {
				allocatedGeometry.position.x + m_panOffset.x + (node.position.x * m_zoom),
				allocatedGeometry.position.y + m_panOffset.y + (node.position.y * m_zoom)
			};

			// Node Body And Header Backgrounds
			Geometry nodeGeo = { screenPos, {node.size.x * m_zoom, node.size.y * m_zoom} };
			Geometry headerGeo = { screenPos, {node.size.x * m_zoom, 24.0f * m_zoom} };

			if (node.id == m_selectedNodeID) {
				Geometry outlineGeo = { {screenPos.x - 2.0f, screenPos.y - 2.0f}, {nodeGeo.size.x + 4.0f, nodeGeo.size.y + 4.0f} };
				addRectToDrawList(outDrawList, outlineGeo, Color(255, 165, 0, 255));
			}

			addRectToDrawList(outDrawList, nodeGeo, Color(45, 45, 45, 255));
			addRectToDrawList(outDrawList, headerGeo, node.headerColor);

			// Title
			drawText(outDrawList, node.title, { screenPos.x + (8.0f * m_zoom), screenPos.y + (16.0f * m_zoom) }, Color::white());

			// Draw Inputs
			float pinRadius = 4.0f * m_zoom;
			float pinSize = 8.0f * m_zoom;
			for (auto& pin : node.inputs) {
				addRectToDrawList(outDrawList, { {pin.screenPosition.x - pinRadius, pin.screenPosition.y - pinRadius}, {pinSize, pinSize} }, pin.color);
				drawText(outDrawList, pin.name, { pin.screenPosition.x + (12.0f * m_zoom), pin.screenPosition.y + pinRadius }, GetTheme().textMain);
			}

			// Draw Outputs
			for (auto& pin : node.outputs) {
				addRectToDrawList(outDrawList, { {pin.screenPosition.x - pinRadius, pin.screenPosition.y - pinRadius}, {pinSize, pinSize} }, pin.color);
				float textWidth = pin.name.length() * 7.0f * m_zoom;
				drawText(outDrawList, pin.name, { pin.screenPosition.x - textWidth - (12.0f * m_zoom), pin.screenPosition.y + pinRadius }, GetTheme().textMain);
			}
		}

		// -- Draw Currently Dragging Wire --
		if (m_draggingPinID != -1) {
			NodePin* startPin = const_cast<SNodeEditor*>(this)->findPin(m_draggingPinID);
			if (startPin) {
				Vec2 cp1 = { startPin->screenPosition.x + (startPin->type == PinType::Output ? 50.0f : -50.0f), startPin->screenPosition.y };
				Vec2 cp2 = { m_dragWireEndPos.x + (startPin->type == PinType::Output ? -50.0f : 50.0f), m_dragWireEndPos.y };

				float targetThickness = 5.0f * m_zoom;
				float renderThickness = std::max(2.0f, targetThickness);

				Color wireColor = Color(255, 165, 0, 255);
				if (targetThickness < 2.0f) {
					float alphaScale = targetThickness / 2.0f;
					wireColor = Color(wireColor.r(), wireColor.g(), wireColor.b(), static_cast<uint8_t>(wireColor.a() * alphaScale));
				}

				outDrawList.addBezierCurve(startPin->screenPosition, cp1, cp2, m_dragWireEndPos, renderThickness, wireColor);
			}
		}

		outDrawList.popClipRect();
	}

	EventReply SNodeEditor::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (!allocatedGeometry.contains(mousePos)) return EventReply::unhandled();

		// -- Panning --
		if (button == MouseButton::Right) {
			SWidget::setCapturedWidget(this);
			m_lastMousePos = mousePos;
			m_rightClickStartPos = mousePos;
			m_isPanning = true;
			return EventReply::handled();
		}

		if (button == MouseButton::Left) {
			SWidget::setFocusedWidget(this);

			// -- Check Pin Click --
			PinID hitPin = hitTestPins(mousePos);
			if (hitPin != -1) {
				NodePin* clickedPin = findPin(hitPin);

				if (clickedPin) {
					if (clickedPin->type == PinType::Input) {
						auto it = std::find_if(m_links.begin(), m_links.end(), [hitPin](const GraphLink& l) {
							return l.endPin == hitPin;
						});

						if (it != m_links.end()) {
							m_draggingPinID = it->startPin;
							m_dragWireEndPos = mousePos;
							m_links.erase(it);

							return EventReply::handled();
						}
					}

					m_draggingPinID = hitPin;
					m_dragWireEndPos = mousePos;
				}

				return EventReply::handled();
			}

			// -- Check Node Header --
			NodeID hitNode = hitTestNodes(mousePos);
			if (hitNode != -1) {
				m_selectedNodeID = hitNode;
				m_selectedLinkID = -1;
				m_draggingNodeID = hitNode;

				GraphNode* node = findNode(hitNode);
				Vec2 screenPos = {
					allocatedGeometry.position.x + m_panOffset.x + (node->position.x * m_zoom),
					allocatedGeometry.position.y + m_panOffset.y + (node->position.y * m_zoom)
				};
				m_nodeDragOffset = { mousePos.x - screenPos.x, mousePos.y - screenPos.y };

				for (size_t i = 0; i < m_nodes.size(); ++i) {
					if (m_nodes[i].id == hitNode) {
						GraphNode temp = m_nodes[i];
						m_nodes.erase(m_nodes.begin() + i);
						m_nodes.push_back(temp);
						break;
					}
				}
				return EventReply::handled();
			}

			// -- Check Wires --
			LinkID hitLink = hitTestLinks(mousePos);
			if (hitLink != -1) {
				m_selectedLinkID = hitLink;
				m_selectedNodeID = -1;
				return EventReply::handled();
			}

			// -- Clicked Empty Space --
			m_selectedNodeID = -1;
			m_selectedLinkID = -1;
			return EventReply::handled();

		}

		return EventReply::unhandled();
	}

	EventReply SNodeEditor::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		bool handled = false;

		// -- Handle Panning --
		if (m_isPanning) {
			m_panOffset.x += (mousePos.x - m_lastMousePos.x);
			m_panOffset.y += (mousePos.y - m_lastMousePos.y);
			handled = true;
		}

		// -- Handle Node Dragging --
		if (m_draggingNodeID != -1) {
			GraphNode* node = findNode(m_draggingNodeID);
			if (node) {
				node->position.x = (mousePos.x - allocatedGeometry.position.x - m_panOffset.x - m_nodeDragOffset.x) / m_zoom;
				node->position.y = (mousePos.y - allocatedGeometry.position.y - m_panOffset.y - m_nodeDragOffset.y) / m_zoom;
			}
			handled = true;
		}

		// -- Handle Wire Dragging --
		if (m_draggingPinID != -1) {
			m_dragWireEndPos = mousePos;
			handled = true;
		}

		// -- Update Last Mouse Position --
		if (handled) {
			m_lastMousePos = mousePos;
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SNodeEditor::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button == MouseButton::Left) {
			m_draggingNodeID = -1;

			if (m_draggingPinID != -1) {
				PinID droppedPin = hitTestPins(mousePos);
				if (droppedPin != -1 && droppedPin != m_draggingPinID) {
					NodePin* p1 = findPin(m_draggingPinID);
					NodePin* p2 = findPin(droppedPin);

					// -- Only Connect Output To Input --
					if (p1 && p2 && p1->type != p2->type) {
						PinID outPin = (p1->type == PinType::Output) ? p1->id : p2->id;
						PinID inPin = (p1->type == PinType::Input) ? p1->id : p2->id;
						addLink((LinkID)m_links.size() + 1, outPin, inPin, p1->color);
					}
				}
				m_draggingPinID = -1;
			}
		}

		if (button == MouseButton::Right) {
			m_isPanning = false;
			float distSq = (mousePos.x - m_rightClickStartPos.x) * (mousePos.x - m_rightClickStartPos.x) + (mousePos.y - m_rightClickStartPos.y) * (mousePos.y - m_rightClickStartPos.y);

			if (distSq < 9.0f) {
				NodeID hitNode = hitTestNodes(mousePos);

				if (hitNode != -1 && m_onNodeContextClick) {
					m_activeContextMenu = m_onNodeContextClick(hitNode, mousePos);
					if (m_activeContextMenu) {
						m_activeContextMenu->computeDesiredSize();
						m_contextMenuGeometry = { mousePos, m_activeContextMenu->getDesiredSize() };
					}
				}
				else if (hitNode == -1 && m_onBackgroundContextClick) {
					m_activeContextMenu = m_onBackgroundContextClick(mousePos);
					if (m_activeContextMenu) {
						m_activeContextMenu->computeDesiredSize();
						m_contextMenuGeometry = { mousePos, m_activeContextMenu->getDesiredSize() };
					}
				}
			}

			if (m_draggingPinID == -1 && m_draggingNodeID == -1) {
				SWidget::setCapturedWidget(nullptr);
			}

			return EventReply::handled();
		}

		if (!m_isPanning && m_draggingNodeID == -1 && m_draggingPinID == -1) {
			SWidget::setCapturedWidget(nullptr);
		}

		return EventReply::handled();
	}

	EventReply SNodeEditor::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (!allocatedGeometry.contains(mousePos)) return EventReply::unhandled();

		float zoomSpeed = 0.1f;
		float oldZoom = m_zoom;

		m_zoom = std::clamp(m_zoom + (scrollDelta * zoomSpeed), 0.3f, 2.0f);

		Vec2 mouseLocal = { mousePos.x - allocatedGeometry.position.x, mousePos.y - allocatedGeometry.position.y };
		Vec2 canvasPos = { (mouseLocal.x - m_panOffset.x) / oldZoom, (mouseLocal.y - m_panOffset.y) / oldZoom };

		m_panOffset.x = mouseLocal.x - (canvasPos.x * m_zoom);
		m_panOffset.y = mouseLocal.y - (canvasPos.y * m_zoom);

		return EventReply::handled();
	}

	EventReply SNodeEditor::onKeyDown(Key key) {
		if (key == Key::Delete || key == Key::Backspace) {

			// -- Delete Selected Link --
			if (m_selectedLinkID != -1) {
				m_links.erase(std::remove_if(m_links.begin(), m_links.end(), [this](const GraphLink& l) {
					return l.id == m_selectedLinkID;
				}), m_links.end());

				m_selectedLinkID = -1;
				return EventReply::handled();
			}

			// -- Delete Selected Node --
			if (m_selectedNodeID != -1) {
				m_links.erase(std::remove_if(m_links.begin(), m_links.end(), [this](const GraphLink& l) {
					GraphNode* n1 = nullptr; findPin(l.startPin, &n1);
					GraphNode* n2 = nullptr; findPin(l.endPin, &n2);
					return (n1 && n1->id == m_selectedNodeID) || (n2 && n2->id == m_selectedNodeID);
				}), m_links.end());

				m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(), [this](const GraphNode& n) {
					return n.id == m_selectedNodeID;
				}), m_nodes.end());

				m_selectedNodeID = -1;
				return EventReply::handled();
			}
		}

		return EventReply::unhandled();
	}

	void SNodeEditor::addNode(const GraphNode& node) {
		m_nodes.push_back(node);
	}

	void SNodeEditor::addLink(LinkID id, PinID start, PinID end, Color color) {
		m_links.push_back({ id, start, end, color });
	}

	GraphNode* SNodeEditor::findNode(NodeID id) {
		for (auto& n : m_nodes) {
			if (n.id == id) return &n;
		}

		return nullptr;
	}

	NodePin* SNodeEditor::findPin(PinID id, GraphNode** outNode) {
		for (auto& n : m_nodes) {
			for (auto& p : n.inputs) {
				if (p.id == id) {
					if (outNode) *outNode = &n;
					return &p;
				}
			}
			for (auto& p : n.outputs) {
				if (p.id == id) {
					if (outNode) *outNode = &n;
					return &p;
				}
			}
		}

		return nullptr;
	}

	PinID SNodeEditor::hitTestPins(const Vec2& mousePos) {
		float hitRadius = std::max(4.0f, 10.0f * m_zoom);

		for (auto& n : m_nodes) {
			for (auto& p : n.inputs) {
				if (std::abs(mousePos.x - p.screenPosition.x) < hitRadius &&
					std::abs(mousePos.y - p.screenPosition.y) < hitRadius) {
					return p.id;
				}
			}
			for (auto& p : n.outputs) {
				if (std::abs(mousePos.x - p.screenPosition.x) < hitRadius &&
					std::abs(mousePos.y - p.screenPosition.y) < hitRadius) {
					return p.id;
				}
			}
		}

		return -1;
	}

	NodeID SNodeEditor::hitTestNodes(const Vec2& mousePos) {
		for (auto it = m_nodes.rbegin(); it != m_nodes.rend(); ++it) {
			Vec2 screenPos = {
				m_allocatedGeometry.position.x + m_panOffset.x + (it->position.x * m_zoom),
				m_allocatedGeometry.position.y + m_panOffset.y + (it->position.y * m_zoom)
			};

			Rect nodeRect(
				screenPos.x,
				screenPos.x + (it->size.x * m_zoom),
				screenPos.y,
				screenPos.y + (it->size.y * m_zoom)
			);

			if (nodeRect.contains(mousePos)) return it->id;
		}

		return -1;
	}

	void SNodeEditor::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color, float rounding) const {
		uint32_t s = (uint32_t)drawList.vertices.size();

		drawList.vertices.push_back({ {geo.position.x, geo.position.y}, {0,0}, color });
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0,0}, color });
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0,0}, color });
		drawList.vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0,0}, color });
		drawList.indices.push_back(s + 0); drawList.indices.push_back(s + 1); drawList.indices.push_back(s + 2);
		drawList.indices.push_back(s + 0); drawList.indices.push_back(s + 2); drawList.indices.push_back(s + 3);

		if (drawList.commands.empty()) drawList.commands.push_back({ 0, 0, 0 });
		drawList.commands.back().indexCount += 6;
	}

	void SNodeEditor::drawText(DrawList& drawList, const std::string& text, Vec2 pos, Color color) const {
		if (!m_font) return;
		float curX = pos.x;
		for (char c : text) {
			const Glyph& g = m_font->getGlyph(c);
			if (g.size.x > 0) {
				float x0 = curX + (g.offset.x * m_zoom);
				float y0 = pos.y + (g.offset.y * m_zoom);
				float x1 = x0 + (g.size.x * m_zoom);
				float y1 = y0 + (g.size.y * m_zoom);
				uint32_t s = (uint32_t)drawList.vertices.size();

				drawList.vertices.push_back({ {x0,y0}, {g.uvMin.x, g.uvMin.y}, color });
				drawList.vertices.push_back({ {x1,y0}, {g.uvMax.x, g.uvMin.y}, color });
				drawList.vertices.push_back({ {x1,y1}, {g.uvMax.x, g.uvMax.y}, color });
				drawList.vertices.push_back({ {x0,y1}, {g.uvMin.x, g.uvMax.y}, color });
				drawList.indices.push_back(s + 0); drawList.indices.push_back(s + 1); drawList.indices.push_back(s + 2);
				drawList.indices.push_back(s + 0); drawList.indices.push_back(s + 2); drawList.indices.push_back(s + 3);

				if (drawList.commands.empty()) drawList.commands.push_back({ 0,0,0 });
				drawList.commands.back().indexCount += 6;
			}
			curX += (g.advanceX * m_zoom);
		}
	}

	LinkID SNodeEditor::hitTestLinks(const Vec2& mousePos) {
		float hitRadiusSq = (5.0f * m_zoom) * (5.0f * m_zoom);

		for (const auto& link : m_links) {
			NodePin* p1 = findPin(link.startPin);
			NodePin* p2 = findPin(link.endPin);
			if (!p1 || !p2) continue;

			Vec2 cp1 = { p1->screenPosition.x + (50.0f * m_zoom), p1->screenPosition.y };
			Vec2 cp2 = { p2->screenPosition.x - (50.0f * m_zoom), p2->screenPosition.y };

			int segments = 20;
			Vec2 lastPoint = p1->screenPosition;

			for (int i = 1; i <= segments; ++i) {
				float t = (float)i / (float)segments;
				float u = 1.0f - t;
				float tt = t * t, uu = u * u, uuu = uu * u, ttt = tt * t;

				Vec2 currentPoint = {
					uuu * p1->screenPosition.x + 3 * uu * t * cp1.x + 3 * u * tt * cp2.x + ttt * p2->screenPosition.x,
					uuu * p1->screenPosition.y + 3 * uu * t * cp1.y + 3 * u * tt * cp2.y + ttt * p2->screenPosition.y
				};

				float l2 = (currentPoint.x - lastPoint.x) * (currentPoint.x - lastPoint.x) + (currentPoint.y - lastPoint.y) * (currentPoint.y - lastPoint.y);
				if (l2 > 0.0f) {
					float t_proj = std::clamp(((mousePos.x - lastPoint.x) * (currentPoint.x - lastPoint.x) + (mousePos.y - lastPoint.y) * (currentPoint.y - lastPoint.y)) / l2, 0.0f, 1.0f);
					Vec2 proj = { lastPoint.x + t_proj * (currentPoint.x - lastPoint.x), lastPoint.y + t_proj * (currentPoint.y - lastPoint.y) };
					float distSq = (mousePos.x - proj.x) * (mousePos.x - proj.x) + (mousePos.y - proj.y) * (mousePos.y - proj.y);

					if (distSq < hitRadiusSq) return link.id;
				}
				lastPoint = currentPoint;
			}
		}

		return -1;
	}

}
