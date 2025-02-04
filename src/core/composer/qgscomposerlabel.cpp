/***************************************************************************
                         qgscomposerlabel.cpp
                             -------------------
    begin                : January 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposerlabel.h"
#include "qgscomposition.h"
#include "qgsexpression.h"
#include "qgsnetworkaccessmanager.h"

#include <QCoreApplication>
#include <QDate>
#include <QDomElement>
#include <QPainter>
#include <QSettings>
#include <QTimer>
#include <QWebFrame>
#include <QWebPage>
#include <QEventLoop>

QgsComposerLabel::QgsComposerLabel( QgsComposition *composition ):
    QgsComposerItem( composition ), mHtmlState( 0 ), mHtmlUnitsToMM( 1.0 ),
    mHtmlLoaded( false ), mMargin( 1.0 ), mFontColor( QColor( 0, 0, 0 ) ),
    mHAlignment( Qt::AlignLeft ), mVAlignment( Qt::AlignTop ),
    mExpressionFeature( 0 ), mExpressionLayer( 0 )
{
  mHtmlUnitsToMM = htmlUnitsToMM();

  //get default composer font from settings
  QSettings settings;
  QString defaultFontString = settings.value( "/Composer/defaultFont" ).toString();
  if ( !defaultFontString.isEmpty() )
  {
    mFont.setFamily( defaultFontString );
  }

  //default to a 10 point font size
  mFont.setPointSizeF( 10 );

  //default to no background
  setBackgroundEnabled( false );

  if ( mComposition && mComposition->atlasMode() == QgsComposition::PreviewAtlas )
  {
    //a label added while atlas preview is enabled needs to have the expression context set,
    //otherwise fields in the label aren't correctly evaluated until atlas preview feature changes (#9457)
    setExpressionContext( mComposition->atlasComposition().currentFeature(), mComposition->atlasComposition().coverageLayer() );
  }

  //connect to atlas feature changes
  //to update the expression context
  connect( &mComposition->atlasComposition(), SIGNAL( featureChanged( QgsFeature* ) ), this, SLOT( refreshExpressionContext() ) );

}

QgsComposerLabel::~QgsComposerLabel()
{
}

void QgsComposerLabel::paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget )
{
  Q_UNUSED( itemStyle );
  Q_UNUSED( pWidget );
  if ( !painter )
  {
    return;
  }

  drawBackground( painter );
  painter->save();

  //antialiasing on
  painter->setRenderHint( QPainter::Antialiasing, true );

  double penWidth = hasFrame() ? pen().widthF() : 0;
  QRectF painterRect( penWidth + mMargin, penWidth + mMargin, rect().width() - 2 * penWidth - 2 * mMargin, rect().height() - 2 * penWidth - 2 * mMargin );

  QString textToDraw = displayText();

  if ( mHtmlState )
  {
    painter->scale( 1.0 / mHtmlUnitsToMM / 10.0, 1.0 / mHtmlUnitsToMM / 10.0 );

    QWebPage *webPage = new QWebPage();
    webPage->setNetworkAccessManager( QgsNetworkAccessManager::instance() );

    //Setup event loop and timeout for rendering html
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot( true );

    //This makes the background transparent. Found on http://blog.qt.digia.com/blog/2009/06/30/transparent-qwebview-or-qwebpage/
    QPalette palette = webPage->palette();
    palette.setBrush( QPalette::Base, Qt::transparent );
    webPage->setPalette( palette );
    //webPage->setAttribute(Qt::WA_OpaquePaintEvent, false); //this does not compile, why ?

    webPage->setViewportSize( QSize( painterRect.width() * mHtmlUnitsToMM * 10.0, painterRect.height() * mHtmlUnitsToMM * 10.0 ) );
    webPage->mainFrame()->setZoomFactor( 10.0 );
    webPage->mainFrame()->setScrollBarPolicy( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
    webPage->mainFrame()->setScrollBarPolicy( Qt::Vertical, Qt::ScrollBarAlwaysOff );

    // QGIS segfaults when rendering web page while in composer if html
    // contains images. So if we are not printing the composition, then
    // disable image loading
    if ( mComposition->plotStyle() != QgsComposition::Print &&
         mComposition->plotStyle() != QgsComposition::Postscript )
    {
      webPage->settings()->setAttribute( QWebSettings::AutoLoadImages, false );
    }

    //Connect timeout and webpage loadFinished signals to loop
    connect( &timeoutTimer, SIGNAL( timeout() ), &loop, SLOT( quit() ) );
    connect( webPage, SIGNAL( loadFinished( bool ) ), &loop, SLOT( quit() ) );

    //mHtmlLoaded tracks whether the QWebPage has completed loading
    //its html contents, set it initially to false. The loadingHtmlFinished slot will
    //set this to true after html is loaded.
    mHtmlLoaded = false;
    connect( webPage, SIGNAL( loadFinished( bool ) ), SLOT( loadingHtmlFinished( bool ) ) );

    webPage->mainFrame()->setHtml( textToDraw );

    //For very basic html labels with no external assets, the html load will already be
    //complete before we even get a chance to start the QEventLoop. Make sure we check
    //this before starting the loop
    if ( !mHtmlLoaded )
    {
      // Start a 20 second timeout in case html loading will never complete
      timeoutTimer.start( 20000 );
      // Pause until html is loaded
      loop.exec();
    }

    webPage->mainFrame()->render( painter );//DELETE WEBPAGE ?
  }
  else
  {
    painter->setPen( QPen( QColor( mFontColor ) ) );
    painter->setFont( mFont );

    QFontMetricsF fontSize( mFont );

    //debug
    //painter->setPen( QColor( Qt::red ) );
    //painter->drawRect( painterRect );
    drawText( painter, painterRect, textToDraw, mFont, mHAlignment, mVAlignment, Qt::TextWordWrap );
  }

  painter->restore();

  drawFrame( painter );
  if ( isSelected() )
  {
    drawSelectionBoxes( painter );
  }
}

/*Track when QWebPage has finished loading its html contents*/
void QgsComposerLabel::loadingHtmlFinished( bool result )
{
  Q_UNUSED( result );
  mHtmlLoaded = true;
}

double QgsComposerLabel::htmlUnitsToMM()
{
  if ( !mComposition )
  {
    return 1.0;
  }

  //TODO : fix this more precisely so that the label's default text size is the same with or without "display as html"
  return ( mComposition->printResolution() / 72.0 ); //webkit seems to assume a standard dpi of 72
}

void QgsComposerLabel::setText( const QString& text )
{
  mText = text;
  emit itemChanged();
}

void QgsComposerLabel::setExpressionContext( QgsFeature* feature, QgsVectorLayer* layer, QMap<QString, QVariant> substitutions )
{
  mExpressionFeature = feature;
  mExpressionLayer = layer;
  mSubstitutions = substitutions;
  // Force label to redraw -- fixes label printing for labels with blend modes when used with atlas
  update();
}

void QgsComposerLabel::refreshExpressionContext()
{
  QgsVectorLayer * vl = 0;
  QgsFeature* feature = 0;

  if ( mComposition->atlasComposition().enabled() )
  {
    vl = mComposition->atlasComposition().coverageLayer();
  }
  if ( mComposition->atlasMode() != QgsComposition::AtlasOff )
  {
    feature = mComposition->atlasComposition().currentFeature();
  }

  setExpressionContext( feature, vl );
}

QString QgsComposerLabel::displayText() const
{
  QString displayText = mText;
  replaceDateText( displayText );
  QMap<QString, QVariant> subs = mSubstitutions;
  subs[ "$page" ] = QVariant(( int )mComposition->itemPageNumber( this ) + 1 );
  return QgsExpression::replaceExpressionText( displayText, mExpressionFeature, mExpressionLayer, &subs );
}

void QgsComposerLabel::replaceDateText( QString& text ) const
{
  QString constant = "$CURRENT_DATE";
  int currentDatePos = text.indexOf( constant );
  if ( currentDatePos != -1 )
  {
    //check if there is a bracket just after $CURRENT_DATE
    QString formatText;
    int openingBracketPos = text.indexOf( "(", currentDatePos );
    int closingBracketPos = text.indexOf( ")", openingBracketPos + 1 );
    if ( openingBracketPos != -1 &&
         closingBracketPos != -1 &&
         ( closingBracketPos - openingBracketPos ) > 1 &&
         openingBracketPos == currentDatePos + constant.size() )
    {
      formatText = text.mid( openingBracketPos + 1, closingBracketPos - openingBracketPos - 1 );
      text.replace( currentDatePos, closingBracketPos - currentDatePos + 1, QDate::currentDate().toString( formatText ) );
    }
    else //no bracket
    {
      text.replace( "$CURRENT_DATE", QDate::currentDate().toString() );
    }
  }
}

void QgsComposerLabel::setFont( const QFont& f )
{
  mFont = f;
}

void QgsComposerLabel::adjustSizeToText()
{
  double textWidth = textWidthMillimeters( mFont, displayText() );
  double fontHeight = fontHeightMillimeters( mFont );

  double penWidth = hasFrame() ? pen().widthF() : 0;

  double width = textWidth + 2 * mMargin + 2 * penWidth + 1;
  double height = fontHeight + 2 * mMargin + 2 * penWidth;

  //keep alignment point constant
  double xShift = 0;
  double yShift = 0;
  itemShiftAdjustSize( width, height, xShift, yShift );

  //update rect for data defined size and position
  QRectF evaluatedRect = evalItemRect( QRectF( pos().x() + xShift, pos().y() + yShift, width, height ) );
  setSceneRect( evaluatedRect );
}

QFont QgsComposerLabel::font() const
{
  return mFont;
}

bool QgsComposerLabel::writeXML( QDomElement& elem, QDomDocument & doc ) const
{
  QString alignment;

  if ( elem.isNull() )
  {
    return false;
  }

  QDomElement composerLabelElem = doc.createElement( "ComposerLabel" );

  composerLabelElem.setAttribute( "htmlState", mHtmlState );

  composerLabelElem.setAttribute( "labelText", mText );
  composerLabelElem.setAttribute( "margin", QString::number( mMargin ) );

  composerLabelElem.setAttribute( "halign", mHAlignment );
  composerLabelElem.setAttribute( "valign", mVAlignment );

  //font
  QDomElement labelFontElem = doc.createElement( "LabelFont" );
  labelFontElem.setAttribute( "description", mFont.toString() );
  composerLabelElem.appendChild( labelFontElem );

  //font color
  QDomElement fontColorElem = doc.createElement( "FontColor" );
  fontColorElem.setAttribute( "red", mFontColor.red() );
  fontColorElem.setAttribute( "green", mFontColor.green() );
  fontColorElem.setAttribute( "blue", mFontColor.blue() );
  composerLabelElem.appendChild( fontColorElem );

  elem.appendChild( composerLabelElem );
  return _writeXML( composerLabelElem, doc );
}

bool QgsComposerLabel::readXML( const QDomElement& itemElem, const QDomDocument& doc )
{
  QString alignment;

  if ( itemElem.isNull() )
  {
    return false;
  }

  //restore label specific properties

  //text
  mText = itemElem.attribute( "labelText" );

  //html state
  mHtmlState = itemElem.attribute( "htmlState" ).toInt();

  //margin
  mMargin = itemElem.attribute( "margin" ).toDouble();

  //Horizontal alignment
  mHAlignment = ( Qt::AlignmentFlag )( itemElem.attribute( "halign" ).toInt() );

  //Vertical alignment
  mVAlignment = ( Qt::AlignmentFlag )( itemElem.attribute( "valign" ).toInt() );

  //font
  QDomNodeList labelFontList = itemElem.elementsByTagName( "LabelFont" );
  if ( labelFontList.size() > 0 )
  {
    QDomElement labelFontElem = labelFontList.at( 0 ).toElement();
    mFont.fromString( labelFontElem.attribute( "description" ) );
  }

  //font color
  QDomNodeList fontColorList = itemElem.elementsByTagName( "FontColor" );
  if ( fontColorList.size() > 0 )
  {
    QDomElement fontColorElem = fontColorList.at( 0 ).toElement();
    int red = fontColorElem.attribute( "red", "0" ).toInt();
    int green = fontColorElem.attribute( "green", "0" ).toInt();
    int blue = fontColorElem.attribute( "blue", "0" ).toInt();
    mFontColor = QColor( red, green, blue );
  }
  else
  {
    mFontColor = QColor( 0, 0, 0 );
  }

  //restore general composer item properties
  QDomNodeList composerItemList = itemElem.elementsByTagName( "ComposerItem" );
  if ( composerItemList.size() > 0 )
  {
    QDomElement composerItemElem = composerItemList.at( 0 ).toElement();

    //rotation
    if ( composerItemElem.attribute( "rotation", "0" ).toDouble() != 0 )
    {
      //check for old (pre 2.1) rotation attribute
      setItemRotation( composerItemElem.attribute( "rotation", "0" ).toDouble() );
    }

    _readXML( composerItemElem, doc );
  }
  emit itemChanged();
  return true;
}

void QgsComposerLabel::itemShiftAdjustSize( double newWidth, double newHeight, double& xShift, double& yShift ) const
{
  //keep alignment point constant
  double currentWidth = rect().width();
  double currentHeight = rect().height();
  xShift = 0;
  yShift = 0;

  if ( mItemRotation >= 0 && mItemRotation < 90 )
  {
    if ( mHAlignment == Qt::AlignHCenter )
    {
      xShift = - ( newWidth - currentWidth ) / 2.0;
    }
    else if ( mHAlignment == Qt::AlignRight )
    {
      xShift = - ( newWidth - currentWidth );
    }
    if ( mVAlignment == Qt::AlignVCenter )
    {
      yShift = -( newHeight - currentHeight ) / 2.0;
    }
    else if ( mVAlignment == Qt::AlignBottom )
    {
      yShift = - ( newHeight - currentHeight );
    }
  }
  if ( mItemRotation >= 90 && mItemRotation < 180 )
  {
    if ( mHAlignment == Qt::AlignHCenter )
    {
      yShift = -( newHeight  - currentHeight ) / 2.0;
    }
    else if ( mHAlignment == Qt::AlignRight )
    {
      yShift = -( newHeight  - currentHeight );
    }
    if ( mVAlignment == Qt::AlignTop )
    {
      xShift = -( newWidth - currentWidth );
    }
    else if ( mVAlignment == Qt::AlignVCenter )
    {
      xShift = -( newWidth - currentWidth / 2.0 );
    }
  }
  else if ( mItemRotation >= 180 && mItemRotation < 270 )
  {
    if ( mHAlignment == Qt::AlignHCenter )
    {
      xShift = -( newWidth - currentWidth ) / 2.0;
    }
    else if ( mHAlignment == Qt::AlignLeft )
    {
      xShift = -( newWidth - currentWidth );
    }
    if ( mVAlignment == Qt::AlignVCenter )
    {
      yShift = ( newHeight - currentHeight ) / 2.0;
    }
    else if ( mVAlignment == Qt::AlignTop )
    {
      yShift = ( newHeight - currentHeight );
    }
  }
  else if ( mItemRotation >= 270 && mItemRotation < 360 )
  {
    if ( mHAlignment == Qt::AlignHCenter )
    {
      yShift = -( newHeight  - currentHeight ) / 2.0;
    }
    else if ( mHAlignment == Qt::AlignLeft )
    {
      yShift = -( newHeight  - currentHeight );
    }
    if ( mVAlignment == Qt::AlignBottom )
    {
      xShift = -( newWidth - currentWidth );
    }
    else if ( mVAlignment == Qt::AlignVCenter )
    {
      xShift = -( newWidth - currentWidth / 2.0 );
    }
  }
}
