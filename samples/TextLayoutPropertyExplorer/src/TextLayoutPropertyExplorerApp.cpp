#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/params/Params.h"

#include "StyleManager.h"
#include "FontManager.h"
#include "StyledTextLayout.h"
#include "StyledTextParser.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::text;

class TextLayoutPropertyExplorerApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void resize() override;

	void loadText();
	void updateText();

protected:

	StyledTextLayout mTextLayout;
	ci::Surface mTextSurface;
	gl::TextureRef mTextTexture;

	ci::params::InterfaceGlRef mParams;

	vector<string> mParamClipModes;
	vector<string> mParamLayoutModes;
	std::string mLoremIpsum;

	bool mForceUpdates = false;
	bool mOscillate = false;
	bool mDrawTextBoxOnTop = false;
};

void TextLayoutPropertyExplorerApp::setup()
{
	// Set up text parser
	StyledTextParser::getInstance()->setDefaultOptions(StyledTextParser::OptionFlags::INVERT_NESTED_ITALICS);

	// Set up style manager
	StyleManager::getInstance()->setup(getAssetPath("Fonts/styles.json"), "styles");

	// Set up font manager
	FontManager::getInstance()->setLogLevel(FontManager::LogLevel::Warning);
	FontManager::getInstance()->setup(getAssetPath("Fonts/fonts.json"));

	// Test content
	loadText();

	// Configure text
	mTextLayout.setMaxWidth((float)getWindowWidth() * 0.5f);
	mTextLayout.setClipMode(StyledTextLayout::ClipMode::Clip);
	mTextLayout.setLayoutMode(StyledTextLayout::LayoutMode::WordWrap);
	mTextLayout.setPadding(10.f, 10.f, 10.f, 10.f);

	// sets and renders text
	updateText();

	// Configure params
	mParamClipModes.push_back("Clip");
	mParamClipModes.push_back("NoClip");

	mParamLayoutModes.push_back("WordWrap");
	mParamLayoutModes.push_back("NoWrap");
	mParamLayoutModes.push_back("SingleLine");

	mParams = ci::params::InterfaceGl::create("Settings", ivec2(400, 400), ColorA(0, 0, 0, 0.7f));
	mParams->addParam("Text", &mLoremIpsum).updateFn([&] { updateText(); });
	mParams->addButton("Reload", [&] { loadText(); updateText(); });
	mParams->addSeparator();
	mParams->addParam("Clip Mode", mParamClipModes, [&](int v) { mTextLayout.setClipMode((StyledTextLayout::ClipMode)v); }, [&] { return (int)mTextLayout.getClipMode(); });
	mParams->addParam("Layout Mode", mParamLayoutModes, [&](int v) { mTextLayout.setLayoutMode((StyledTextLayout::LayoutMode)v); }, [&] { return (int)mTextLayout.getLayoutMode(); });
	mParams->addSeparator();
	mParams->addParam<float>("Max Width", [&](float v) { mTextLayout.setMaxWidth(v); }, [&] { return mTextLayout.getMaxWidth(); });
	mParams->addParam<float>("Padding Top", [&](float v) { mTextLayout.setPaddingTop(v); }, [&] { return mTextLayout.getPaddingTop(); });
	mParams->addParam<float>("Padding Right", [&](float v) { mTextLayout.setPaddingRight(v); }, [&] { return mTextLayout.getPaddingRight(); });
	mParams->addParam<float>("Padding Bottom", [&](float v) { mTextLayout.setPaddingBottom(v); }, [&] { return mTextLayout.getPaddingBottom(); });
	mParams->addParam<float>("Padding Left", [&](float v) { mTextLayout.setPaddingLeft(v); }, [&] { return mTextLayout.getPaddingLeft(); });
	mParams->addSeparator();
	mParams->addParam("Oscillate", &mOscillate);
	mParams->addParam("Force Updates", &mForceUpdates);
}

void TextLayoutPropertyExplorerApp::update() {
	if (mTextLayout.hasChanges() && !mForceUpdates) {
		updateText();
	}
}

void TextLayoutPropertyExplorerApp::loadText() {
	mLoremIpsum = loadString(loadAsset("lorem_ipsum.txt"));
}

void TextLayoutPropertyExplorerApp::updateText() {
	mTextLayout.setText("This is a test title", "test.title");
	mTextLayout.appendText("<br>" + mLoremIpsum, "test.body");

	mTextSurface = mTextLayout.renderToSurface();
	mTextTexture = gl::Texture::create(mTextSurface);
}

void TextLayoutPropertyExplorerApp::draw()
{
	gl::clear(Color(0.3f, 0.3f, 0.3f));

	{
		gl::ScopedMatrices scopedMatrices;

		// update here to react to window resize immediately
		if (mForceUpdates) {
			updateText();
		}

		// oscilate view pos
		if (mOscillate) {
			gl::translate(
				20.0f * cosf((float)getElapsedSeconds()),
				20.0f * sinf(0.3f * (float)getElapsedSeconds())
			);
		}

		// draw text
		gl::color(Color::white());
		gl::draw(mTextTexture);

		// draw padding
		gl::color(ColorA(1.0f, 0.0f, 0.0f, 0.75f));
		gl::drawStrokedRect(ci::Rectf(mTextLayout.getPaddingLeft(), mTextLayout.getPaddingTop(), mTextTexture->getWidth() - mTextLayout.getPaddingRight(), mTextTexture->getHeight() - mTextLayout.getPaddingBottom()));

		// draw max width
		gl::color(ColorA(0.0f, 0.0f, 1.0f, 0.75f));
		gl::drawStrokedRect(ci::Rectf(0.0f, 0.0f, mTextLayout.getMaxWidth(), (float)mTextTexture->getHeight()));
	}

	// draw debug info
	mParams->draw();

	gl::color(ColorA(0, 0, 0, 0.5f));
	gl::drawSolidRect(Rectf(0, (float)getWindowHeight() - 25.0f, 160.0f, (float)getWindowHeight()));
	gl::drawString(to_string(getAverageFps()), vec2(0, (float)getWindowHeight() - 25.0f), ColorA(1.0f, 0.0f, 0.0f, 1.0f), FontManager::getInstance()->getFont("Arial", 32));

}

void TextLayoutPropertyExplorerApp::resize() {
	float width = (float)getWindowWidth() * 0.5f;
	mTextLayout.setMaxWidth(width);

	updateText();
	
	ivec2 paramsPos = getWindowSize() - mParams->getSize() - ivec2(16);
	if (paramsPos.x < 0) paramsPos.x = 0;
	if (paramsPos.y < 0) paramsPos.y = 0;
	mParams->setPosition(paramsPos);
}

CINDER_APP(TextLayoutPropertyExplorerApp,
	RendererGl(RendererGl::Options().msaa(2)),
	[&](ci::app::App::Settings *settings)
{
	settings->setWindowSize(900, 640);
})