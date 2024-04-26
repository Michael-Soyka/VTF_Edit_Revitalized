
#include "ImageViewWidget.h"

#include <QColorSpace>
#include <QPainter>
#include <QStyleOption>
#include <QWheelEvent>
#include <iostream>

#define remap( value, low1, high1, low2, high2 ) ( low2 + ( value - low1 ) * ( high2 - low2 ) / ( high1 - low1 ) )

using namespace VTFLib;

void ImageViewWidget::Animate()
{
	if ( !file_ )
		return;

	if ( frame_ < file_->GetFrameCount() )
		frame_++;
	else
		frame_ = 0;
	this->update();
	emit animated( frame_ );
}

ImageViewWidget::ImageViewWidget( QWidget *pParent ) :
	QOpenGLWidget( pParent ), QOpenGLFunctions_4_5_Core()

{
	setFocusPolicy( Qt::StrongFocus );
}

void ImageViewWidget::startAnimation( int fps )
{
	animationTimer_ = startTimer( 1000 / fps );
}

void ImageViewWidget::stopAnimating()
{
	if ( animationTimer_ > 0 )
	{
		killTimer( animationTimer_ );
		animationTimer_ = -1;
	}
}

void ImageViewWidget::set_vtf( VTFLib::CVTFFile *file )
{
	file_ = file;
	// Force refresh of data
	currentFrame_ = -1;
	currentFace_ = -1;
	currentMip_ = -1;

	zoom_ = 1.f;
	pos_ = { 0, 0 };

	// No file, sad.
	if ( !file )
		return;

	update_size();
}

void ImageViewWidget::initializeGL()
{
	initializeOpenGLFunctions();

	shaderProgram = new QOpenGLShaderProgram( this );

	shaderProgram->addShaderFromSourceFile( QOpenGLShader::Vertex, ":/vertex.glsl" );
	shaderProgram->addShaderFromSourceFile( QOpenGLShader::Fragment, ":/fragment.glsl" );

	shaderProgram->link();
	shaderProgram->bind();

	this->vertices.create();
	this->vertices.bind();
	this->vertices.setUsagePattern( QOpenGLBuffer::StaticDraw );
	this->vertices.allocate( texCoords, sizeof( texCoords ) );
	this->vertices.release();

	this->indexes.create();
	this->indexes.bind();
	this->indexes.allocate( texIndeces, sizeof( texIndeces ) );
	this->indexes.release();

	this->resize( 4096, 4096 );
}

void ImageViewWidget::resizeGL( int w, int h )
{
	// Update projection matrix and other size related settings:
	glViewport( 0, 0, w, h );
	//	glMatrixMode( GL_PROJECTION );
	//	glOrtho( -aspect, aspect, -1, 1, -1, 1 );
	//
	//	glMatrixMode( GL_MODELVIEW );
	//	glLoadIdentity();
}

void ImageViewWidget::paintGL()
{
	// Draw the scene:
	QStyleOption opt;
	opt.initFrom( this );
	auto clearColor = opt.palette.color( QPalette::ColorRole::Window );
	this->glClearColor( clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF() );
	this->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	this->glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	shaderProgram->bind();

	float fov = 90.0f;
	float tanHalFOV = tanf( ( fov / 2.0f ) * ( 3.141592 / 180 ) );
	float d = 1 / tanHalFOV;

	float aspect = (float)this->width() / (float)this->height();

	QMatrix4x4 projectionMatrix = {
		1.0f / aspect, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f };

	//	qInfo() << aspect;
	int AspectRatioLocation = shaderProgram->uniformLocation( "ProjMat" ); // glGetUniformLocation( shaderProgram, "RGBA" );
	shaderProgram->setUniformValue( AspectRatioLocation, projectionMatrix );

	int RGBAProcessing = shaderProgram->uniformLocation( "RGBA" ); // glGetUniformLocation( shaderProgram, "RGBA" );

	shaderProgram->setUniformValue( RGBAProcessing, rgba_ );

	int scalingTransformation = shaderProgram->uniformLocation( "SCALING" );

	QMatrix4x4 scalar = { zoom_, 0.0f, 0.0f, 0.0f,
						  0.0f, zoom_, 0.0f, 0.0f,
						  0.0f, 0.0f, zoom_, 0.0f,
						  0.0f, 0.0f, 0.0f, 1.0f };

	float offs = ( 0.5f / aspect );
	QVector2D offsets = { remap( xOffset_, 0, 4096, offs * zoom_, -offs * zoom_ ), remap( yOffset_, 0, 4096, -offs * zoom_, offs * zoom_ ) };
	//	qInfo() << offsets;

	int OFFSETProcessing = shaderProgram->uniformLocation( "OFFSET" ); // glGetUniformLocation( shaderProgram, "RGBA" );

	shaderProgram->setUniformValue( OFFSETProcessing, offsets );

	shaderProgram->setUniformValue( scalingTransformation, scalar );

	indexes.bind();
	vertices.bind();

	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( GLfloat ) * 8, 0 );

	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof( float ), (void *)( 3 * sizeof( float ) ) );

	glEnableVertexAttribArray( 2 );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof( float ), (void *)( 6 * sizeof( float ) ) );

	if ( file_ )
	{
		GLuint width, height, whatever;
		CVTFFile::ComputeMipmapDimensions( file_->GetWidth(), file_->GetHeight(), 1, mip_, width, height, whatever );
		auto size = CVTFFile::ComputeImageSize( width, width, whatever, IMAGE_FORMAT_RGBA8888 );
		auto imgData = new vlByte[size];
		CVTFFile::ConvertToRGBA8888( file_->GetData( frame_, face_, 0, mip_ ), reinterpret_cast<vlByte *>( imgData ), width, height, file_->GetFormat() );

		texture.create();
		texture.setData( QImage( imgData, width, height, QImage::Format_RGBA8888 ) );

		texture.bind( 0 );

		delete[] imgData;
	}
	else
	{
		static constexpr unsigned char buff[4] = { 0, 0, 0, 0 };
		texture.create();
		texture.setData( QImage( buff, 1, 1, QImage::Format_RGBA8888 ) );
		texture.bind( 0 );
	}

	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr );
	indexes.release();
	vertices.release();
	texture.release( 0 );
	texture.destroy();
	shaderProgram->release();
}

// void ImageViewWidget::paintEvent( QPaintEvent *event )
//{
//	QPainter painter( this );
//
//	if ( !file_ )
//		return;
//
//	// Compute draw size for this mip, frame, etc
//	vlUInt imageWidth, imageHeight, imageDepth;
//	CVTFFile::ComputeMipmapDimensions(
//		file_->GetWidth(), file_->GetHeight(), file_->GetDepth(), mip_, imageWidth, imageHeight, imageDepth );
//
//	// Needs decode
//	if ( frame_ != currentFrame_ || mip_ != currentMip_ || face_ != currentFace_ || requestColorChange )
//	{
//		const bool hasAlpha = CVTFFile::GetImageFormatInfo( file_->GetFormat() ).uiAlphaBitsPerPixel > 0;
//		const VTFImageFormat format = hasAlpha ? IMAGE_FORMAT_RGBA8888 : IMAGE_FORMAT_RGB888;
//		auto size = file_->ComputeMipmapSize( file_->GetWidth(), file_->GetHeight(), 1, mip_, format );
//
//		if ( imgBuf_ )
//		{
//			free( imgBuf_ );
//		}
//		// This buffer needs to persist- QImage does not own the mem you give it
//		imgBuf_ = static_cast<vlByte *>( malloc( size ) );
//
//		bool ok = CVTFFile::Convert(
//			file_->GetData( frame_, face_, 0, mip_ ), (vlByte *)imgBuf_, imageWidth, imageHeight, file_->GetFormat(),
//			format );
//
//		if ( !ok )
//		{
//			std::cerr << "Could not convert image for display.\n";
//			return;
//		}
//
//		image_ = QImage(
//			(uchar *)imgBuf_, imageWidth, imageHeight, hasAlpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888 );
//
//		if ( requestColorChange )
//		{
//			//			for ( int y = 0; y < image_.height(); ++y )
//			//			{
//			//				QRgb *line = reinterpret_cast<QRgb *>( image_.scanLine( y ) );
//			//				for ( int x = 0; x < image_.width(); ++x )
//			//				{
//			//					QRgb &rgb = line[x];
//			//					rgb = qRgba( qRed( red_ ? rgb : 0 ), qGreen( green_ ? rgb : 0 ), qBlue( blue_ ? rgb : 0 ), qAlpha( alpha_ ? rgb : 255 ) );
//			//				}
//			//			}
//
//			for ( int i = 0; i < ( image_.width() ); i++ )
//				for ( int j = 0; j < image_.height(); j++ )
//				{
//					QColor QImageColor = QColor( image_.pixel( i, j ) );
//					QRgb r = red_ ? QImageColor.red() : 0;
//					QRgb g = green_ ? QImageColor.green() : 0;
//					QRgb b = blue_ ? QImageColor.blue() : 0;
//					QRgb a = alpha_ ? qAlpha( image_.pixel( i, j ) ) : 255;
//
//					image_.setPixelColor( i, j, QColor( r, g, b, a ) );
//				}
//		}
//
//		requestColorChange = false;
//		currentFace_ = face_;
//		currentFrame_ = frame_;
//		currentMip_ = mip_;
//	}
//
//	QPoint destpt =
//		QPoint( width() / 2, height() / 2 ) - QPoint( ( imageWidth * zoom_ ) / 2, ( imageHeight * zoom_ ) / 2 ) + pos_;
//	QRect target = QRect( destpt.x(), destpt.y(), image_.width() * zoom_, image_.height() * zoom_ );
//
//	painter.drawImage( target, image_, QRect( 0, 0, image_.width(), image_.height() ) );
// }

void ImageViewWidget::wheelEvent( QWheelEvent *event )
{
	if ( event->angleDelta().y() > 0 ) // up Wheel
	{
		if ( m_isCTRLHeld )
		{
			zoom( 0.1 );
			event->ignore();
			return;
		}
	}
	else if ( event->angleDelta().y() < 0 ) // down Wheel
	{
		if ( m_isCTRLHeld )
		{
			zoom( -0.1 );
			event->ignore();
			return;
		}
	}
	QOpenGLWidget::wheelEvent( event );
}

bool ImageViewWidget::event( QEvent *event )
{
	if ( event->type() == QEvent::KeyPress )
	{
		auto ke = static_cast<QKeyEvent *>( event );
		if ( ( ke->key() == Qt::Key_Control ) )
			m_isCTRLHeld = true;
	}

	if ( event->type() == QEvent::KeyRelease )
	{
		auto ke = static_cast<QKeyEvent *>( event );
		if ( ( ke->key() == Qt::Key_Control ) )
			m_isCTRLHeld = false;
	}

	// When we lose focus, we can no longer check for key events, so to prevent
	// weird behaviour upon defocus, we set m_isCTRLHeld to false.
	if ( event->type() == QEvent::FocusOut )
	{
		auto fe = static_cast<QFocusEvent *>( event );
		if ( fe->lostFocus() )
			m_isCTRLHeld = false;
	}

	return QOpenGLWidget::event( event );
}

void ImageViewWidget::zoom( float amount )
{
	if ( amount == 0 || !file_ )
		return; // Skip expensive repaint
	zoom_ += amount;
	if ( zoom_ < 0.01f )
		zoom_ = 0.01f;

	emit zoomChanged( zoom_ );

	update_size();
}

void ImageViewWidget::update_size()
{
	if ( !file_ )
		return;

	this->update();
}
void ImageViewWidget::timerEvent( QTimerEvent *event )
{
	Animate();
	QObject::timerEvent( event );
}
float ImageViewWidget::getZoom() const
{
	return zoom_;
}