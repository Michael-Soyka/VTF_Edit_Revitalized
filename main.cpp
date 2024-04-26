#include "dialogs/VTFEdit.h"
#include "src/MainWindow.h"
#include "src/Options.h"

#include <QApplication>
#include <QCommonStyle>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QStyleFactory>

using namespace ui;

int main( int argc, char **argv )
{
	QApplication app( argc, argv );

	QCommonStyle *style = (QCommonStyle *)QStyleFactory::create( "fusion" );
	QApplication::setStyle( style );

	QPalette palette;
	palette.setColor( QPalette::Window, QColor( 49, 54, 59 ) );
	palette.setColor( QPalette::WindowText, Qt::white );
	palette.setColor( QPalette::Base, QColor( 27, 30, 32 ) );
	palette.setColor( QPalette::AlternateBase, QColor( 49, 54, 59 ) );
	palette.setColor( QPalette::ToolTipBase, Qt::black );
	palette.setColor( QPalette::ToolTipText, Qt::white );
	palette.setColor( QPalette::Text, Qt::white );
	palette.setColor( QPalette::Button, QColor( 49, 54, 59 ) );
	palette.setColor( QPalette::ButtonText, Qt::white );
	palette.setColor( QPalette::BrightText, Qt::red );
	palette.setColor( QPalette::Link, QColor( 42, 130, 218 ) );
	palette.setColor( QPalette::Highlight, QColor( 42, 130, 218 ) );
	palette.setColor( QPalette::HighlightedText, Qt::black );
	palette.setColor( QPalette::Active, QPalette::Button, QColor( 49, 54, 59 ) );
	palette.setColor( QPalette::Disabled, QPalette::ButtonText, Qt::darkGray );
	palette.setColor( QPalette::Disabled, QPalette::WindowText, Qt::darkGray );
	palette.setColor( QPalette::Disabled, QPalette::Text, Qt::darkGray );
	palette.setColor( QPalette::Disabled, QPalette::Light, QColor( 49, 54, 59 ) );

	QApplication::setPalette( palette );

	std::unique_ptr<QSettings> options;
	if ( Options::isStandalone() )
	{
		auto configPath = QApplication::applicationDirPath() + "/config.ini";
		options = std::make_unique<QSettings>( configPath, QSettings::Format::IniFormat );
	}
	else
	{
		options = std::make_unique<QSettings>();
	}

	if ( options->value( STR_OPEN_RECENT ).value<QStringList>().isEmpty() )
		options->setValue( STR_OPEN_RECENT, QStringList() << QDir::currentPath() );

	Options::setupOptions( *options );

	auto pVTFEdit = new ui::CMainWindow();
	pVTFEdit->processCLIArguments( argc, argv );
	pVTFEdit->setAttribute( Qt::WA_DeleteOnClose );

	if ( !Options::get<bool>( OPT_START_MAXIMIZED ) )
	{
		pVTFEdit->show();
	}
	else
	{
		pVTFEdit->showMaximized();
	}

	QApplication::setWindowIcon( QIcon( "vtf_edit_revitalised2.png" ).pixmap( 1080, 1080 ) );
	return QApplication::exec();
}
