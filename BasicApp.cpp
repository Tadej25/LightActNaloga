#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CinderImGui.h"
#include "cinder/Rand.h"

#include <Windows.h>
#include <iostream>
#include <sstream>

using namespace ci;
using namespace ci::app;
namespace gui = ImGui;

#define VELO_DAMPIN 0.45f // 16 pixels

namespace PhysicsEngine {

	struct BallStruct {
		float ballRadius = ci::Rand::randInt(16,64);
		BallStruct(glm::vec2 pos, float spd = 5.0f) : position(pos), speed(spd) {}
		glm::vec2 position;
		glm::vec2 velocity;
		float speed;
		bool applyGravity = true;
	};
	struct BallForce {
		BallForce(const char* label, glm::vec2 force) : name(label), velocity(force) {}
		std::string name = "";
		glm::vec2 velocity;

		glm::vec2 friction = glm::vec2(0.0f);
	};
	struct EngineSettings {
		void Set(glm::ivec2 size) { bounds = size; }
		glm::ivec2 bounds;
	};

	class BallEngine {
	public:
		std::vector<ci::Color> colors = {
			ci::Color(ci::Rand::randFloat(0,1),ci::Rand::randFloat(0,1),ci::Rand::randFloat(0,1)),
			ci::Color(ci::Rand::randFloat(0,1),ci::Rand::randFloat(0,1),ci::Rand::randFloat(0,1))
		};
		std::vector<BallStruct*> balls;
		std::vector<BallForce*> forces;
		EngineSettings settings;
	public:
		BallEngine(glm::ivec2 bounds);
		~BallEngine();
		void Update();
		void Draw();
		void HandleCollisions();
		void AddRandomBall();
	};

	BallEngine::BallEngine(glm::ivec2 bounds) {
		settings.bounds = bounds;
		float friction = .95f;
		balls.push_back(new BallStruct(glm::vec2((float)settings.bounds.x * .25f, settings.bounds.y * .5f)));
		balls.push_back(new BallStruct(glm::vec2((float)settings.bounds.x * .75f, settings.bounds.y * .5f)));
		forces.push_back(new BallForce("Gravity", glm::vec2(0.f, .5f)));
		forces.push_back(new BallForce("Wind", glm::vec2(0.f, 0.f)));
		forces.push_back(new BallForce("Friction", glm::vec2(0.f, 0.f)));
	}

	BallEngine::~BallEngine() {
		while (balls.size()) {
			delete balls.front();
			balls.erase(balls.begin());
		}
		while (forces.size()) {
			delete forces.front();
			forces.erase(forces.begin());
		}
	}

	void BallEngine::Update() {

		HandleCollisions();

		for (BallStruct* ball : balls) {
			for (BallForce* force : forces) {
				if (force->name == "Gravity" && !ball->applyGravity) {
					// Skip applying gravity if applyGravity is false
					continue;
				}
				ball->velocity += force->velocity;

				// Apply friction force
				glm::vec2 frictionForce = -ball->velocity * force->friction;
				ball->velocity += frictionForce;
			}
			ball->position += ball->velocity;
			if (ball->position.y >= settings.bounds.y - ball->ballRadius * .5f) {
				if (ball->velocity.y > 0.f) {
					ball->velocity.y *= -VELO_DAMPIN;
					ball->position.y = settings.bounds.y - ball->ballRadius * .5f;
				}
			}
			if (ball->position.y <= ball->ballRadius * .5f) {
				if (ball->velocity.y < 0.f) {
					ball->velocity.y = 0.f;
					ball->position.y = ball->ballRadius * .5f;
				}
			}
			if (ball->position.x <= ball->ballRadius * .5f) {
				if (ball->velocity.x < 0.f) {
					ball->velocity.x *= -VELO_DAMPIN * VELO_DAMPIN;
					ball->position.x = ball->ballRadius * .5f;
				}
			}
			if (ball->position.x >= settings.bounds.x - ball->ballRadius * .5f) {
				if (ball->velocity.x > 0.f) {
					ball->velocity.x *= -VELO_DAMPIN * VELO_DAMPIN;
					ball->position.x = settings.bounds.x - ball->ballRadius * .5f;
				}
			}
		}
	}
	void BallEngine::Draw() {
		int index = 0;
		for (BallStruct* ball : balls) {
			ci::gl::ScopedColor color(colors[index++]);
			gl::drawSolidCircle(ball->position, ball->ballRadius);
		}
	}

	void BallEngine::HandleCollisions() {
		for (size_t i = 0; i < balls.size(); ++i) {
			for (size_t j = i + 1; j < balls.size(); ++j) {
				BallStruct* ball1 = balls[i];
				BallStruct* ball2 = balls[j];

				// Calculate the distance between the centers of the two balls
				float distance = glm::distance(ball1->position, ball2->position);

				if (distance < ball1->ballRadius + ball2->ballRadius) {
					// Balls are colliding, apply elastic collision
					glm::vec2 relativeVelocity = ball2->velocity - ball1->velocity;
					glm::vec2 collisionNormal = glm::normalize(ball2->position - ball1->position);

					float impulse = (2.0f * glm::dot(relativeVelocity, collisionNormal)) /
						(1.0f / 1.0f + 1.0f / 1.0f);  // Mass of balls are assumed to be 1.0f

					ball1->velocity += impulse * collisionNormal;
					ball2->velocity -= impulse * collisionNormal;
				}
			}
		}
	}

	void BallEngine::AddRandomBall() {
		glm::vec2 randomPosition(ci::Rand::randFloat(0, settings.bounds.x), ci::Rand::randFloat(0, settings.bounds.y));
		balls.push_back(new BallStruct(randomPosition));
		forces.push_back(new BallForce("Gravity", glm::vec2(0.f, .5f)));
		forces.push_back(new BallForce("Wind", glm::vec2(0.f, 0.f)));
		forces.push_back(new BallForce("Friction", glm::vec2(0.f, 0.f)));

		ci::Color randomColor(ci::Rand::randFloat(0, 1), ci::Rand::randFloat(0, 1), ci::Rand::randFloat(0, 1));
		colors.push_back(randomColor);
	}

}

class BasicApp : public App {

protected:

private:

	PhysicsEngine::BallEngine* engine = nullptr;

public:

	// Cinder will call 'mouseDrag' when the user moves the mouse while holding one of its buttons.
	// See also: mouseMove, mouseDown, mouseUp and mouseWheel.
	void mouseDrag(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;

	// Cinder will call 'keyDown' when the user presses a key on the keyboard.
	// See also: keyUp.
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override {}
	void resize() override;

	// Cinder will call 'draw' each time the contents of the window need to be redrawn.
	void draw() override;
	void update() override;
	void setup() override;

};

void prepareSettings(BasicApp::Settings* settings) {
	settings->setMultiTouchEnabled(false);
}

void BasicApp::mouseDown(MouseEvent event) {
	for (PhysicsEngine::BallStruct* ball : engine->balls) {
		if (glm::distance(ball->position, (glm::vec2)event.getPos()) < ball->ballRadius) {
			// what happens when you click on ball?
			ball->applyGravity = false;
			ball->velocity = glm::vec2(0,0);
		}
	}
}

void BasicApp::mouseUp(MouseEvent event) {
	for (PhysicsEngine::BallStruct* ball : engine->balls) {
		if (glm::distance(ball->position, (glm::vec2)event.getPos()) < ball->ballRadius) {
			// what happens when you click on ball (release click)?
			ball->applyGravity = true;

			// Calculate the direction vector from the ball to the mouse cursor
			glm::vec2 direction = glm::normalize((glm::vec2)event.getPos() - ball->position);

			// Set the ball's velocity to move in the direction of the mouse cursor
			if (!glm::isnan(direction.x) || !glm::isnan(direction.y)) {
				// 'direction' does NOT contain NaN values
				ball->velocity = direction * ball->speed;
			}
		}
	}
}

void BasicApp::mouseDrag(MouseEvent event) {
	for (PhysicsEngine::BallStruct* ball : engine->balls) {
		if (glm::distance(ball->position, (glm::vec2)event.getPos()) < ball->ballRadius) {
			// what happens when you click on ball?

			ball->position = (glm::vec2)event.getPos();
		}
	}
}

void BasicApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f') {
		// Toggle full screen when the user presses the 'f' key.
		setFullScreen(!isFullScreen());
	}
	else if (event.getCode() == KeyEvent::KEY_SPACE) {}
	else if (event.getCode() == KeyEvent::KEY_ESCAPE) {
		// Exit full screen, or quit the application, when the user presses the ESC key.
		if (isFullScreen())
			setFullScreen(false);
		else
			quit();
	}
}

void BasicApp::resize() {
	engine->settings.Set(ci::app::getWindowSize());
}

void BasicApp::setup() {
	gui::Initialize();
	engine = new PhysicsEngine::BallEngine(ci::app::getWindowSize());
}

void BasicApp::update() {
	engine->Update();
}

void BasicApp::draw() {

	// Clear the contents of the window. This call will clear
	// both the color and depth buffers. 
	gl::clear(Color::gray(0.1f));
	engine->Draw();

	{
		if (gui::Begin("Settings")) {
			// Check if the "Add Ball" button is clicked
			if (gui::Button("Add Ball")) {
				engine->AddRandomBall();
			}
			for (PhysicsEngine::BallStruct* ball : engine->balls) {
				gui::PushID(ball);
				gui::DragFloat2("Position", &ball->position);
				gui::DragFloat2("Velocity", &ball->velocity);
				gui::SliderFloat2("Speed", &ball->speed, 0.0f, 10.0f, "%.1f");
				gui::PopID();
			}
			gui::Separator();
			for (PhysicsEngine::BallForce* force : engine->forces) {
				gui::PushID(force);
				gui::TextDisabled(force->name.c_str());
				if (force->name != "Friction"){
					gui::SliderFloat2("Force", &force->velocity, 0.0f, 1.0f, "%.1f");
				}
				else {
					gui::SliderFloat2("Friction", &force->friction.x, 0.0f, 1.0f, "%.001f");
				}

				gui::PopID();
			}
			gui::End();
		}
	}

}

// This line tells Cinder to actually create and run the application.
CINDER_APP(BasicApp, RendererGl, prepareSettings)
