//
// Created by Henrique on 10/8/2019.
//

#ifndef GRAPHICSPROGRAMMINGEXERCISES_CAMERA_H
#define GRAPHICSPROGRAMMINGEXERCISES_CAMERA_H


class FirstPersonCamera{
public:
    FirstPersonCamera(float fov, float width, float height, float near, float far, glm::vec3 camPosition, glm::vec3 camForward, float linearSpeed = 30.f, float rotationGain = 800.0f){
        perspectiveFOV(fov, width, height, near, far);
        m_linearSpeed = linearSpeed;
        m_rotationGain = rotationGain;
        m_view = glm::lookAt(camPosition, camPosition + camForward, glm::vec3(0,1,0));
    }

    void perspectiveFOV(float fov, float width, float height, float near, float far){
        m_projection = glm::perspectiveFov(fov, width, height, near, far);
    }

    void moveCamera(glm::vec3 localDirection, float deltaTime){
        glm::vec3 deltaPos = glm::transpose(m_view) * glm::vec4(localDirection, 0);
        deltaPos.y = 0;
        m_camPosition += glm::normalize(deltaPos) * deltaTime * m_linearSpeed ;
        //view = glm::translate(view, translation);
        m_view = glm::lookAt(m_camPosition, m_camPosition + m_camForward, glm::vec3(0,1,0));
    }

    void rotateCamera(glm::vec2 cursorPosition, float deltaTime){

        // initialize with first value so that there is no jump at startup
        static glm::vec2 lastCursorPosition = cursorPosition;

        // compute the cursor position change
        auto positionDiff = cursorPosition - lastCursorPosition;

        // require a minimum threshold to rotate
        if (glm::dot(positionDiff, positionDiff) > .00001f){
            // rotate the forward vector around the Y axis, notices that w is set to 0 since it is a vector
            m_rotationAroundVertical += glm::radians(-positionDiff.x * m_rotationGain * deltaTime);
            m_camForward = glm::rotateY(m_rotationAroundVertical) * glm::vec4(0,0,-1,0);
            // rotate the forward vector around the lateral axis
            m_rotationAroundLateral +=  glm::radians(positionDiff.y * m_rotationGain * deltaTime);
            // we need to clamp the range of the rotation, otherwise forward and Y axes get parallel
            m_rotationAroundLateral = glm::clamp(m_rotationAroundLateral, -glm::half_pi<float>() * 0.9f, glm::half_pi<float>() * 0.9f);
            glm::vec3 lateralAxis = glm::cross(m_camForward, glm::vec3(0, 1,0));
            m_camForward = glm::rotate(m_rotationAroundLateral, lateralAxis) * glm::rotateY(m_rotationAroundVertical) * glm::vec4(0,0,-1,0);
            m_camForward = glm::normalize(m_camForward);

            // save current cursor position
            lastCursorPosition = cursorPosition;
        }
        m_view = glm::lookAt(m_camPosition, m_camPosition + m_camForward, glm::vec3(0,1,0));
    }

    //---
    glm::mat4 GetViewMatrix()
    {
        return m_view;
        return glm::lookAt(m_camPosition, m_camPosition + m_camForward, glm::vec3(0,1,0));
    }
    //--

    glm::mat4 getViewProjection(){
        return m_projection * m_view;
    }

    glm::mat4 getView(){
        return m_view;
    }

    glm::mat4 getProjection(){
        return m_projection;
    }

    glm::vec3 getForward(){
        return m_camForward;
    }

    glm::vec3 getPosition(){
        return m_camPosition;
    }

private:

    float m_linearSpeed = 1.5f;
    float m_rotationGain = 30.0f;

    glm::vec3 m_camForward = glm::vec3(.0f, .0f, -1.0f);
    glm::vec3 m_camPosition = glm::vec3(.0f, 1.6f, 0.0f);

    glm::mat4 m_projection = glm::mat4(1.0f);
    glm::mat4 m_view = glm::mat4(1.0f);

    float m_rotationAroundVertical = 0;
    float m_rotationAroundLateral = 0;

};

#endif //GRAPHICSPROGRAMMINGEXERCISES_CAMERA_H
