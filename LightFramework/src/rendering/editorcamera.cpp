#include "rendering/editorcamera.hpp"

#include "core/input.hpp"
#include "input/keycodes.hpp"
#include "input/mousecodes.hpp"


#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

namespace Light
{
	EditorCamera::EditorCamera(float fovy, float aspectRatio, float near, float far)
		: m_aspectRatio(aspectRatio),
		  m_fovy(fovy),
		  m_near(near),
		  m_far(far)
	{
		updateView();
		updateProjection();
		shadowCascadeLevels = {m_far/100.0f,m_far/70.0f,m_far/55.0f,m_far/25.0f,m_far/10.0f,m_far/2.0f};
	}

	void EditorCamera::setViewportSize(uint32_t width, uint32_t height)
	{
		m_viewportWidth = width;
		m_viewportHeight = height;
		m_aspectRatio = static_cast<float>(width) / height;
		updateProjection();
	}

	void EditorCamera::updateView()
	{
		m_position = calculatePosition();

		glm::quat orientation = getOrientation();
		m_viewMatrix = glm::translate(glm::mat4(1.0f), m_position) * glm::toMat4(orientation);
		m_viewMatrix = glm::inverse(m_viewMatrix);
	}

	void EditorCamera::updateProjection()
	{
		setProjectionMatrix(glm::perspective(glm::radians(m_fovy), m_aspectRatio, m_near, m_far));
	}

	void EditorCamera::recalculateOrientation()
	{

		auto invertedView = glm::inverse(m_viewMatrix);
		m_position = glm::vec3(invertedView[3]);

		const glm::vec3 direction = -glm::vec3(invertedView[2]);
		m_yaw   = glm::atan(direction.x, -direction.z);
		m_pitch = -glm::asin(direction.y);

		m_focalPoint = m_position + m_distance * getForwardDirection();
	}

	std::pair<float, float> EditorCamera::panSpeed() const
	{
		float x = std::min(m_viewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_viewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return {xFactor, yFactor};
	}

	float EditorCamera::rotationSpeed() const
	{
		return 0.8f;
	}

	float EditorCamera::zoomSpeed() const
	{
		float distance_ = m_distance * 0.2f;
		distance_ = std::max(distance_, 0.0f);
		float speed = distance_ * distance_;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::onUpdate(Timestep)
	{
		auto [mouseX, mouseY] = Input::getMousePos();
		const glm::vec2& mouse{mouseX, mouseY};
		glm::vec2 delta = (mouse - m_initialMousePos) * 0.003f;
		m_initialMousePos = mouse;

		if (Input::isKeyPressed(LIGHT_KEY_LEFT_ALT))
		{
			if (Input::isMouseButtonPressed(LIGHT_MOUSE_BUTTON_MIDDLE))
				mousePan(delta);
			else if (Input::isMouseButtonPressed(LIGHT_MOUSE_BUTTON_LEFT))
				mouseRotate(delta);
			else if (Input::isMouseButtonPressed(LIGHT_MOUSE_BUTTON_RIGHT))
				mouseZoom(delta.y);

			updateView();
		}
	}

	void EditorCamera::onEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(EditorCamera::onMouseScrolled));
	}

	bool EditorCamera::onMouseScrolled(MouseScrolledEvent& e)
	{
		auto [xOffset, yOffset] = e.getOffset();
		float delta = static_cast<float>(yOffset) * 0.1f;
		mouseZoom(delta);
		updateView();
		return false;
	}

	void EditorCamera::mousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = panSpeed();
		m_focalPoint += -getRightDirection() * delta.x * xSpeed * m_distance;
		m_focalPoint += getUpDirection() * delta.y * ySpeed * m_distance;
	}

	void EditorCamera::mouseRotate(const glm::vec2& delta)
	{
		float yawSign = getUpDirection().y < 0 ? -1.0f : 1.0f;
		m_yaw += yawSign * delta.x * rotationSpeed();
		m_pitch += delta.y * rotationSpeed();
	}

	void EditorCamera::mouseZoom(float delta)
	{
		m_distance -= delta * zoomSpeed();
		if (m_distance < 1.0f)
		{
			m_focalPoint += getForwardDirection();
			m_distance = 1.0f;
		}
	}

	glm::vec3 EditorCamera::getUpDirection() const
	{
		return glm::rotate(getOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::getRightDirection() const
	{
		return glm::rotate(getOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::getForwardDirection() const
	{
		return glm::rotate(getOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::calculatePosition() const
	{
		return m_focalPoint - getForwardDirection() * m_distance;
	}

	glm::quat EditorCamera::getOrientation() const
	{
		return glm::quat(glm::vec3(-m_pitch, -m_yaw, 0.0f));
	}

	std::vector<glm::vec4> EditorCamera::getFrustumCornersWorldSpace(const glm::mat4& projview)
{
    const auto inv = glm::inverse(projview);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}


std::vector<glm::vec4> EditorCamera::getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
    return getFrustumCornersWorldSpace(proj * view);
}

glm::mat4 EditorCamera::getLightSpaceMatrix(const float nearPlane, const float farPlane, glm::vec3 lightDir)
{
    const auto proj = glm::perspective(
        glm::radians(m_fovy), m_aspectRatio, nearPlane,
        farPlane);
    const auto corners = getFrustumCornersWorldSpace(proj, m_viewMatrix);

    glm::vec3 center = m_focalPoint;
    for (const auto& v : corners)
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
    for (const auto& v : corners)
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

    return lightProjection * lightView;
}

std::vector<glm::mat4> EditorCamera::getLightSpaceMatrices(glm::vec3 lightDir)
{
    std::vector<glm::mat4> ret;
    for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
    {
        if (i == 0)
        {
            ret.push_back(getLightSpaceMatrix(m_near, shadowCascadeLevels[i], lightDir));
        }
        else if (i < shadowCascadeLevels.size())
        {
            ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i],lightDir));
        }
        else
        {
            ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], m_far, lightDir));
        }
    }
    return ret;
}
}
