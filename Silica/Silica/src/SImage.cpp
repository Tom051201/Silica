#include "SImage.h"

namespace Silica {

	void SImage::construct(const Args& args) {
		m_textureId = args.textureID;
		m_tintColor = args.tint.value_or(Color::white());
		m_desiredSize = args.desiredSize;
	}

	void SImage::arrangeChildren(const Geometry & allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SImage::onDraw(DrawList & outDrawList, const Geometry & allocatedGeometry) const {
		if (m_tintColor.a() == 0) return;

		outDrawList.pushTextureID(m_textureId);

		uint32_t startIndex = (uint32_t)outDrawList.vertices.size();

		outDrawList.vertices.push_back({ {allocatedGeometry.position.x, allocatedGeometry.position.y}, {0.0f, 0.0f}, m_tintColor });
		outDrawList.vertices.push_back({ {allocatedGeometry.position.x + allocatedGeometry.size.x, allocatedGeometry.position.y}, {1.0f, 0.0f}, m_tintColor });
		outDrawList.vertices.push_back({ {allocatedGeometry.position.x + allocatedGeometry.size.x, allocatedGeometry.position.y + allocatedGeometry.size.y}, {1.0f, 1.0f}, m_tintColor });
		outDrawList.vertices.push_back({ {allocatedGeometry.position.x, allocatedGeometry.position.y + allocatedGeometry.size.y}, {0.0f, 1.0f}, m_tintColor });

		outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 1); outDrawList.indices.push_back(startIndex + 2);
		outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 2); outDrawList.indices.push_back(startIndex + 3);

		if (outDrawList.commands.empty()) outDrawList.commands.push_back({ 0, 0, 0 });
		outDrawList.commands.back().indexCount += 6;

		outDrawList.popTextureID();
	}

}
