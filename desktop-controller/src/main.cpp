#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"

//========================================================================
int main( ){
	ofGLFWWindowSettings settings;
	settings.setSize(1280, 960);
	settings.setPosition(glm::vec2(60, 60));
	settings.resizable = true;
	settings.title = "Ant MIDI Desktop Controller";
	auto mainWindow = ofCreateWindow(settings);

	// settings / camera control window
	settings.setSize(420, 560);
	settings.setPosition(glm::vec2(1360, 60));
	settings.resizable = false;
	settings.title = "Settings";
	auto settingsWindow = ofCreateWindow(settings);
	settingsWindow->setVerticalSync(false);

	auto app = std::make_shared<ofApp>();
	ofAddListener(settingsWindow->events().draw, app.get(), &ofApp::drawSettings);
	ofAddListener(settingsWindow->events().mousePressed, app.get(), &ofApp::mousePressedSettings);

	ofRunApp(mainWindow, app);
	ofRunMainLoop();
}
