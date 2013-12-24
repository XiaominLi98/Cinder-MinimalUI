#include "UIElement.h"
#include "UIController.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace MinimalUI;

int UIElement::DEFAULT_HEIGHT = 36;
ci::ColorA UIElement::DEFAULT_NAME_COLOR = ci::ColorA( 0.14f, 0.49f, 0.54f, 1.0f );
ci::ColorA UIElement::DEFAULT_BACKGROUND_COLOR = ci::ColorA( 0.0f, 0.0f, 0.0f, 1.0f );

UIElement::UIElement( UIController *aUIController, const std::string &aName, const std::string &aParamString )
: mParent( aUIController ), mName( aName ), mParams( ci::JsonTree( aParamString ) )
{
    // attach mouse callbacks
	mCbMouseDown = mParent->getWindow()->getSignalMouseDown().connect( mParent->getDepth(), std::bind( &UIElement::mouseDown, this, std::placeholders::_1 ) );
    mCbMouseUp = mParent->getWindow()->getSignalMouseUp().connect( mParent->getDepth(), std::bind( &UIElement::mouseUp, this, std::placeholders::_1 ) );
    mCbMouseDrag = mParent->getWindow()->getSignalMouseDrag().connect( mParent->getDepth(), std::bind( &UIElement::mouseDrag, this, std::placeholders::_1 ) );
    
    // initialize some variables
    mActive = false;
    
    // parse params that are common to all UIElements
    mGroup = hasParam( "group" ) ? getParam<string>( "group" ) : "";
    mIcon = hasParam( "icon" ) ? getParam<bool>( "icon" ) : false;
    mLocked = hasParam( "locked" ) ? getParam<bool>( "locked" ) : false;
    mClear = hasParam( "clear" ) ? getParam<bool>( "clear" ) : true;

    // JSON doesn't support hex literals...
    // TODO: there's got to be a better way to do this, esp considering I need hex versions of default values set above...
    std::stringstream str;
    string strValue;
    uint32_t hexValue;
    strValue = hasParam( "nameColor" ) ? getParam<string>( "nameColor" ) : "0xFF237C89"; // should be same as DEFAULT_NAME_COLOR
    str << strValue;
    str >> std::hex >> hexValue;
    mNameColor = ColorA::hexA( hexValue );
    strValue = hasParam( "backgroundColor" ) ? getParam<string>( "backgroundColor" ) : "0xFF000000"; // should be same as DEFAULT_BACKGROUND_COLOR
    str.clear();
    str << strValue;
    str >> std::hex >> hexValue;
    mBackgroundColor = ColorA::hexA( hexValue );

    if ( hasParam( "justification" ) ) {
        string justification = getParam<string>( "justification" );
        if ( justification == "left" ) {
            mAlignment = TextBox::LEFT;
        } else if ( justification == "right" ) {
            mAlignment = TextBox::RIGHT;
        } else {
            mAlignment = TextBox::CENTER;
        }
    } else {
        mAlignment = TextBox::CENTER;
    }
    
    if ( hasParam( "style" ) ) {
        mFont = mParent->getFont( getParam<string>( "style" ) );
    } else if ( mIcon ) {
        mFont = mParent->getFont( "icon" );
    } else {
        mFont = mParent->getFont( "label" );
    }
}

void UIElement::offsetInsertPosition()
{
    if ( mClear ) {
        mParent->resetInsertPosition( mBounds.getHeight() + UIController::DEFAULT_MARGIN_SMALL );
    } else {
        mParent->offsetInsertPosition( Vec2i( mBounds.getWidth() + UIController::DEFAULT_MARGIN_SMALL, 0 ) );
    }
}

void UIElement::setPositionAndBounds()
{
    mPosition = mParent->getInsertPosition();
    mBounds = Area( mPosition, mPosition + mSize );
    
    // offset the insert position
    offsetInsertPosition();
}

void UIElement::mouseDown( MouseEvent &event )
{
    if ( mParent->isVisible() && !mLocked && mBounds.contains( event.getPos() - mParent->getPosition() ) ) {
        mActive = true;
        handleMouseDown( event.getPos() - mParent->getPosition() );
        event.setHandled();
    }
}

void UIElement::mouseUp( MouseEvent &event )
{
    if ( mParent->isVisible() && !mLocked && mActive ) {
        mActive = false;
        handleMouseUp( event.getPos() - mParent->getPosition() );
        //        event.setHandled(); // maybe?
    }
}

void UIElement::mouseDrag( MouseEvent &event )
{
    if ( mParent->isVisible() && !mLocked && mActive ) {
        handleMouseDrag( event.getPos() - mParent->getPosition() );
    }
}

void UIElement::renderNameTexture()
{
    TextBox textBox = TextBox().size( Vec2i( mSize.x, TextBox::GROW ) ).font( mFont ).color( mNameColor ).alignment( mAlignment ).text( mName );
    mNameTexture = textBox.render();
}

void UIElement::drawLabel()
{
    gl::pushMatrices();
    // intentional truncation
    Vec2i offset = mBounds.getCenter() - mNameTexture.getSize() / 2;
    gl::translate( offset );
    gl::color( Color::white() );
    gl::draw( mNameTexture );
    gl::popMatrices();
}