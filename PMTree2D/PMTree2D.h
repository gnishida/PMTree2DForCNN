#pragma once

#include <glm/glm.hpp>
#include "boost/shared_ptr.hpp"
#include <string>
#include <vector>
#include "Vertex.h"

class RenderManager;

namespace pmtree {
	float shapeRatio(int shape, float ratio);

	class TreeNode {
	public:
		float baseFactor;			// この枝の根元部分の割合
		float attenuationFactor;	// 親枝に対する長さの比率
		float downAngle;
		float curve;
		float curveBack;
		std::vector<float> curvesV;
		std::vector<int> branching;

		int level;
		int index;
		boost::shared_ptr<TreeNode> parent;
		std::vector<boost::shared_ptr<TreeNode> > children;

	public:
		TreeNode(boost::shared_ptr<TreeNode> parent, int level, int index);
		void generateRandom();
		std::string to_string();
	};

	class PMTree2D {
	private:
		boost::shared_ptr<TreeNode> root;

	public:
		PMTree2D();

		void generateRandom();
		bool generateGeometry(RenderManager* renderManager);
		std::string to_string();

	private:
		bool generateGeometry(RenderManager* renderManager, const glm::mat4& modelMat, float segment_length, float segment_width, boost::shared_ptr<TreeNode>& node, std::vector<Vertex>& vertices);
	};

}