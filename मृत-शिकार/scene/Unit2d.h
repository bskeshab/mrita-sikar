#pragma once
#include "../graphics/graphics.h"

class Unit2d
{
protected:
	struct FadeEffect
	{
		bool end; 
		double time; 
		double elapsedtime;
	};

	Sprite * m_sprite;
	SpriteAnimation m_sprAnim;
	bool m_animate;

	glm::vec2 m_position;
	bool m_visible;

	FadeEffect * m_fade;
public:
	Unit2d(const glm::vec2 &position = glm::vec2());

	virtual void Initialize(Sprite * sprite, glm::vec2 position = glm::vec2(), bool animate = false) { m_sprite = sprite; m_position = position; m_animate = animate; m_fade = NULL; m_visible = true; }
	virtual void CleanUp() {}

	void Animate(bool animate) { m_animate = animate; }
	void SetImageId(unsigned int index) { m_sprAnim.imageid = index; }

	virtual void Update(double deltaTime) { 
		if (m_fade)
		{
			m_fade->elapsedtime += deltaTime;
			if (m_fade->elapsedtime >= m_fade->time) { delete m_fade; m_fade = NULL; }
		}
		if (m_animate) m_sprite->Animate(m_sprAnim, deltaTime); 
	}
	virtual void Draw()
	{
		if (!m_visible) return;
		if (m_fade)
			m_sprite->DrawSprite(m_sprAnim, m_position.x, m_position.y, 1.0f, static_cast<float>(m_fade->elapsedtime/m_fade->time));
		else
			m_sprite->DrawSprite(m_sprAnim, m_position.x, m_position.y);
	}

	void SetPosition(const glm::vec2 &position) { m_position = position; }
	const glm::vec2 &GetPosition() { return m_position; }

	void SetVisible(bool visible) { m_visible = visible; }
	void Fade(double time)
	{
		m_fade = new FadeEffect;
		m_fade->elapsedtime = 0.0;
		m_fade->end = false;
		m_fade->time = time;
	}
};

