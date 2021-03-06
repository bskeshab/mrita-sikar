
#include "LiveUnit.h"

GameScene::GameScene(Renderer * renderer) : Scene(renderer), m_skybox(renderer)
{
}

void GameScene::Initialize(const Rect &area)
{
	Scene::Initialize();
	m_quadTree.Initialize(0, area);
	m_skybox.Initialize();
}

void GameScene::Resize(float width, float height)
{
	if (!m_camera) return;
	m_camera->UpdateProjection(width, height);
	m_renderer->Resize(width, height, m_camera->GetProjection());
}

void GameScene::Update(double deltaTime)
{
	if (m_camera) m_camera->UpdateView(deltaTime);

	for (unsigned i = 0; i < m_units.size(); ++i)
		m_units[i]->Update(deltaTime);
	for (unsigned i = 0; i < m_billboards.size(); ++i)
		m_billboards[i]->Update(deltaTime);
	for (unsigned i = 0; i < m_unit2ds.size(); ++i)
		m_unit2ds[i]->Update(deltaTime);
}

void GameScene::Draw()
{
	if (!m_camera) return;

	m_firstPass = true;
	glm::mat4 camInverse = glm::inverse(m_camera->GetView());
	glm::vec3 target = glm::vec3(camInverse[3]);// -camInverse[2]);
	m_renderer->SetupLightMatrix(glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f)), target, 10000, 5000, -10000, 10000000);


	m_renderer->BeginRender(Renderer::SHADOW_PASS);
	for (unsigned i = 0; i < m_units.size(); ++i)
			m_units[i]->Draw();
	/*
	// For frustum-culling during depth-map rendering
	Frustum m_shadowfrustum;
	m_shadowfrustum.FromMatrix(m_renderer->GetLightViewProjection());
	for (unsigned i = 0; i < m_units.size(); ++i)
	{
		bool toDraw;
		if (m_units[i]->IsLiveUnit())
			toDraw = m_shadowfrustum.BoxInFrustum(static_cast<LiveUnit*>(m_units[i])->GetAABB());
		else
			toDraw = m_shadowfrustum.BoxInFrustum(m_units[i]->GetBoundParent());
		if (toDraw)
			m_units[i]->Draw();
	}
	*/

	m_firstPass = false;
	m_renderer->UpdateView(m_camera->GetView());
	m_renderer->BeginRender(Renderer::NORMAL_PASS);

	for (unsigned i = 0; i < m_units.size(); ++i)
	{
		bool toDraw;
		if (m_units[i]->IsLiveUnit())
			toDraw = m_camera->IntersectBox(static_cast<LiveUnit*>(m_units[i])->GetAABB());
		else
			toDraw = m_camera->IntersectBox(m_units[i]->GetBoundParent());

		m_units[i]->SetVisible(toDraw);
		if (toDraw)
			m_units[i]->Draw();
	}

	m_skybox.Draw(glm::vec3(camInverse[3]));

	for (unsigned i = 0; i < m_billboards.size(); ++i)
		m_billboards[i]->Draw();

	m_renderer->DisableDepth();
	for (unsigned i = 0; i < m_unit2ds.size(); ++i)
		m_unit2ds[i]->Draw();
	m_renderer->EnableDepth();
	DrawTexts();

	m_renderer->EndRender();
}

void GameScene::CleanUp()
{
	Scene::CleanUp();
	m_units.clear();
	m_billboards.clear();
	m_skybox.CleanUp();
}

bool GameScene::CheckPotentialCollision(const Unit * unit1, const Unit * unit2) const
{
	const Box &bx1 = unit1->GetBoundParent();
	const Box &bx2 = unit2->GetBoundParent();
	if (unit1->IsLiveUnit())
	{
		const Box &bx11 = static_cast<const LiveUnit*>(unit1)->GetAABB();
		if (unit2->IsLiveUnit())
			return bx11.IntersectBox(static_cast<const LiveUnit*>(unit2)->GetAABB());
		else
			return bx11.IntersectBox(bx2);
	}
	else
	{
		if (unit2->IsLiveUnit())
			return bx1.IntersectBox(static_cast<const LiveUnit*>(unit2)->GetAABB());
		else
			return bx1.IntersectBox(bx2);
	}
}



Unit* GameScene::GetNearestIntersection(const Ray &ray, float * ttmin, const Unit * ignoreUnit) const
{
	UnitCollections unitCollections;
	m_quadTree.GetPotentialCollisions(ray, unitCollections);
	Unit * nearestUnit = NULL;
	float tmin = FLT_MAX;
	for (unsigned int i = 0; i < unitCollections.size(); ++i)
	{
		Unit * testUnit = unitCollections[i];
		if (testUnit == ignoreUnit) continue;
		bool test; float t;
		if (testUnit->IsLiveUnit())
			test = ray.IntersectBox(testUnit->GetBoundParent(), static_cast<glm::mat3>(testUnit->GetOrient()), t);
		//test = ray.IntersectBox(((LiveUnit*)testUnit)->GetAABB(), t);
		else
			test = ray.IntersectBox(testUnit->GetBoundParent(), t);

		if (test && t < tmin)
		{
			nearestUnit = testUnit;
			tmin = t;
		}
	}
	if (ttmin) *ttmin = tmin;
	return nearestUnit;
}

Unit* GameScene::GetNearestIntersection(const Ray &ray, int &child, float &tmin, const Unit * ignoreUnit) const
{
	UnitCollections unitCollections;
	m_quadTree.GetPotentialCollisions(ray, unitCollections);
	Unit * nearestUnit = NULL;
	tmin = FLT_MAX;
	for (unsigned int i = 0; i < unitCollections.size(); ++i)
	{
		Unit * testUnit = unitCollections[i];
		if (testUnit == ignoreUnit) continue;
		bool test; float t;
		if (testUnit->IsLiveUnit())
			test = ray.IntersectBox(testUnit->GetBoundParent(), static_cast<glm::mat3>(testUnit->GetOrient()), t);
		else
			test = ray.IntersectBox(testUnit->GetBoundParent(), t);

		if (test && t < tmin)
		{
			if (testUnit->GetChildrenSize() == 0)
			{
				nearestUnit = testUnit;
				tmin = t;
				child = -1;
			}
			else
			for (unsigned int i = 0; i < testUnit->GetChildrenSize(); ++i)
			{
				bool tocheck = true;
				for (unsigned int j = 0; j < testUnit->GetIgnoreChildren().size(); ++j)
					if (testUnit->GetIgnoreChildren()[j] == i) tocheck = false;
				if (!tocheck) continue;

				if (testUnit->IsLiveUnit())
					test = ray.IntersectBox(testUnit->GetBoundChild(i), static_cast<glm::mat3>(testUnit->GetOrient()), t);
				else
					test = ray.IntersectBox(testUnit->GetBoundChild(i), t);

				if (test && t < tmin)
				{
					nearestUnit = testUnit;
					tmin = t;
					child = static_cast<int>(i);
				}
			}
		}
		
	}
	return nearestUnit;
}