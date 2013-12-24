#include "UIController.h"
#include "UIElement.h"
#include "Slider.h"
#include "Button.h"
#include "Label.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace MinimalUI;

int UIController::DEFAULT_PANEL_WIDTH = 216;
int UIController::DEFAULT_MARGIN_LARGE = 10;
int UIController::DEFAULT_MARGIN_SMALL = 4;
int UIController::DEFAULT_UPDATE_FREQUENCY = 2;
int UIController::DEFAULT_FBO_WIDTH = 2048;
ci::ColorA UIController::DEFAULT_STROKE_COLOR = ci::ColorA( 0.07f, 0.26f, 0.29f, 1.0f );
ci::ColorA UIController::ACTIVE_STROKE_COLOR = ci::ColorA( 0.19f, 0.66f, 0.71f, 1.0f );

UIController::UIController( app::WindowRef aWindow, const string &aParamString )
: mWindow( aWindow ), mParamString( aParamString )
{
    JsonTree params( mParamString );
    mVisible = params.hasChild( "visible" ) ? params["visible"].getValue<bool>() : true;
    mAlpha = mVisible ? 1.0f : 0.0f;
    mWidth = params.hasChild( "width" ) ? params["width"].getValue<int>() : DEFAULT_PANEL_WIDTH;
    if ( params.hasChild( "height" ) ) {
        mHeightSpecified = true;
        mHeight = params["height"].getValue<int>();
    } else {
        mHeightSpecified = false;
        mHeight = getWindow()->getHeight();
    }
    mCentered = params.hasChild( "centered" ) ? params["centered"].getValue<bool>() : false;
    mDepth = params.hasChild( "depth" ) ? params["depth"].getValue<int>() : 0;
    mForceInteraction = params.hasChild( "forceInteraction" ) ? params["forceInteraction"].getValue<bool>() : false;
    mMarginLarge = params.hasChild( "marginLarge" ) ? params["marginLarge"].getValue<int>() : DEFAULT_MARGIN_LARGE;
    
    // JSON doesn't support hex literals...
    std::stringstream str;
    string panelColor = params.hasChild( "panelColor" ) ? params["panelColor"].getValue<string>() : "0xCC000000";
    str << panelColor;
    uint32_t hexValue;
    str >> std::hex >> hexValue;
    mPanelColor = ColorA::hexA( hexValue );
    
    resize();
    
	mCbMouseDown = mWindow->getSignalMouseDown().connect( mDepth + 99, std::bind( &UIController::mouseDown, this, std::placeholders::_1 ) );
    
    mLabelFont = Font( "Helvetica", 16 );
    mSmallLabelFont = Font( "Helvetica", 12 );
//    mIconFont = Font( loadResource( RES_GLYPHICONS_REGULAR ), 22 );
    mHeaderFont = Font( "Helvetica", 48 );
    mBodyFont = Font( "Garamond", 19 );
    mFooterFont = Font( "Garamond Italic", 14 );
    
    mInsertPosition = Vec2i( mMarginLarge, mMarginLarge );
    
    setupFbo();
}

UIControllerRef UIController::create( const string &aParamString )
{
    return shared_ptr<UIController>( new UIController( App::get()->getWindow(), aParamString ) );
}

void UIController::resize()
{
    Vec2i size;
    if ( mCentered ) {
        size = Vec2i( mWidth, mHeight );
        mPosition = getWindow()->getCenter() - size / 2;
    } else if ( mHeightSpecified ) {
        size = Vec2i( mWidth, mHeight );
        mPosition = Vec2i::zero();
    } else {
        size = Vec2i( mWidth, getWindow()->getHeight() );
        mPosition = Vec2i::zero();
    }
    mBounds = Area( Vec2i::zero(), size );
}

void UIController::mouseDown( MouseEvent &event )
{
    if ( mVisible ) {
        if ( (mBounds + mPosition).contains( event.getPos() ) || mForceInteraction )
        {
            event.setHandled();
        }
    }
}

void UIController::draw()
{
    if ( !mVisible )
        return;

    // start drawing to the Fbo
    mFbo.bindFramebuffer();

    gl::lineWidth( 2.0f );
    gl::enable( GL_LINE_SMOOTH );
    gl::enableAlphaBlending();
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // clear and set viewport and matrices
	gl::clear( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ) );
    gl::setViewport( mBounds + mPosition );
    gl::setMatricesWindow( mBounds.getSize(), false);
    
    // draw backing panel
    gl::color( mPanelColor );
    gl::drawSolidRect( mBounds );
    
    // draw elements
    for (unsigned int i = 0; i < mUIElements.size(); i++) {
        mUIElements[i]->draw();
    }

    // finish drawing to the Fbo
    mFbo.unbindFramebuffer();

    // reset the matrices and blending
    gl::setViewport( getWindow()->getBounds() );
    gl::setMatricesWindow( getWindow()->getSize() );
    gl::enableAlphaBlending( true );

    // if forcing interaction, draw an overlay over the whole window
    if ( mForceInteraction ) {
        gl::color(ColorA( 0.0f, 0.0f, 0.0f, 0.5f * mAlpha));
        gl::drawSolidRect(getWindow()->getBounds());
    }

    // draw the FBO to the screen
    gl::color( ColorA( mAlpha, mAlpha, mAlpha, mAlpha ) );
	gl::draw( mFbo.getTexture() );
}

void UIController::update()
{
    if ( !mVisible )
        return;

    if ( getElapsedFrames() % DEFAULT_UPDATE_FREQUENCY == 0 ) {
        for (unsigned int i = 0; i < mUIElements.size(); i++) {
            mUIElements[i]->update();
        }
    }
}

void UIController::show()
{
    mVisible = true;
    timeline().apply( &mAlpha, 1.0f, 0.25f );
}

void UIController::hide()
{
    timeline().apply( &mAlpha, 0.0f, 0.25f ).finishFn( [&]{ mVisible = false; } );
}

void UIController::addSlider( const string &aName, float *aValueToLink, const string &aParamString )
{
    mUIElements.push_back( Slider::create( this, aName, aValueToLink, aParamString ) );
}

void UIController::addButton( const string &aName, const function<void( bool )> &aEventHandler, const string &aParamString )
{
    mUIElements.push_back( Button::create( this, aName, aEventHandler, aParamString ) );
}

void UIController::addLinkedButton( const string &aName, const function<void( bool )> &aEventHandler, bool *aLinkedState, const string &aParamString )
{
    mUIElements.push_back( LinkedButton::create( this, aName, aEventHandler, aLinkedState, aParamString ) );
}

void UIController::addLabel( const string &aName, const string &aParamString )
{
    mUIElements.push_back( Label::create( this, aName, aParamString ) );
}

void UIController::addSlider2D( const string &aName, Vec2f *aValueToLink, const string &aParamString )
{
    mUIElements.push_back( Slider2D::create( this, aName, aValueToLink, aParamString ) );
}

void UIController::addToggleSlider( const string &aSliderName, float *aValueToLink, const string &aButtonName, const function<void (bool)> &aEventHandler, const string &aSliderParamString, const string &aButtonParamString )
{
    // create the slider
    UIElementRef newSlider = Slider::create( this, aSliderName, aValueToLink, aSliderParamString );
    
    // add the slider to the controller
    mUIElements.push_back( newSlider );

    // create the button
    UIElementRef newButtonRef = Button::create( this, aButtonName, aEventHandler, aButtonParamString );
    
    // add an additional event handler to link the button to the slider
    std::shared_ptr<class Button> newButton = std::static_pointer_cast<class Button>(newButtonRef);
    newButton->addEventHandler( std::bind(&Slider::setLocked, newSlider, std::placeholders::_1 ) );
    
    // add the button to the controller
    mUIElements.push_back( newButton );
}

void UIController::releaseGroup( const string &aGroup )
{
    for (unsigned int i = 0; i < mUIElements.size(); i++) {
        if (mUIElements[i]->getGroup() == aGroup ) {
            mUIElements[i]->release();
        }
    }
}

Font UIController::getFont( const string &aStyle )
{
    if ( aStyle == "icon" ) {
        return mIconFont;
    } else if ( aStyle == "header" ) {
        return mHeaderFont;
    } else if ( aStyle == "body" ) {
        return mBodyFont;
    } else if ( aStyle == "footer" ) {
        return mFooterFont;
    } else if ( aStyle == "smallLabel" ) {
        return mSmallLabelFont;
    } else {
        return mLabelFont;
    }
}

void UIController::setupFbo()
{
	mFormat.enableDepthBuffer( false );
    mFbo = gl::Fbo( DEFAULT_FBO_WIDTH, DEFAULT_FBO_WIDTH, mFormat );
    mFbo.bindFramebuffer();
	gl::clear( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ) );
    mFbo.unbindFramebuffer();
}