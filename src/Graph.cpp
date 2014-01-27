#include "Graph.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace MinimalUI;

int MovingGraph::DEFAULT_HEIGHT = UIElement::DEFAULT_HEIGHT;
int MovingGraph::DEFAULT_WIDTH = 96;

// MovingGraph
MovingGraph::MovingGraph( UIController *aUIController, const string &aName, float *aValueToLink, const string &aParamString )
	: UIElement( aUIController, aName, aParamString )
{
	// initialize unique variables
	mLinkedValue = aValueToLink;
	mMin = hasParam( "min" ) ? getParam<float>( "min" ) : 0.0f;
	mMax = hasParam( "max" ) ? getParam<float>( "max" ) : 1.0f;

	// set size
	int x = hasParam( "width" ) ? getParam<int>( "width" ) : MovingGraph::DEFAULT_WIDTH;
	int y = MovingGraph::DEFAULT_HEIGHT;
	setSize( Vec2i( x, y ) );

	// set position and bounds
	setPositionAndBounds();
	mScreenMin = mPosition.x;
	mScreenMax = mBounds.getX2();

	mBufferSize = 128;
	mScale = mBounds.getHeight()*.5f; 
	mInc = mBounds.getWidth()/((float)mBufferSize-1.0f);  
	mBuffer.push_back( 20.0 ); 
	// set screen value
	update();
}

UIElementRef MovingGraph::create( UIController *aUIController, const string &aName, float *aValueToLink, const string &aParamString )
{
	return shared_ptr<MovingGraph>( new MovingGraph( aUIController, aName, aValueToLink, aParamString ) );
}

void MovingGraph::draw()
{
	// draw the outer rect
	gl::color( UIController::DEFAULT_STROKE_COLOR );
	gl::drawStrokedRect( getBounds() );
	gl::drawSolidRect( getBounds() );

	// draw the graph
	gl::color( UIController::ACTIVE_STROKE_COLOR );
	ci::gl::pushMatrices();
	ci::gl::translate(mBounds.getX1(),mBounds.getY1()+mScale);

	mShape.clear();
	mShape.moveTo(0.0f, ci::lmap<float>(mBuffer[0], mMin, mMax, mScale, -mScale));
	for (int i = 1; i < mBuffer.size(); i++)
	{			
		mShape.lineTo( mInc*(float)i, ci::lmap<float>(mBuffer[i], mMin, mMax, mScale, -mScale));
	}             
	ci::gl::draw(mShape);
	ci::gl::popMatrices();   
}

void MovingGraph::update()
{
	mValue = *mLinkedValue;
	mBuffer.push_back( mValue );

	if( mBuffer.size() >= mBufferSize )
	{
		mBuffer.erase( mBuffer.begin(), mBuffer.begin() + 1 );
	}
}
