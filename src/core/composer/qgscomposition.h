/***************************************************************************
                              qgscomposition.h
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
#ifndef QGSCOMPOSITION_H
#define QGSCOMPOSITION_H

#include <memory>

#include <QDomDocument>
#include <QGraphicsScene>
#include <QLinkedList>
#include <QList>
#include <QPair>
#include <QSet>
#include <QUndoStack>
#include <QPrinter>
#include <QPainter>

#include "qgsaddremoveitemcommand.h"
#include "qgscomposeritemcommand.h"
#include "qgsatlascomposition.h"
#include "qgspaperitem.h"
#include "qgscomposeritem.h"

class QgisApp;
class QgsComposerFrame;
class QgsComposerMap;
class QGraphicsRectItem;
class QgsMapRenderer;
class QDomElement;
class QgsComposerArrow;
class QgsComposerMouseHandles;
class QgsComposerHtml;
class QgsComposerItem;
class QgsComposerLabel;
class QgsComposerLegend;
class QgsComposerMap;
class QgsComposerPicture;
class QgsComposerScaleBar;
class QgsComposerShape;
class QgsComposerAttributeTable;
class QgsComposerMultiFrame;
class QgsComposerMultiFrameCommand;
class QgsVectorLayer;
class QgsComposer;
class QgsFillSymbolV2;
class QgsDataDefined;

/** \ingroup MapComposer
 * Graphics scene for map printing. The class manages the paper item which always
 * is the item in the back (z-value 0). It maintains the z-Values of the items and stores
 * them in a list in ascending z-Order. This list can be changed to lower/raise items one position
 * or to bring them to front/back.
 * */
class CORE_EXPORT QgsComposition : public QGraphicsScene
{
    Q_OBJECT
  public:

    /** \brief Plot type */
    enum PlotStyle
    {
      Preview = 0, // Use cache etc
      Print,       // Render well
      Postscript   // Fonts need different scaling!
    };

    /**Style to draw the snapping grid*/
    enum GridStyle
    {
      Solid,
      Dots,
      Crosses
    };

    enum ZValueDirection
    {
      ZValueBelow,
      ZValueAbove
    };

    enum PaperOrientation
    {
      Portrait,
      Landscape
    };

    //! @deprecated since 2.4 - use the constructor with QgsMapSettings
    Q_DECL_DEPRECATED QgsComposition( QgsMapRenderer* mapRenderer );
    explicit QgsComposition( const QgsMapSettings& mapSettings );

    /**Composition atlas modes*/
    enum AtlasMode
    {
      AtlasOff,     // Composition is not being controlled by an atlas
      PreviewAtlas, // An atlas composition is being previewed in the app
      ExportAtlas   // The composition is being exported as an atlas
    };

    ~QgsComposition();

    /**Changes size of paper item. Also moves all items so that they retain
     * their same relative position to the top left corner of their current page.
    */
    void setPaperSize( double width, double height );

    /**Returns height of paper item*/
    double paperHeight() const;

    /**Returns width of paper item*/
    double paperWidth() const;

    double spaceBetweenPages() const { return mSpaceBetweenPages; }

    /**Note: added in version 1.9*/
    void setNumPages( int pages );
    /**Note: added in version 1.9*/
    int numPages() const;

    /**Note: added in version 2.1*/
    void setPageStyleSymbol( QgsFillSymbolV2* symbol );
    /**Note: added in version 2.1*/
    QgsFillSymbolV2* pageStyleSymbol() { return mPageStyleSymbol; }

    /**Returns the position within a page of a point in the composition
      @note Added in QGIS 2.1
    */
    QPointF positionOnPage( const QPointF & position ) const;

    /**Returns the page number corresponding to a point in the composition
      @note Added in QGIS 2.1
    */
    int pageNumberForPoint( const QPointF & position ) const;

    /**Sets the status bar message for the composer window
      @note Added in QGIS 2.1
    */
    void setStatusMessage( const QString & message );

    /**Refreshes the composition when composer related options change
     *Note: added in version 2.1*/
    void updateSettings();

    void setSnapToGridEnabled( bool b );
    bool snapToGridEnabled() const {return mSnapToGrid;}

    void setGridVisible( bool b );
    bool gridVisible() const {return mGridVisible;}

    /**Hides / shows custom snap lines*/
    void setSnapLinesVisible( bool visible );
    bool snapLinesVisible() const {return mGuidesVisible;}

    void setAlignmentSnap( bool s ) { mAlignmentSnap = s; }
    bool alignmentSnap() const { return mAlignmentSnap; }

    void setSmartGuidesEnabled( bool b ) { mSmartGuides = b; }
    bool smartGuidesEnabled() const {return mSmartGuides;}

    /**Removes all snap lines*/
    void clearSnapLines();

    void setSnapGridResolution( double r );
    double snapGridResolution() const {return mSnapGridResolution;}

    void setSnapGridOffsetX( double offset );
    double snapGridOffsetX() const {return mSnapGridOffsetX;}

    void setSnapGridOffsetY( double offset );
    double snapGridOffsetY() const {return mSnapGridOffsetY;}

    void setGridPen( const QPen& p );
    const QPen& gridPen() const {return mGridPen;}

    void setGridStyle( GridStyle s );
    GridStyle gridStyle() const {return mGridStyle;}

    /**Sets the snap tolerance to use when automatically snapping items during movement and resizing to the
     * composition grid.
     * @param tolerance snap tolerance in pixels
     * @see snapGridTolerance
     * @deprecated Use setSnapTolerance instead
    */
    Q_DECL_DEPRECATED void setSnapGridTolerance( double tolerance ) { mSnapTolerance = tolerance; }

    /**Returns the snap tolerance to use when automatically snapping items during movement and resizing to the
     * composition grid.
     * @returns snap tolerance in pixels
     * @see setSnapGridTolerance
     * @deprecated Use snapTolerance instead
    */
    Q_DECL_DEPRECATED double snapGridTolerance() const {return mSnapTolerance;}

    /**Sets the snap tolerance to use when automatically snapping items during movement and resizing to guides
     * and the edges and centers of other items.
     * @param t snap tolerance in pixels
     * @see alignmentSnapTolerance
     * @deprecated Use setSnapTolerance instead
    */
    Q_DECL_DEPRECATED void setAlignmentSnapTolerance( double t ) { mSnapTolerance = t; }

    /**Returns the snap tolerance to use when automatically snapping items during movement and resizing to guides
     * and the edges and centers of other items.
     * @returns snap tolerance in pixels
     * @see setAlignmentSnapTolerance
     * @deprecated Use snapTolerance instead
    */
    Q_DECL_DEPRECATED double alignmentSnapTolerance() const { return mSnapTolerance; }

    /**Sets the snap tolerance to use when automatically snapping items during movement and resizing to guides
     * and the edges and centers of other items.
     * @param snapTolerance snap tolerance in pixels
     * @see alignmentSnapTolerance
     * @note Added in QGIS 2.5
    */
    void setSnapTolerance( int snapTolerance ) { mSnapTolerance = snapTolerance; }

    /**Returns the snap tolerance to use when automatically snapping items during movement and resizing to guides
     * and the edges and centers of other items.
     * @returns snap tolerance in pixels
     * @see setAlignmentSnapTolerance
     * @note Added in QGIS 2.5
    */
    int snapTolerance() const { return mSnapTolerance; }

    /**Returns pointer to undo/redo command storage*/
    QUndoStack* undoStack() { return &mUndoStack; }

    /**Returns the topmost composer item. Ignores mPaperItem*/
    QgsComposerItem* composerItemAt( const QPointF & position );

    /**Returns the highest composer item at a specified position which is below a specified item. Ignores mPaperItem
      @note Added in QGIS 2.1
    */
    QgsComposerItem* composerItemAt( const QPointF & position, const QgsComposerItem* belowItem );

    /** Returns the page number (0-bsaed) given a coordinate */
    int pageNumberAt( const QPointF& position ) const;

    /** Returns on which page number (0-based) is displayed an item */
    int itemPageNumber( const QgsComposerItem* ) const;

    QList<QgsComposerItem*> selectedComposerItems();

    /**Returns pointers to all composer maps in the scene
      @note available in python bindings only with PyQt >= 4.8.4
      */
    QList<const QgsComposerMap*> composerMapItems() const;

    /**Return composer items of a specific type
      @note not available in python bindings
     */
    template<class T> void composerItems( QList<T*>& itemList );

    /**Returns the composer map with specified id
     @return QgsComposerMap or 0 pointer if the composer map item does not exist*/
    const QgsComposerMap* getComposerMapById( int id ) const;

    /**Returns the composer html with specified id (a string as named in the
      composer user interface item properties).
      @note Added in QGIS 2.0
      @param item the item.
      @return QgsComposerHtml pointer or 0 pointer if no such item exists.
     */
    const QgsComposerHtml* getComposerHtmlByItem( QgsComposerItem *item ) const;

    /**Returns a composer item given its text identifier.
       Ids are not necessarely unique, but this function returns only one element.
      @note added in 2.0
      @param theId - A QString representing the identifier of the item to
        retrieve.
      @return QgsComposerItem pointer or 0 pointer if no such item exists.
     */
    const QgsComposerItem* getComposerItemById( QString theId ) const;

    /**Returns a composer item given its unique identifier.
      @note added in 2.0
      @param theUuid A QString representing the UUID of the item to
      **/
    const QgsComposerItem* getComposerItemByUuid( QString theUuid ) const;

    int printResolution() const {return mPrintResolution;}
    void setPrintResolution( int dpi );

    bool printAsRaster() const {return mPrintAsRaster;}
    void setPrintAsRaster( bool enabled ) { mPrintAsRaster = enabled; }

    bool generateWorldFile() const { return mGenerateWorldFile; }
    void setGenerateWorldFile( bool enabled ) { mGenerateWorldFile = enabled; }

    QgsComposerMap* worldFileMap() const { return mWorldFileMap; }
    void setWorldFileMap( QgsComposerMap* map ) { mWorldFileMap = map; }

    /**Returns true if a composition should use advanced effects such as blend modes
      @note added in 1.9*/
    bool useAdvancedEffects() const {return mUseAdvancedEffects;}
    /**Used to enable or disable advanced effects such as blend modes in a composition
      @note: added in version 1.9*/
    void setUseAdvancedEffects( bool effectsEnabled );

    /**Returns pointer to map renderer of qgis map canvas*/
    //! @deprecated since 2.4 - use mapSettings() instead. May return null if not initialized with QgsMapRenderer
    Q_DECL_DEPRECATED QgsMapRenderer* mapRenderer() {return mMapRenderer;}

    //! Return setting of QGIS map canvas
    //! @note added in 2.4
    const QgsMapSettings& mapSettings() const { return mMapSettings; }

    QgsComposition::PlotStyle plotStyle() const {return mPlotStyle;}
    void setPlotStyle( QgsComposition::PlotStyle style ) {mPlotStyle = style;}

    /**Returns the pixel font size for a font that has point size set.
     The result depends on the resolution (dpi) and of the preview mode. Each item that sets
    a font should call this function before drawing text*/
    int pixelFontSize( double pointSize ) const;

    /**Does the inverse calculation and returns points for pixels (equals to mm in QgsComposition)*/
    double pointFontSize( int pixelSize ) const;

    /**Writes settings to xml (paper dimension)*/
    bool writeXML( QDomElement& composerElem, QDomDocument& doc );

    /**Reads settings from xml file*/
    bool readXML( const QDomElement& compositionElem, const QDomDocument& doc );

    /**Load a template document
        @param doc template document
        @param substitutionMap map with text to replace. Text needs to be enclosed by brackets (e.g. '[text]' )
        @param addUndoCommands whether or not to add undo commands
      */
    bool loadFromTemplate( const QDomDocument& doc, QMap<QString, QString>* substitutionMap = 0, bool addUndoCommands = false );

    /**Add items from XML representation to the graphics scene (for project file reading, pasting items from clipboard)
      @param elem items parent element, e.g. \verbatim <Composer> \endverbatim or \verbatim <ComposerItemClipboard> \endverbatim
      @param doc xml document
      @param mapsToRestore for reading from project file: set preview move 'rectangle' to all maps and save the preview states to show composer maps on demand
      @param addUndoCommands insert AddItem commands if true (e.g. for copy/paste)
      @param pos item position. Optional, take position from xml if 0
      @param pasteInPlace whether the position should be kept but mapped to the page origin. (the page is the page under to the mouse cursor)
      @note not available in python bindings
     */
    void addItemsFromXML( const QDomElement& elem, const QDomDocument& doc, QMap< QgsComposerMap*, int >* mapsToRestore = 0,
                          bool addUndoCommands = false, QPointF* pos = 0, bool pasteInPlace = false );

    /**Adds item to z list. Usually called from constructor of QgsComposerItem*/
    void addItemToZList( QgsComposerItem* item );
    /**Removes item from z list. Usually called from destructor of QgsComposerItem*/
    void removeItemFromZList( QgsComposerItem* item );

    //functions to move selected items in hierarchy
    void raiseSelectedItems();
    void raiseItem( QgsComposerItem* item );
    void lowerSelectedItems();
    void lowerItem( QgsComposerItem* item );
    void moveSelectedItemsToTop();
    void moveItemToTop( QgsComposerItem* item );
    void moveSelectedItemsToBottom();
    void moveItemToBottom( QgsComposerItem* item );

    //functions to find items by their position in the z list
    void selectNextByZOrder( ZValueDirection direction );
    QgsComposerItem* getComposerItemBelow( QgsComposerItem* item );
    QgsComposerItem* getComposerItemAbove( QgsComposerItem* item );

    //functions to align selected items
    void alignSelectedItemsLeft();
    void alignSelectedItemsHCenter();
    void alignSelectedItemsRight();
    void alignSelectedItemsTop();
    void alignSelectedItemsVCenter();
    void alignSelectedItemsBottom();

    //functions to lock and unlock items
    /**Lock the selected items*/
    void lockSelectedItems();
    /**Unlock all items*/
    void unlockAllItems();

    /**Sorts the zList. The only time where this function needs to be called is from QgsComposer
     after reading all the items from xml file*/
    void sortZList();

    /**Rebuilds the z order list based on current order of items in scene*/
    void refreshZList();

    /**Snaps a scene coordinate point to grid*/
    QPointF snapPointToGrid( const QPointF& scenePoint ) const;

    /**Returns pointer to snap lines collection*/
    QList< QGraphicsLineItem* >* snapLines() {return &mSnapLines;}

    /**Returns pointer to selection handles
     * @note not available in python bindings
     */
    QgsComposerMouseHandles* selectionHandles() {return mSelectionHandles;}

    /**Add a custom snap line (can be horizontal or vertical)*/
    QGraphicsLineItem* addSnapLine();
    /**Remove custom snap line (and delete the object)*/
    void removeSnapLine( QGraphicsLineItem* line );
    /**Get nearest snap line
     * @note not available in python bindings
     */
    QGraphicsLineItem* nearestSnapLine( bool horizontal, double x, double y, double tolerance, QList< QPair< QgsComposerItem*, QgsComposerItem::ItemPositionMode > >& snappedItems );

    /**Allocates new item command and saves initial state in it
      @param item target item
      @param commandText descriptive command text
      @param c context for merge commands (unknown for non-mergeable commands)*/
    void beginCommand( QgsComposerItem* item, const QString& commandText, QgsComposerMergeCommand::Context c = QgsComposerMergeCommand::Unknown );

    /**Saves end state of item and pushes command to the undo history*/
    void endCommand();
    /**Deletes current command*/
    void cancelCommand();

    void beginMultiFrameCommand( QgsComposerMultiFrame* multiFrame, const QString& text );
    void endMultiFrameCommand();

    /**Adds multiframe. The object is owned by QgsComposition until removeMultiFrame is called*/
    void addMultiFrame( QgsComposerMultiFrame* multiFrame );
    /**Removes multi frame (but does not delete it)*/
    void removeMultiFrame( QgsComposerMultiFrame* multiFrame );
    /**Adds an arrow item to the graphics scene and advices composer to create a widget for it (through signal)
      @note not available in python bindings*/
    void addComposerArrow( QgsComposerArrow* arrow );
    /**Adds label to the graphics scene and advices composer to create a widget for it (through signal)*/
    void addComposerLabel( QgsComposerLabel* label );
    /**Adds map to the graphics scene and advices composer to create a widget for it (through signal)*/
    void addComposerMap( QgsComposerMap* map, bool setDefaultPreviewStyle = true );
    /**Adds scale bar to the graphics scene and advices composer to create a widget for it (through signal)*/
    void addComposerScaleBar( QgsComposerScaleBar* scaleBar );
    /**Adds legend to the graphics scene and advices composer to create a widget for it (through signal)*/
    void addComposerLegend( QgsComposerLegend* legend );
    /**Adds picture to the graphics scene and advices composer to create a widget for it (through signal)*/
    void addComposerPicture( QgsComposerPicture* picture );
    /**Adds a composer shape to the graphics scene and advices composer to create a widget for it (through signal)*/
    void addComposerShape( QgsComposerShape* shape );
    /**Adds a composer table to the graphics scene and advices composer to create a widget for it (through signal)*/
    void addComposerTable( QgsComposerAttributeTable* table );
    /**Adds composer html frame and advices composer to create a widget for it (through signal)*/
    void addComposerHtmlFrame( QgsComposerHtml* html, QgsComposerFrame* frame );

    /**Remove item from the graphics scene. Additionally to QGraphicsScene::removeItem, this function considers undo/redo command*/
    void removeComposerItem( QgsComposerItem* item, bool createCommand = true );

    /**Convenience function to create a QgsAddRemoveItemCommand, connect its signals and push it to the undo stack*/
    void pushAddRemoveCommand( QgsComposerItem* item, const QString& text, QgsAddRemoveItemCommand::State state = QgsAddRemoveItemCommand::Added );

    /**If true, prevents any mouse cursor changes by the composition or by any composer items
      Used by QgsComposer and QgsComposerView to prevent unwanted cursor changes*/
    void setPreventCursorChange( bool preventChange ) { mPreventCursorChange = preventChange; }
    bool preventCursorChange() { return mPreventCursorChange; }

    //printing

    /** Prepare the printer for printing */
    void beginPrint( QPrinter& printer );
    /** Prepare the printer for printing in a PDF */
    void beginPrintAsPDF( QPrinter& printer, const QString& file );

    /**Print on a preconfigured printer
     * @param printer QPrinter destination
     * @painter QPainter source
     * @startNewPage set to true to begin the print on a new page
     */
    void doPrint( QPrinter& printer, QPainter& painter, bool startNewPage = false );

    /**Convenience function that prepares the printer and prints
     * @returns true if print was successful
    */
    bool print( QPrinter &printer );

    /**Convenience function that prepares the printer for printing in PDF and prints
     * @returns true if export was successful
    */
    bool exportAsPDF( const QString& file );

    //! print composer page to image
    //! If the image does not fit into memory, a null image is returned
    QImage printPageAsRaster( int page );

    /**Render a page to a paint device
        @note added in version 1.9*/
    void renderPage( QPainter* p, int page );

    /** Compute world file parameters */
    void computeWorldFileParameters( double& a, double& b, double& c, double& d, double& e, double& f ) const;

    QgsAtlasComposition& atlasComposition() { return mAtlasComposition; }

    /**Resizes a QRectF relative to the change from boundsBefore to boundsAfter*/
    static void relativeResizeRect( QRectF& rectToResize, const QRectF& boundsBefore, const QRectF& boundsAfter );
    /**Returns a scaled position given a before and after range*/
    static double relativePosition( double position, double beforeMin, double beforeMax, double afterMin, double afterMax );

    /** Returns the current atlas mode of the composition */
    QgsComposition::AtlasMode atlasMode() const { return mAtlasMode; }
    /** Sets the current atlas mode of the composition. Returns false if the mode could not be changed. */
    bool setAtlasMode( QgsComposition::AtlasMode mode );

    /** Return pages in the correct order
     @note composerItems(QList< QgsPaperItem* > &) may not return pages in the correct order
     @note added in version 2.4*/
    QList< QgsPaperItem* > pages() { return mPages; }

    /**Returns a reference to the data defined settings for one of the composition's data defined properties.
     * @param property data defined property to return
     * @note this method was added in version 2.5
    */
    QgsDataDefined* dataDefinedProperty( QgsComposerItem::DataDefinedProperty property );

    /**Sets parameters for a data defined property for the composition
     * @param property data defined property to set
     * @param active true if data defined property is active, false if it is disabled
     * @param useExpression true if the expression should be used
     * @param expression expression for data defined property
     * @field field name if the data defined property should take its value from a field
     * @note this method was added in version 2.5
    */
    void setDataDefinedProperty( QgsComposerItem::DataDefinedProperty property, bool active, bool useExpression, const QString &expression, const QString &field );

  public slots:
    /**Casts object to the proper subclass type and calls corresponding itemAdded signal*/
    void sendItemAddedSignal( QgsComposerItem* item );

    /**Updates the scene bounds of the composition
    @note added in version 2.2*/
    void updateBounds();

    /**Forces items in the composition to refresh. For instance, this causes maps to redraw
     * and rebuild cached images, html items to reload their source url, and attribute tables
     * to refresh their contents. Calling this also triggers a recalculation of all data defined
     * attributes within the composition.
     * @note added in version 2.3*/
    void refreshItems();

    /**Clears any selected items and sets an item as the current selection.
     * @param item item to set as selected
     * @note added in version 2.3*/
    void setSelectedItem( QgsComposerItem* item );

    /**Refreshes a data defined property for the composition by reevaluating the property's value
     * and redrawing the composition with this new value.
     * @param property data defined property to refresh. If property is set to
     * QgsComposerItem::AllProperties then all data defined properties for the composition will be
     * refreshed.
     * @note this method was added in version 2.5
    */
    void refreshDataDefinedProperty( QgsComposerItem::DataDefinedProperty property = QgsComposerItem::AllProperties );

  protected:
    void init();


  private:
    /**Pointer to map renderer of QGIS main map*/
    QgsMapRenderer* mMapRenderer;
    const QgsMapSettings& mMapSettings;

    QgsComposition::PlotStyle mPlotStyle;
    double mPageWidth;
    double mPageHeight;
    QList< QgsPaperItem* > mPages;
    double mSpaceBetweenPages; //space in preview between pages

    /**Drawing style for page*/
    QgsFillSymbolV2* mPageStyleSymbol;
    void createDefaultPageStyleSymbol();

    /**Maintains z-Order of items. Starts with item at position 1 (position 0 is always paper item)*/
    QLinkedList<QgsComposerItem*> mItemZList;

    /**List multiframe objects*/
    QSet<QgsComposerMultiFrame*> mMultiFrames;

    /**Dpi for printout*/
    int mPrintResolution;

    /**Flag if map should be printed as a raster (via QImage). False by default*/
    bool mPrintAsRaster;

    /**Flag if a world file should be generated on raster export */
    bool mGenerateWorldFile;
    /** Composer map to use for the world file generation */
    QgsComposerMap* mWorldFileMap;

    /**Flag if advanced visual effects such as blend modes should be used. True by default*/
    bool mUseAdvancedEffects;

    /**Parameters for snap to grid function*/
    bool mSnapToGrid;
    bool mGridVisible;
    double mSnapGridResolution;
    double mSnapGridOffsetX;
    double mSnapGridOffsetY;
    QPen mGridPen;
    GridStyle mGridStyle;

    /**Parameters for alignment snap*/
    bool mAlignmentSnap;
    bool mGuidesVisible;
    bool mSmartGuides;
    int mSnapTolerance;

    /**Arbitraty snap lines (horizontal and vertical)*/
    QList< QGraphicsLineItem* > mSnapLines;

    QgsComposerMouseHandles* mSelectionHandles;

    QUndoStack mUndoStack;

    QgsComposerItemCommand* mActiveItemCommand;
    QgsComposerMultiFrameCommand* mActiveMultiFrameCommand;

    /** The atlas composition object. It is held by the QgsComposition */
    QgsAtlasComposition mAtlasComposition;

    QgsComposition::AtlasMode mAtlasMode;

    bool mPreventCursorChange;

    /**Map of data defined properties for the composition to string name to use when exporting composition to xml*/
    QMap< QgsComposerItem::DataDefinedProperty, QString > mDataDefinedNames;
    /**Map of current data defined properties to QgsDataDefined for the composition*/
    QMap< QgsComposerItem::DataDefinedProperty, QgsDataDefined* > mDataDefinedProperties;

    QgsComposition(); //default constructor is forbidden

    /**Calculates the bounds of all non-gui items in the composition. Ignores snap lines and mouse handles*/
    QRectF compositionBounds() const;

    /**Reset z-values of items based on position in z list*/
    void updateZValues( bool addUndoCommands = true );

    /**Returns the bounding rectangle of the selected items in scene coordinates
     @return 0 in case of success*/
    int boundingRectOfSelectedItems( QRectF& bRect );

    /**Loads default composer settings*/
    void loadDefaults();

    /**Loads composer settings which may change, eg grid color*/
    void loadSettings();

    /**Calculates the item minimum position from an xml string*/
    QPointF minPointFromXml( const QDomElement& elem ) const;

    void connectAddRemoveCommandSignals( QgsAddRemoveItemCommand* c );

    void updatePaperItems();
    void addPaperItem();
    void removePaperItems();
    void deleteAndRemoveMultiFrames();

    static QString encodeStringForXML( const QString& str );

    //tries to return the current QGraphicsView attached to the composition
    QGraphicsView* graphicsView() const;

    /*Recalculates the page size using data defined page settings*/
    void refreshPageSize();

    /*Decodes a string representing a paper orientation*/
    QgsComposition::PaperOrientation decodePaperOrientation( QString orientationString, bool &ok );

    /*Decodes a string representing a preset page size*/
    bool decodePresetPaperSize( QString presetString, double &width, double &height );

    /**Evaluate a data defined property and return the calculated value
     * @returns true if data defined property could be successfully evaluated
     * @param property data defined property to evaluate
     * @param expressionValue QVariant for storing the evaluated value
     * @param dataDefinedProperties map of data defined properties to QgsDataDefined
     * @note this method was added in version 2.5
    */
    bool dataDefinedEvaluate( QgsComposerItem::DataDefinedProperty property, QVariant &expressionValue,
                              QMap< QgsComposerItem::DataDefinedProperty, QgsDataDefined* >* dataDefinedProperties );

    /**Reads all data defined properties from xml
     * @param itemElem dom element containing data defined properties
     * @param dataDefinedNames map of data defined property to name used within xml
     * @param dataDefinedProperties map of data defined properties to QgsDataDefined in which to store properties from xml
     * @note this method was added in version 2.5
    */
    void readDataDefinedPropertyMap( const QDomElement &itemElem,
                                     QMap< QgsComposerItem::DataDefinedProperty, QString >* dataDefinedNames,
                                     QMap< QgsComposerItem::DataDefinedProperty, QgsDataDefined* >* dataDefinedProperties
                                   ) const;

    /**Reads a single data defined property from xml DOM element
     * @param property data defined property to read
     * @param ddElem dom element containing settings for data defined property
     * @param dataDefinedProperties map of data defined properties to QgsDataDefined in which to store properties from xml
     * @note this method was added in version 2.5
    */
    void readDataDefinedProperty( QgsComposerItem::DataDefinedProperty property, const QDomElement &ddElem,
                                  QMap< QgsComposerItem::DataDefinedProperty, QgsDataDefined* >* dataDefinedProperties ) const;

    /**Writes data defined properties to xml
     * @param itemElem DOM element in which to store data defined properties
     * @param doc DOM document
     * @param dataDefinedNames map of data defined property to name used within xml
     * @param dataDefinedProperties map of data defined properties to QgsDataDefined for storing in xml
     * @note this method was added in version 2.5
    */
    void writeDataDefinedPropertyMap( QDomElement &itemElem, QDomDocument &doc,
                                      const QMap< QgsComposerItem::DataDefinedProperty, QString >* dataDefinedNames,
                                      const QMap< QgsComposerItem::DataDefinedProperty, QgsDataDefined* >* dataDefinedProperties ) const;

    /**Evaluates a data defined property and returns the calculated value.
     * @param property data defined property to evaluate
     * @param feature current atlas feature to evaluate property for
     * @param fields fields from atlas layer
     * @param dataDefinedProperties map of data defined properties to QgsDataDefined
     * @note this method was added in version 2.5
    */
    QVariant dataDefinedValue( QgsComposerItem::DataDefinedProperty property, const QgsFeature *feature, const QgsFields *fields,
                               QMap<QgsComposerItem::DataDefinedProperty, QgsDataDefined *> *dataDefinedProperties );


    /**Prepares the expression for a data defined property, using the current atlas layer if set.
     * @param dd data defined to prepare. If no data defined is set, all data defined expressions will be prepared
     * @param dataDefinedProperties map of data defined properties to QgsDataDefined
     * @note this method was added in version 2.5
    */
    void prepareDataDefinedExpression( QgsDataDefined *dd, QMap< QgsComposerItem::DataDefinedProperty, QgsDataDefined* >* dataDefinedProperties ) const;

  private slots:
    /*Prepares all data defined expressions*/
    void prepareAllDataDefinedExpressions();

  signals:
    void paperSizeChanged();
    void nPagesChanged();

    /**Is emitted when the compositions print resolution changes*/
    void printResolutionChanged();

    /**Is emitted when selected item changed. If 0, no item is selected*/
    void selectedItemChanged( QgsComposerItem* selected );
    /**Is emitted when new composer arrow has been added to the view*/
    void composerArrowAdded( QgsComposerArrow* arrow );
    /**Is emitted when a new composer html has been added to the view*/
    void composerHtmlFrameAdded( QgsComposerHtml* html, QgsComposerFrame* frame );
    /**Is emitted when new composer label has been added to the view*/
    void composerLabelAdded( QgsComposerLabel* label );
    /**Is emitted when new composer map has been added to the view*/
    void composerMapAdded( QgsComposerMap* map );
    /**Is emitted when new composer scale bar has been added*/
    void composerScaleBarAdded( QgsComposerScaleBar* scalebar );
    /**Is emitted when a new composer legend has been added*/
    void composerLegendAdded( QgsComposerLegend* legend );
    /**Is emitted when a new composer picture has been added*/
    void composerPictureAdded( QgsComposerPicture* picture );
    /**Is emitted when a new composer shape has been added*/
    void composerShapeAdded( QgsComposerShape* shape );
    /**Is emitted when a new composer table has been added*/
    void composerTableAdded( QgsComposerAttributeTable* table );
    /**Is emitted when a composer item has been removed from the scene*/
    void itemRemoved( QgsComposerItem* );

    /**Is emitted when item in the composition must be refreshed*/
    void refreshItemsTriggered();

    /**Is emitted when the composition has an updated status bar message for the composer window*/
    void statusMsgChanged( QString message );

    friend class QgsComposerItem; //for accessing dataDefinedEvaluate, readDataDefinedPropertyMap and writeDataDefinedPropertyMap
};

template<class T> void QgsComposition::composerItems( QList<T*>& itemList )
{
  itemList.clear();
  QList<QGraphicsItem *> graphicsItemList = items();
  QList<QGraphicsItem *>::iterator itemIt = graphicsItemList.begin();
  for ( ; itemIt != graphicsItemList.end(); ++itemIt )
  {
    T* item = dynamic_cast<T*>( *itemIt );
    if ( item )
    {
      itemList.push_back( item );
    }
  }
}

#endif



