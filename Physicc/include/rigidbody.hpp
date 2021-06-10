#ifndef __RIGIDBODY_H__
#define __RIGIDBODY_H__

#include "../../Light/libs/glm/glm/glm.hpp"

class RigidBody
{
public:
    RigidBody(const glm::vec3& linearPosition, const glm::vec3& linearVelocity);

    inline glm::vec3 getLinearPosition() const
    {
        return m_linearPosition;
    }

    inline glm::vec3 getLinearVelocity() const
    {
        return m_linearVelocity;
    }

    inline void setLinearPosition(const glm::vec3& position)
    {
        m_linearPosition = position;
    }

    inline void setLinearVelocity(const glm::vec3& velocity)
    {
        m_linearVelocity = velocity;
    }

    void updatePosition(float time);

private:
    glm::vec3 m_linearPosition;
    glm::vec3 m_linearVelocity;
};

#endif // __RIGIDBODY_H__