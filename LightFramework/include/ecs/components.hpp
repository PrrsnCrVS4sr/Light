#ifndef __COMPONENTS_HPP__
#define __COMPONENTS_HPP__

#include <memory>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "light/rendering/shader.hpp"
#include "light/rendering/buffer.hpp"
#include "light/rendering/mesh.hpp"
#include "light/rendering/texture.hpp"
#include "light/rendering/vertexarray.hpp"
#include "light/rendering/camera.hpp"
#include "light/rendering/lights.hpp"
#include "core/uuid.hpp"

namespace Light
{

	struct Component
	{
		std::string uuid;

		Component() : uuid(newUUID()) {}
	};

	struct TagComponent : public Component
	{
		std::string tag;

		TagComponent() = default;
		TagComponent(const TagComponent &) = default;
		TagComponent &operator=(const TagComponent &) = default;
		TagComponent(const std::string &tag) : tag(tag) {}
	};
	struct TransformComponent : public Component
	{
		TransformComponent(glm::vec3 position = glm::vec3(0, 0, 0),
						   glm::vec3 rotation = glm::vec3(0, 0, 0),
						   glm::vec3 scale = glm::vec3(0.5))
			: position(position), rotation(rotation), scale(scale) {}
		inline glm::mat4 getTransform() const
		{
			return glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::quat(rotation)) * glm::scale(glm::mat4(1.0f), glm::abs(scale));
		}
		inline glm::mat4 getViewMatrix() const
		{	
			glm::vec3 dir = glm::vec3(glm::normalize(this->getTransform() * glm::vec4(0.0, 0.0, 1.0, 0.0)));
			glm::mat4 mtr = glm::lookAt(dir, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
			return glm::lookAt(dir, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		}
		inline glm::mat4 getModel() const
		{
			glm::mat4 model = glm::mat4(1.0);
			model = glm::translate(model, position);
			model = glm::scale(model, scale);
			return model;
		}

		glm::mat4 getProjectionMatrix(std::vector<glm::vec4> corners,glm::vec3 lightDir)
		{
			
			

			glm::vec3 center = glm::vec3(0, 0, 0);
			for (const auto &v : corners)
			{
				center += glm::vec3(v);
			}
			center /= corners.size();
			

			const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

			float minX = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::min();
			float minY = std::numeric_limits<float>::max();
			float maxY = std::numeric_limits<float>::min();
			float minZ = std::numeric_limits<float>::max();
			float maxZ = std::numeric_limits<float>::min();
			for (const auto &v : corners)
			{
				const auto trf = lightView * v;
				minX = std::min(minX, trf.x);
				maxX = std::max(maxX, trf.x);
				minY = std::min(minY, trf.y);
				maxY = std::max(maxY, trf.y);
				minZ = std::min(minZ, trf.z);
				maxZ = std::max(maxZ, trf.z);
			}

			// Tune this parameter according to the scene
			constexpr float zMult = 10.0f;
			if (minZ < 0)
			{
				minZ *= zMult;
			}
			else
			{
				minZ /= zMult;
			}
			if (maxZ < 0)
			{
				maxZ /= zMult;
			}
			else
			{
				maxZ *= zMult;
			}
			
			const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

			//glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);

			return lightProjection * lightView;
		}
		
		inline glm::mat4 getProjectionMatrix() const
		{
			return glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);
		}
		inline glm::mat4 getSpaceMatrix() const
		{
			return getProjectionMatrix() * getViewMatrix();
		}

		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
	};

	struct MeshRendererComponent : public Component
	{
		MeshRendererComponent(const char *path);
		inline void bind()
		{
			shader->bind();
		}
		inline void setUniformInt(const std::string &name, int value)
		{
			shader->setUniformInt(name, value);
		}
		std::shared_ptr<Light::Shader> shader;
	};

	struct MeshComponent : public Component
	{
		MeshComponent(std::shared_ptr<Light::Mesh> mesh);
		std::shared_ptr<Light::Mesh> mesh;
	};

	struct LightComponent : public Component
	{
		LightComponent() : m_lightColor({1.0, 1.0, 1.0}) {}
		LightComponent(glm::vec3 lightColor) : m_lightColor(lightColor) {}
		glm::vec3 m_lightColor;
		LightType m_lightType = LightType::Directional;
		float m_inner = 12.5;
		float m_outer = 17.5;
		float m_range = 10.0;
	};

	struct CameraComponent : public Component
	{
		CameraComponent() = default;
		CameraComponent(glm::mat4 projectionMatrix) : m_camera(projectionMatrix) {}
		Camera m_camera;
	};
}

#endif // __COMPONENTS_HPP__
