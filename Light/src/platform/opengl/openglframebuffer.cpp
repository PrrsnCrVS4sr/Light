#include "light/platform/opengl/openglframebuffer.hpp"

#include "core/logging.hpp"
#include "core/assert.hpp"

#include "glad/glad.h"

namespace Light
{
	std::shared_ptr<Framebuffer> Framebuffer::create(const FramebufferSpec& spec) 
	{
		return std::make_shared<OpenGLFramebuffer>(spec);
	}

	static GLenum TexFormat2OpenGLFormat(FramebufferTextureFormat fmt)
	{
		switch (fmt)
		{
		case FramebufferTextureFormat::RGBA8:
			return GL_RGBA;
		case FramebufferTextureFormat::RED_INTEGER:
			return GL_RED_INTEGER;
		case FramebufferTextureFormat::DEPTH24_STENCIL8:
			return GL_DEPTH_STENCIL;
		default:
			LIGHT_CORE_ERROR("Unrecognized Framebuffer texture format");
			return GL_NONE;
		}
	}

	static GLenum TexFormat2OpenGLInternalFormat(FramebufferTextureFormat fmt)
	{
		switch (fmt)
		{
		case FramebufferTextureFormat::RGBA8:
			return GL_RGBA8;
		case FramebufferTextureFormat::RED_INTEGER:
			return GL_R32I;
		case FramebufferTextureFormat::DEPTH24_STENCIL8:
			return GL_DEPTH24_STENCIL8;
		default:
			LIGHT_CORE_ERROR("Unrecognized Framebuffer texture internal format");
			return GL_NONE;
		}
	}

	static GLenum TexFormat2OpenGLType(FramebufferTextureFormat fmt)
	{
		switch(fmt)
		{
		case FramebufferTextureFormat::RGBA8:
			return GL_UNSIGNED_BYTE;
		case FramebufferTextureFormat::RED_INTEGER:
			return GL_INT;
		case FramebufferTextureFormat::DEPTH24_STENCIL8:
			return GL_UNSIGNED_INT_24_8;
		default:
			LIGHT_CORE_ERROR("Unrecognized Framebuffer texture internal format");
			return GL_NONE;	
		}
	}

	static GLenum TexWrap2OpenGLType(TextureWrap fmt)
	{
		switch (fmt)
		{
		case TextureWrap::REPEAT:
			return GL_REPEAT;
		case TextureWrap::MIRRORED_REPEAT:
			return GL_MIRRORED_REPEAT;
		case TextureWrap::CLAMP_TO_BORDER:
			return GL_CLAMP_TO_BORDER;
		case TextureWrap::CLAMP_TO_EDGE:
			return GL_CLAMP_TO_EDGE;
		default:
			LIGHT_CORE_ERROR("Unrecognized texture wrap format");
			return GL_NONE;
		}
	}

	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpec& spec)
		: m_spec(spec), m_depthAttachmentSpec(FramebufferTextureFormat::None, TextureWrap::None)
	{
		for(auto attachmentSpec : spec.attachments)
		{
			if(attachmentSpec.textureFormat >= FramebufferTextureFormat::DepthTypes)
				m_depthAttachmentSpec = attachmentSpec;
			else
				m_colorAttachmentSpecs.emplace_back(attachmentSpec);
		}

		invalidate();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer() 
	{
		glDeleteFramebuffers(1, &m_rendererId);
		glDeleteTextures((GLsizei)m_colorAttachmentIds.size(), m_colorAttachmentIds.data());
		glDeleteTextures(1, &m_depthAttachmentId);
	}

	void OpenGLFramebuffer::resize(uint32_t width, uint32_t height) 
	{

		if (width == 0 || height == 0)
		{
			LIGHT_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
			return;
		}

        m_spec.width = width;
        m_spec.height = height;

		invalidate();
	}

	void OpenGLFramebuffer::invalidate() 
	{
		if(m_rendererId != 0)
		{
			glDeleteFramebuffers(1, &m_rendererId);
			glDeleteTextures((GLsizei)m_colorAttachmentIds.size(), m_colorAttachmentIds.data());
			if(m_depthAttachmentId != 0)
				glDeleteRenderbuffers(1, &m_depthAttachmentId);

			m_colorAttachmentIds.clear();
			m_depthAttachmentId = 0;
		}

		glGenFramebuffers(1, &m_rendererId);
		glBindFramebuffer(GL_FRAMEBUFFER, m_rendererId);


		if(m_colorAttachmentSpecs.size() > 0)
		{
			GLenum textureTarget = m_spec.samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
			m_colorAttachmentIds.resize(m_colorAttachmentSpecs.size());
			glGenTextures((GLsizei)m_colorAttachmentSpecs.size(), m_colorAttachmentIds.data());

			// Attach all color buffers
			for(int i = 0; i < (int)m_colorAttachmentSpecs.size(); i++)
			{
				glBindTexture(textureTarget, m_colorAttachmentIds[i]);

				if(m_spec.samples > 1)
				{
					glTexImage2DMultisample(
						textureTarget,
						m_spec.samples,
						TexFormat2OpenGLInternalFormat(m_colorAttachmentSpecs[i].textureFormat),
						m_spec.width,
						m_spec.height,
						GL_FALSE
					);
				}
				else
				{
					glTexImage2D(
						textureTarget,
						0,
						TexFormat2OpenGLInternalFormat(m_colorAttachmentSpecs[i].textureFormat),
						m_spec.width,
						m_spec.height,
						0,
						TexFormat2OpenGLFormat(m_colorAttachmentSpecs[i].textureFormat),
						TexFormat2OpenGLType(m_colorAttachmentSpecs[i].textureFormat),
						nullptr
					);

					if(m_colorAttachmentSpecs[i].textureFormat == FramebufferTextureFormat::RED_INTEGER)
					{
						glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					}
					else
					{
						glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					}
					glTexParameterfv(textureTarget, GL_TEXTURE_BORDER_COLOR, &(glm::vec4(0,0,0,0)[0]));

					glTexParameteri(textureTarget, GL_TEXTURE_WRAP_R,
						TexWrap2OpenGLType(m_colorAttachmentSpecs[i].wrapFormat));
					glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, 
						TexWrap2OpenGLType(m_colorAttachmentSpecs[i].wrapFormat));
					glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, 
						TexWrap2OpenGLType(m_colorAttachmentSpecs[i].wrapFormat));
				}

				glFramebufferTexture2D(GL_FRAMEBUFFER,
					GL_COLOR_ATTACHMENT0 + i,
					textureTarget,
					m_colorAttachmentIds[i],
					0
				);
			}
		}

		// Attach depth buffer
		if(m_depthAttachmentSpec.textureFormat != FramebufferTextureFormat::None)
		{
			glGenRenderbuffers(1, &m_depthAttachmentId);
			glBindRenderbuffer(GL_RENDERBUFFER, m_depthAttachmentId);
			if(m_spec.samples > 1)
			{
				glRenderbufferStorageMultisample(
					GL_RENDERBUFFER,
					m_spec.samples,
					TexFormat2OpenGLFormat(m_depthAttachmentSpec.textureFormat),
					m_spec.width,
					m_spec.height
				);
			}
			else
			{
				glRenderbufferStorage(
					GL_RENDERBUFFER,
					TexFormat2OpenGLInternalFormat(m_depthAttachmentSpec.textureFormat),
					m_spec.width,
					m_spec.height
				);
			}

			glFramebufferRenderbuffer(GL_FRAMEBUFFER,
				GL_DEPTH_STENCIL_ATTACHMENT,
				GL_RENDERBUFFER,
				m_depthAttachmentId
			);
		}

		if(m_colorAttachmentSpecs.size() > 1)
		{
			LIGHT_CORE_ASSERT(m_colorAttachmentSpecs.size() <= 4, "Only 4 color attachments supported");

			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers((GLsizei)m_colorAttachmentSpecs.size(), buffers);
		}
		else if(m_colorAttachmentSpecs.empty())
		{
			glDrawBuffer(GL_NONE);
		}

		
		LIGHT_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	int OpenGLFramebuffer::readPixelInt(uint32_t attachmentIndex, uint32_t x, uint32_t y) 
	{
		LIGHT_CORE_ASSERT(attachmentIndex < m_colorAttachmentIds.size(), "Index exceeds number of color attachments");

		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

		int pixelData;
		LIGHT_CORE_ASSERT(
			m_colorAttachmentSpecs[attachmentIndex].textureFormat == FramebufferTextureFormat::RED_INTEGER,
			"Can call readPixelInt() only on RED_INTEGER format"
		);
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

		return pixelData;
	}
	
	glm::vec4 OpenGLFramebuffer::readPixelVec4(uint32_t attachmentIndex, uint32_t x, uint32_t y) 
	{
		LIGHT_CORE_ASSERT(attachmentIndex < m_colorAttachmentIds.size(), "Index exceeds number of color attachments");

		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

		glm::vec4 pixelData;
		LIGHT_CORE_ASSERT(
			m_colorAttachmentSpecs[attachmentIndex].textureFormat == FramebufferTextureFormat::RGBA8,
			"Can call readPixelVec4() only on RGBA8 format"
		);
		glReadPixels(x, y, m_spec.width, m_spec.height, GL_RGBA, GL_FLOAT, &pixelData);
		return pixelData;
	}
	
	void OpenGLFramebuffer::clearAttachment(uint32_t attachmentIndex, int clearValue) 
	{
		LIGHT_CORE_ASSERT(attachmentIndex < m_colorAttachmentIds.size(), "Index exceeds number of color attachments");
		LIGHT_CORE_ASSERT(m_colorAttachmentSpecs[attachmentIndex].textureFormat == FramebufferTextureFormat::RED_INTEGER, "Can call clearAttachment(,int) only on RED_INTEGER format");
		
		// glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
		GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers((GLsizei)m_colorAttachmentSpecs.size(), buffers);
		GLint clearColor[] = { clearValue, clearValue, clearValue, clearValue };
		glClearBufferiv(GL_COLOR, attachmentIndex, clearColor);

		// glClearColor((GLfloat)clearValue, (GLfloat)clearValue, (GLfloat)clearValue, (GLfloat)clearValue);
		// glClear(GL_COLOR_BUFFER_BIT);
	}
	
	void OpenGLFramebuffer::clearAttachment(uint32_t attachmentIndex, glm::vec4 clearValue) 
	{
		LIGHT_CORE_ASSERT(attachmentIndex < m_colorAttachmentIds.size(), "Index exceeds number of color attachments");
		
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
		glClearColor(clearValue.r, clearValue.g, clearValue.b, clearValue.a);
		glClear(GL_COLOR_BUFFER_BIT);

		GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers((GLsizei)m_colorAttachmentSpecs.size(), buffers);
	}

	void OpenGLFramebuffer::clearDepthAttachment() 
	{
		glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0f, 0);
	}

	void OpenGLFramebuffer::bind() 
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_rendererId);
		glViewport(0, 0, m_spec.width, m_spec.height);
	}
	
	void OpenGLFramebuffer::unbind() 
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::bindAttachmentTexture(uint32_t attachmentIndex, uint32_t slot) 
	{
		LIGHT_CORE_ASSERT(attachmentIndex < m_colorAttachmentIds.size(), "Index exceeds number of color attachments");

		glActiveTexture(GL_TEXTURE0 + slot);

		if(m_spec.samples > 1)
		{
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_colorAttachmentIds[attachmentIndex]);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_colorAttachmentIds[attachmentIndex]);
		}

	}
	unsigned int OpenGLFramebuffer::attachDepthTexture(unsigned int t)
	{	
	unsigned int texture;
	glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_DEPTH_COMPONENT32F,
        1280,
        1280,
        7,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

    //glBindFramebuffer(GL_FRAMEBUFFER, m_rendererId);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
	t = texture;
	return texture;
	}
	void OpenGLFramebuffer::bindDepthTextureArray(unsigned int texture,uint32_t slot)
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
		
	}


	void OpenGLFramebuffer::renderQuad()
	{
    	if (quadVAO == 0)
    	{
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        	};
        // setup plane VAO
        	glGenVertexArrays(1, &quadVAO);
        	glGenBuffers(1, &quadVBO);
        	glBindVertexArray(quadVAO);
        	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        	glEnableVertexAttribArray(0);
        	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        	glEnableVertexAttribArray(1);
        	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    	}
    	glBindVertexArray(quadVAO);
    	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    	glBindVertexArray(0);
		
	}


	

}