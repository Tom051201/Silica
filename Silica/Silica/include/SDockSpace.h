#pragma once

#include <memory>
#include <optional>

#include "SWidget.h"

namespace Silica {

	struct SDockNode;
	using DockNodePtr = std::shared_ptr<SDockNode>;



	enum class SplitDirection {
		None,
		Horizontal,
		Vertical
	};



	enum class DockZone {
		None,
		Left, Right,
		Top, Bottom,
		Center
	};



	struct SDockNode {
		SplitDirection splitDirection = SplitDirection::None;
		float splitRatio = 0.5f;

		DockNodePtr child[2];
		WidgetPtr content = nullptr;

		Geometry allocatedGeometry;
		Rect splitterRect;
		Rect titleBarRect;
	};



	class SDockSpace : public SWidget {
	public:

		struct Args {
			WidgetPtr initialContent = nullptr;
			std::optional<Color> titleBarColor;
			std::function<void(WidgetPtr, Vec2)> onUndockWindow = nullptr;
		};

		void construct(const Args& args);

		void computeDesiredSize() override;
		void arrangeChildren(const Geometry& allocatedGeometry) override;
		void onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const override;

		EventReply onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) override;
		EventReply onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) override;
		EventReply onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) override;
		EventReply onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) override;

		void splitNode(DockNodePtr node, SplitDirection dir, float ratio, WidgetPtr newContent, bool insertFirst = false);
		DockNodePtr getRootNode() const { return m_rootNode; }
		void updateDragDropPreview(const Vec2& mousePos, bool isDragging);
		bool processDrop(WidgetPtr draggedContent);


	private:

		DockNodePtr m_rootNode;
		DockNodePtr m_draggingNode;
		DockNodePtr m_hoveredNode = nullptr;
		DockNodePtr m_previewNode = nullptr;
		float m_splitterThickness = 4.0f;
		DockZone m_previewZone = DockZone::None;
		std::function<void(WidgetPtr, Vec2)> m_onUndockWindow;
		Color m_titleBarColor;

		void arrangeNode(DockNodePtr node, const Geometry& geo);
		void drawNode(const DockNodePtr& node, DrawList& drawList) const;
		void addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const;
		DockNodePtr hitTestSplitter(const DockNodePtr& node, const Vec2& mousePos);
		DockNodePtr hitTestContentNode(const DockNodePtr& node, const Vec2& mousePos);
		DockNodePtr hitTestTitleBar(const DockNodePtr& node, const Vec2& mousePos);
		bool removeLeafNode(DockNodePtr parent, DockNodePtr target);
		void undockNode(DockNodePtr node, const Vec2& mousePos);

	};

}
