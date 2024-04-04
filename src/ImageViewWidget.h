#pragma once
#include "../libs/VTFLib/VTFLib/VTFLib.h"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLPaintDevice>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QWidget>

enum ColorSelection
{
	ALL,
	RGB,
	RED,
	GREEN,
	BLUE,
	ALPHA
};

class ImageViewWidget : public QOpenGLWidget,
						protected QOpenGLFunctions_4_5_Core
{
	Q_OBJECT;

	static constexpr GLfloat texCoords[] = {
		// positions          // colors           // texture coords
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,	  // top right
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
		-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f	  // top left
	};

	static constexpr GLbyte texIndeces[] = {
		0, 1, 2, 2, 3, 0 };

public:
	ImageViewWidget( QWidget *pParent = nullptr );

	void timerEvent( QTimerEvent *event ) override;

	void set_vtf( VTFLib::CVTFFile *file );

	void initializeGL() override;

	void resizeGL( int w, int h ) override;

	void paintGL() override;

	void set_red( bool red )
	{
		if ( red == hasRed_ )
			return;

		rgba_ += red ? 8 : -8;
		hasRed_ = red;
		this->update();
	}

	void set_green( bool green )
	{
		if ( green == hasGreen_ )
			return;
		rgba_ += green ? 4 : -4;
		hasGreen_ = green;
		this->update();
	}

	void set_blue( bool blue )
	{
		if ( blue == hasBlue_ )
			return;
		rgba_ += blue ? 2 : -2;
		hasBlue_ = blue;
		this->update();
	}

	void set_alpha( bool alpha )
	{
		if ( alpha == hasAlpha_ )
			return;
		rgba_ += alpha ? 1 : -1;
		hasAlpha_ = alpha;
		this->update();
	}

	void set_rgba( bool r, bool g, bool b, bool a )
	{
		if ( r == hasRed_ && g == hasGreen_ && b == hasBlue_ && hasAlpha_ == a )
			return;

		if ( a != hasAlpha_ )
		{
			rgba_ += a ? 1 : -1;
			hasAlpha_ = !hasAlpha_;
		}
		{
			rgba_ += b ? 2 : -2;
			hasBlue_ = !hasBlue_;
		}
		{
			rgba_ += g ? 4 : -4;
			hasGreen_ = !hasGreen_;
		}
		{
			rgba_ += r ? 8 : -8;
			hasRed_ = !hasRed_;
		}

		this->update();
	}

	void set_frame( int f )
	{
		frame_ = f;
		this->update();
	}

	int get_frame() const
	{
		return frame_;
	}

	void set_face( int f )
	{
		if ( m_animating )
			return; // We do not allow the face to be set/messed with publicly when it's animating.
		face_ = f;
		this->update();
	}
	void set_mip( int f )
	{
		mip_ = f;
		this->update();
	}

	float getZoom() const;

	void zoom( float amount );

	void setXOffset( int offset )
	{
		xOffset_ = offset;
		this->update();
	};

	void setYOffset( int offset )
	{
		yOffset_ = offset;
		this->update();
	}

	void startAnimation( int fps );

	void stopAnimating();

private:
	void update_size();

	QOpenGLTexture texture { QOpenGLTexture::Target2D };
	QOpenGLShaderProgram *shaderProgram;
	VTFLib::CVTFFile *file_ = nullptr;

	bool m_animating = false;

	QOpenGLBuffer vertices { QOpenGLBuffer::Type::VertexBuffer };

	QOpenGLBuffer indexes { QOpenGLBuffer::Type::IndexBuffer };

	float zoom_ = 1.0f;
	QPoint pos_;

	int frame_ = 0;
	int face_ = 0;
	int mip_ = 0;
	int rgba_ = 16;
	float xOffset_ = 0;
	float yOffset_ = 0;
	bool hasRed_ = true;
	bool hasGreen_ = true;
	bool hasBlue_ = true;
	bool hasAlpha_ = true;

	int animationTimer_ = -1;

	int currentFrame_ = 0;
	int currentFace_ = 0;
	int currentMip_ = 0;
	bool requestColorChange = false;
	void Animate();
signals:
	void animated( int frame );
};