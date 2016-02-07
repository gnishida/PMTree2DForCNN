#include "PMTree2D.h"
#include "Utils.h"
#include <sstream>
#include <iostream>
#include "RenderManager.h"
#include "Camera.h"

namespace pmtree {

	const float M_PI = 3.1415926535f;
	const int NUM_SEGMENTS = 30;
	const int NUM_LEVELS = 1;

	/**
	* Shape ratioを返却する。
	* 論文Cretion and rendering of realistic treesの4.3節に記載されている内容に基づく。
	*
	* @param shape		shape id
	* @param ratio		ratio
	* @return			shape ratio
	*/
	float shapeRatio(int shape, float ratio) {
		if (shape == 0) {
			return 0.2f + 0.8f * ratio;
		}
		else if (shape == 1) {
			return 0.2f + 0.8f * sinf(M_PI * ratio);
		}
		else if (shape == 2) {
			return 0.2f + 0.8f * sinf(0.5f * M_PI * ratio);
		}
		else if (shape == 3) {
			return 1.0f;
		}
		else if (shape == 4) {
			return 0.5f + 0.5f * ratio;
		}
		else if (shape == 5) {
			if (ratio <= 0.7f) {
				return ratio / 0.7f;
			}
			else {
				return (1.0f - ratio) / 0.3f;
			}
		}
		else if (shape == 6) {
			return 1.0f - 0.8f * ratio;
		}
		else if (shape == 7) {
			if (ratio <= 0.7f) {
				return 0.5f + 0.5f * ratio / 0.7f;
			}
			else {
				return 0.5f + 0.5f * (1.0f - ratio) / 0.3f;
			}
		}
		else {
			return 0.0f;
		}
	}

	TreeNode::TreeNode(boost::shared_ptr<TreeNode> parent, int level, int index) {
		this->parent = parent;
		this->level = level;
		this->index = index;

		baseFactor = 0.0f;
		attenuationFactor = 0.0f;

		downAngle = 0.0f;
		curve = 0.0f;
		curveBack = 0.0f;
		for (int k = 0; k < NUM_SEGMENTS - 1; ++k) {
			curvesV.push_back(0);
		}

		for (int k = 0; k < NUM_SEGMENTS - 1; ++k) {
			branching.push_back(0);
		}
	}

	void TreeNode::generateRandom() {
		if (level == 0) {
			baseFactor = utils::uniform(0.0f, 0.5f);
		}
		
		if (level == 0) {
			attenuationFactor = 1.0f;
		}
		else if (level == 1) {
			attenuationFactor = utils::uniform(0.5f, 0.8f) * shapeRatio(7, (NUM_SEGMENTS - index) / (NUM_SEGMENTS * (1.0f - baseFactor)));
		}
		else {
			attenuationFactor = utils::uniform(0.3f, 0.6f) * (NUM_SEGMENTS - index * 0.9f) / NUM_SEGMENTS;
		}

		if (level > 0) {
			downAngle = utils::uniform(-90, 90);
		}
		curve = utils::uniform(-90, 90);
		curveBack = utils::uniform(-90, 90);
		for (int k = 0; k < NUM_SEGMENTS - 1; ++k) {
			curvesV[k] = utils::uniform(-5, 5);
		}

		if (level < NUM_LEVELS - 1) {
			// 枝分かれをランダムに選択
			for (int k = 0; k < NUM_SEGMENTS - 1; ++k) {
				branching[k] = utils::uniform(0, 2);

				// 根元部分なら、枝分かれしない
				if (k < NUM_SEGMENTS * baseFactor) branching[k] = 0;
			}
		}
	}

	std::string TreeNode::to_string() {
		std::stringstream ss;
		ss << "Level: " << level << ", Index: " << index;
		return ss.str();
	}

	PMTree2D::PMTree2D() {
		root = boost::shared_ptr<TreeNode>(new TreeNode(NULL, 0, 0));
	}

	void PMTree2D::generateRandom() {
		root = boost::shared_ptr<TreeNode>(new TreeNode(NULL, 0, 0));
		root->generateRandom();

		std::list<boost::shared_ptr<TreeNode> > queue;
		queue.push_back(root);

		// generate random param values for branches in the breadth-first order
		while (!queue.empty()) {
			boost::shared_ptr<TreeNode> node = queue.front();
			queue.pop_front();

			if (node->level < NUM_LEVELS - 1) {
				for (int k = 0; k < node->branching.size(); ++k) {
					boost::shared_ptr<TreeNode> child = boost::shared_ptr<TreeNode>(new TreeNode(node, node->level + 1, k));
					if (node->branching[k] == 1) {
						child->generateRandom();
					}
					node->children.push_back(child);
					queue.push_back(child);
				}
			}
		}
	}

	bool PMTree2D::generateGeometry(RenderManager* renderManager) {
		bool underground = false;

		glm::mat4 modelMat;
		float length = 0.3f;
		float width = 0.3f;

		std::vector<Vertex> vertices;
		if (generateGeometry(renderManager, modelMat, length, width, root, vertices)) underground = true;
		renderManager->addObject("tree", "", vertices, true);

		return underground;
	}

	bool PMTree2D::generateGeometry(RenderManager* renderManager, const glm::mat4& modelMat, float segment_length, float segment_width, boost::shared_ptr<TreeNode>& node, std::vector<Vertex>& vertices) {
		bool underground = false;

		glm::mat4 mat = modelMat;

		mat = glm::rotate(mat, node->downAngle / 180.0f * M_PI, glm::vec3(0, 0, 1));

		float w1 = segment_width;
		glm::vec4 p0(-w1 * 0.5, 0, 0, 1);
		glm::vec4 p1(w1 * 0.5, 0, 0, 1);
		p0 = mat * p0;
		p1 = mat * p1;

		for (int k = 0; k < NUM_SEGMENTS; ++k) {
			w1 = segment_width * (NUM_SEGMENTS - k) / NUM_SEGMENTS;
			float w2 = segment_width * (NUM_SEGMENTS - k - 1) / NUM_SEGMENTS;
			glm::vec4 p2(w2 * 0.5, segment_length, 0, 1);
			glm::vec4 p3(-w2 * 0.5, segment_length, 0, 1);
			p2 = mat * p2;
			p3 = mat * p3;

			if (p2.y < 0 || p3.y < 0) underground = true;

			std::vector<glm::vec3> pts(4);
			pts[0] = glm::vec3(p0);
			pts[1] = glm::vec3(p1);
			pts[2] = glm::vec3(p2);
			pts[3] = glm::vec3(p3);
			glutils::drawPolygon(pts, glm::vec4(0, 0, 0, 1), vertices);

			p0 = p3;
			p1 = p2;
			//glutils::drawTrapezoid(w1, w2, segment_length, glm::vec4(0, 0, 0, 1), mat, vertices);

			// 最後のsegmentなら、終了
			if (k >= NUM_SEGMENTS - 1) break;

			mat = glm::translate(mat, glm::vec3(0, segment_length, 0));

			if (node->branching[k] == 1) {
				generateGeometry(renderManager, mat, segment_length * node->children[k]->attenuationFactor, w1 * node->children[k]->attenuationFactor, node->children[k], vertices);
			}

			if (k < NUM_SEGMENTS / 2) {
				mat = glm::rotate(mat, (node->curve / NUM_SEGMENTS + node->curvesV[k]) / 180.0f * M_PI, glm::vec3(0, 0, 1));
			}
			else {
				mat = glm::rotate(mat, (node->curveBack / NUM_SEGMENTS + node->curvesV[k]) / 180.0f * M_PI, glm::vec3(0, 0, 1));
			}
		}

		return underground;
	}

	void PMTree2D::generateLocalTrainingData(const cv::Mat& image, Camera* camera, int screenWidth, int screenHeight, std::vector<cv::Mat>& localImages, std::vector<std::vector<float> >& parameters) {
		generateLocalTrainingData(glm::mat4(), 0.3f, root, image, camera, screenWidth, screenHeight, localImages, parameters);
	}

	void PMTree2D::generateLocalTrainingData(const glm::mat4& modelMat, float segment_length, boost::shared_ptr<TreeNode>& node, const cv::Mat& image, Camera* camera, int screenWidth, int screenHeight, std::vector<cv::Mat>& localImages, std::vector<std::vector<float> >& parameters) {
		cv::imwrite("original_image.jpg", image);


		glm::mat4 mat = modelMat;

		// current positionを計算
		glm::vec4 p(0, 0, 0, 1);
		p = mat * p;
		p = camera->mvpMatrix * p;
		glm::vec2 pp((p.x / p.w + 1.0f) * 0.5f * screenWidth, screenHeight - (p.y / p.w + 1.0f) * 0.5f * screenHeight);

		// matから、回転角度を抽出
		float theta = asinf(mat[0][1]);

		// 画像を回転してcropping
		cv::Mat localImage;
		cv::Mat affineMatrix = cv::getRotationMatrix2D(cv::Point2d(pp.x, pp.y), -theta / M_PI * 180, 1.0);
		cv::warpAffine(image, localImage, affineMatrix, image.size());
		localImages.push_back(localImage);

		cv::imwrite("local_image.jpg", localImage);

		// パラメータを格納
		std::vector<float> params;
		params.push_back(node->baseFactor);
		params.push_back(node->attenuationFactor);
		params.push_back(node->downAngle);
		params.push_back(node->curve);
		params.push_back(node->curveBack);
		for (int k = 0; k < node->curvesV.size(); ++k) {
			params.push_back(node->curvesV[k]);
		}
		for (int k = 0; k < node->branching.size(); ++k) {
			params.push_back(node->branching[k]);
		}
		parameters.push_back(params);

		// 子ノードの枝へ、再起処理
		for (int k = 0; k < NUM_SEGMENTS; ++k) {
			// 最後のsegmentなら、終了
			if (k >= NUM_SEGMENTS - 1) break;

			mat = glm::translate(mat, glm::vec3(0, segment_length, 0));

			if (node->branching[k] == 1) {
				generateLocalTrainingData(mat, segment_length * node->children[k]->attenuationFactor, node->children[k], image, camera, screenWidth, screenHeight, localImages, parameters);
			}

			if (k < NUM_SEGMENTS / 2) {
				mat = glm::rotate(mat, (node->curve / NUM_SEGMENTS + node->curvesV[k]) / 180.0f * M_PI, glm::vec3(0, 0, 1));
			}
			else {
				mat = glm::rotate(mat, (node->curveBack / NUM_SEGMENTS + node->curvesV[k]) / 180.0f * M_PI, glm::vec3(0, 0, 1));
			}

		}
	}

	std::string PMTree2D::to_string() {
		std::stringstream ss;
		bool first_node = true;

		std::list<boost::shared_ptr<TreeNode> > queue;
		queue.push_back(root);

		while (!queue.empty()) {
			boost::shared_ptr<TreeNode> node = queue.front();
			queue.pop_front();
			
			if (first_node) {
				first_node = false;
			}
			else {
				ss << ",";
			}
			
			ss << node->baseFactor << "," << node->attenuationFactor << "," << (node->downAngle + 90) / 180.0f << "," << (node->curve + 90) / 180.0f << "," << (node->curveBack + 90) / 180.0f;
			for (int i = 0; i < node->curvesV.size(); ++i) {
				ss << "," << (node->curvesV[i] + 5) / 10.0f;
			}
			for (int i = 0; i < node->branching.size(); ++i) {
				ss << "," << node->branching[i];
			}

			for (int i = 0; i < node->children.size(); ++i) {
				queue.push_back(node->children[i]);
			}
		}

		return ss.str();
	}

}