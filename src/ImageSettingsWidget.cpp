#include "ImageSettingsWidget.h"

#include "flagsandformats.hpp"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

ImageSettingsWidget::ImageSettingsWidget( ImageViewWidget *viewer, QWidget *parent ) :
	QWidget( parent )
{
	setup_ui( viewer );
}

void ImageSettingsWidget::setup_ui( ImageViewWidget *viewer )
{
	auto *layout = new QVBoxLayout( this );
	auto *ImageGeneralSettingsGroupBox = new QGroupBox( tr( "General" ), this );
	auto *ImageFlagsGroupBox = new QGroupBox( tr( "Flags" ), this );

	auto *GeneralSettingsLayout = new QGridLayout( ImageGeneralSettingsGroupBox );
	auto *FlagsLayout = new QGridLayout( ImageFlagsGroupBox );

	// General Settings
	int row = 0;
	frame_ = new QSpinBox( this );
	connect(
		frame_, &QSpinBox::textChanged,
		[viewer, this]( const QString & )
		{
			viewer->set_frame( frame_->value() );
		} );
	GeneralSettingsLayout->addWidget( frame_, row, 1 );
	GeneralSettingsLayout->addWidget( new QLabel( tr( "Frame:" ) ), row, 0 );

	++row;
	mip_ = new QSpinBox( this );
	connect(
		mip_, &QSpinBox::textChanged,
		[viewer, this]( const QString & )
		{
			viewer->set_mip( mip_->value() );
		} );
	GeneralSettingsLayout->addWidget( mip_, row, 1 );
	GeneralSettingsLayout->addWidget( new QLabel( tr( "Mip:" ) ), row, 0 );

	++row;
	face_ = new QSpinBox( this );
	connect(
		face_, &QSpinBox::textChanged,
		[viewer, this]( const QString & )
		{
			viewer->set_face( face_->value() );
		} );
	GeneralSettingsLayout->addWidget( face_, row, 1 );
	GeneralSettingsLayout->addWidget( new QLabel( tr( "Face:" ) ), row, 0 );

	++row;
	startFrame_ = new QSpinBox( this );
	connect(
		startFrame_, &QSpinBox::textChanged,
		[viewer, this]( const QString & )
		{
			if ( !file_ )
				return;
			file_->SetStartFrame( startFrame_->value() );
			if ( !settingFile_ )
				emit fileModified();
		} );
	GeneralSettingsLayout->addWidget( startFrame_, row, 1 );
	GeneralSettingsLayout->addWidget( new QLabel( tr( "Start Frame:" ) ), row, 0 );
	++row;
	QSpinBox *frameBox = new QSpinBox( this );
	frameBox->setMinimum( 1 );
	frameBox->setValue( 24 );
	frameBox->setMaximum( 144 ); // I don't think you can even run Source even supports 144
	GeneralSettingsLayout->addWidget( frameBox, row, 1 );
	GeneralSettingsLayout->addWidget( new QLabel( tr( "Animation FPS:" ) ), row, 0 );
	++row;

	animateButton = new QPushButton( tr( "Animate" ), this );

	connect( animateButton, &QPushButton::pressed, this, [&, viewer, frameBox]
			 {
				 if ( !file_ )
					 return;

				 if ( file_->GetFrameCount() <= 1 )
					 return; // Do not animate when we do not have frames to animate, lol.

				 if ( animateButton->text() == tr( "Animate" ) )
				 {
					 viewer->startAnimation( frameBox->value() );
					 animateButton->setText( tr( "Stop" ) );
				 }
				 else
				 {
					 viewer->stopAnimating();
					 animateButton->setText( tr( "Animate" ) );
				 }
			 } );

	GeneralSettingsLayout->addWidget( animateButton, row, 1 );

	// Flags list box
	++row;
	auto *flagsScroll = new QScrollArea( this );
	auto *flagsContent = new QWidget();
	auto *flagsLayout = new QGridLayout( flagsContent );

	for ( auto &flag : TEXTURE_FLAGS )
	{
		auto *check = new QCheckBox( flag.name, this );
		check->setCheckable( true );
		connect(
			check, &QCheckBox::stateChanged,
			[this, flag]( int newState )
			{
				if ( !file_ )
					return;
				file_->SetFlag( ( VTFImageFlag )flag.flag, newState );
				if ( !settingFile_ )
					emit fileModified();
			} );
		flagChecks_.insert( { flag.flag, check } );
		flagsLayout->addWidget( check );
	}

	flagsScroll->setWidget( flagsContent );
	FlagsLayout->addWidget( flagsScroll, row, 0, 1, 2 );

	layout->addWidget( ImageGeneralSettingsGroupBox );
	layout->addWidget( ImageFlagsGroupBox );
}

void ImageSettingsWidget::set_vtf( VTFLib::CVTFFile *file )
{
	// Hack to ensure we don't emit fileModified when setting defaults
	settingFile_ = true;

	// Set some sensible defaults in the event no file is loaded
	if ( !file )
	{
		startFrame_->setValue( 0 );
		startFrame_->setRange( 0, 0 );
		mip_->setValue( 0 );
		mip_->setRange( 0, 0 );
		face_->setValue( 0 );
		face_->setRange( 0, 0 );
		frame_->setValue( 0 );
		frame_->setRange( 0, 0 );
		for ( auto &check : flagChecks_ )
		{
			check.second->setChecked( false );
			check.second->setCheckable( false );
		}
		settingFile_ = false;
		file_ = nullptr;
		animateButton->setText( tr( "Animate" ) );
		return;
	}

	file_ = file;
	startFrame_->setValue( file->GetStartFrame() );
	mip_->setValue( 0 );
	frame_->setValue( file->GetStartFrame() );
	face_->setValue( 0 );

	// Configure ranges
	mip_->setRange( 0, file->GetMipmapCount() );
	frame_->setRange( 0, file->GetFrameCount() - 1 );
	face_->setRange( 1, file->GetFaceCount() );
	startFrame_->setRange( 1, file->GetFrameCount() );

	animateButton->setText( tr( "Animate" ) ); // Tab switching stops animation, this reflects that.

	// Set the flags
	uint32_t flags = file->GetFlags();
	for ( auto &f : TEXTURE_FLAGS )
	{
		auto check = flagChecks_.find( f.flag )->second;
		check->setChecked( !!( flags & f.flag ) );
		check->setCheckable( true );
	}

	settingFile_ = false;
}
