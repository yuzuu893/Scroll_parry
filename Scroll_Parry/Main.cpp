# include <Siv3D.hpp>

void Main()
{
	FontAsset::Register(U"GameFont", 30);

	// プレイヤー
	RectF player{ 100, 400, 40, 60 };

	Vec2 velocity{ 0, 0 };

	constexpr double Gravity = 0.6;
	constexpr double JumpPower = -12.0;
	constexpr double GroundY = 460;

	bool onGround = true;


	struct Obstacle
	{
		Rect rect;
	};

	struct Enemy
	{
		Rect rect;
		bool alive = true;

		bool touching = false;
		double touchTime = 0.0;
	};

	struct Hole
	{
		Rect rect;
	};

	Array<Enemy> enemies;
	Array<Hole> holes;
	Array<Obstacle> obstacles;

	int killEnemys = 0;
	double survivalTime = 0.0;

	// 生成タイマー
	double spawnTimer = 0.0;
	constexpr double SpawnInterval = 1.5;

	bool gameOver = false;
	bool started = false;
	bool overHole = false;
	bool falling = false;

	while (System::Update())
	{
		//スタート処理
		if (!started)
		{
			if (KeyEnter.down())
			{
				started = true;
			}

			// タイトル
			FontAsset(U"GameFont")(U"「スクロールパリィ」").drawAt(Scene::Center().movedBy(0, -120), Palette::White);

			// 操作方法
			FontAsset(U"GameFont")(U"操作方法").drawAt(Scene::Center().movedBy(0, -40), Palette::White);

			FontAsset(U"GameFont")(U"[SPACE] : ジャンプ").drawAt(Scene::Center().movedBy(0, 10), Palette::White);

			FontAsset(U"GameFont")(U"[ENTER] : パリィ").drawAt(Scene::Center().movedBy(0, 50), Palette::White);

			// 開始案内
			FontAsset(U"GameFont")(U"[ENTER]でスタート").drawAt(Scene::Center().movedBy(0, 120), Palette::Yellow);

			continue;
		}

		//スコア関係
		int score = static_cast<int>(survivalTime) + killEnemys * 10;

		double scrollSpeed = falling ? 0.3 : 6.0 + (score / 50) * 0.5;
		double currentSpawnInterval = Max(0.5, 1.5 - survivalTime * 0.01);

		if (started && !gameOver)
		{
			// プレイヤー操作
			if (KeySpace.down() && onGround)
			{
				velocity.y = JumpPower;
				onGround = false;
			}

			// 重力
			velocity.y += Gravity;
			player.y += velocity.y;

			for (const auto& hole : holes)
			{
				if (player.rightX() > hole.rect.x && player.x < hole.rect.rightX() && player.bottomY() >= GroundY)
				{
					overHole = true;
					break;
				}
			}

			if (overHole && player.y >= GroundY - player.h)
			{
				falling = true;
			}

			if (!overHole && player.y >= GroundY - player.h)
			{
				player.y = GroundY - player.h;
				velocity.y = 0;
				onGround = true;
			}

			if (player.y > Scene::Height())
			{
				gameOver = true;
			}

			// 障害物生成
			spawnTimer += Scene::DeltaTime();
			survivalTime += Scene::DeltaTime();

			if (spawnTimer >= currentSpawnInterval)
			{
				spawnTimer = 0.0;

				// 70%
				if (RandomBool(0.7))
				{
					const int height = Random(40, 85);

					obstacles << Obstacle{
						Rect
						{
							Scene::Width(),static_cast<int>(GroundY - height),40,height
						}
					};
				}
				// 20%
				else if (RandomBool(0.67))
				{
					enemies << Enemy{
						Rect
						{
							Scene::Width(),static_cast<int>(GroundY - 60),40,60
						}
					};
				}
				// 10%
				else
				{
					holes << Hole{
						Rect
						{
							Scene::Width(),static_cast<int>(GroundY),120,100
						}
					};
				}
			}

			// 障害物関係
			for (auto& obstacle : obstacles)
			{
				obstacle.rect.x -= scrollSpeed;
			}

			obstacles.remove_if([](const Obstacle& o)
			{
				return o.rect.rightX() < 0;
			});

			for (auto& enemy : enemies)
			{
				enemy.rect.x -= scrollSpeed;
			}

			for (auto& hole : holes)
			{
				hole.rect.x -= scrollSpeed;
			}

			enemies.remove_if([](const Enemy& e)
			{
				return e.rect.rightX() < 0 || !e.alive;
			});

			// 当たり判定

			// 障害物
			for (const auto& obstacle : obstacles)
			{
				if (player.intersects(obstacle.rect))
				{
					gameOver = true;
					break;
				}
			}

			//敵処理
			constexpr double ParryWindow = 0.08; //パリィ受付時間

			for (auto& enemy : enemies)
			{
				if (!enemy.alive)
				{
					continue;
				}

				Rect parryArea =
				{
					enemy.rect.x - 20,
					enemy.rect.y - 20,
					enemy.rect.w + 40,
					enemy.rect.h + 40
				};
				if (parryArea.intersects(player))
				{
					if (KeyEnter.down())
					{
						enemy.alive = false;
						killEnemys++;
					}
				}

				if (player.intersects(enemy.rect))
				{
					if (!enemy.touching)
					{
						enemy.touching = true;
						enemy.touchTime = 0.0;
					}

					enemy.touchTime += Scene::DeltaTime();

					//パリィ時間内にEnter
					if (KeyEnter.down() && enemy.touchTime <= ParryWindow)
					{
						enemy.alive = false;
						killEnemys++;
						continue;
					}

					//パリィ時間を過ぎたらゲームオーバー
					if (enemy.touchTime > ParryWindow)
					{
						gameOver = true;
						break;
					}
				}
				else
				{
					enemy.touching = false;
					enemy.touchTime = 0.0;
				}
			}
		}

		// 描画関係
		// 地面
		Rect
		{ 0,static_cast<int>(GroundY),Scene::Width(),Scene::Height() - static_cast<int>(GroundY) }.draw(Palette::Green);

		double currentX = 0;

		for (const auto& hole : holes)
		{
			RectF
			{ currentX,GroundY,hole.rect.x - currentX,Scene::Height() - GroundY }.draw(Palette::Green);

			currentX = hole.rect.rightX();
		}

		RectF
		{ currentX,GroundY,Scene::Width() - currentX,Scene::Height() - GroundY }.draw(Palette::Green);

		// プレイヤー
		player.draw(Palette::Orange);

		// 敵
		for (const auto& enemy : enemies)
		{
			enemy.rect.draw(Palette::Blue);
		}

		// 障害物
		for (const auto& obstacle : obstacles)
		{
			obstacle.rect.draw(Palette::Red);
		}

		// 穴
		for (const auto& hole : holes)
		{
			hole.rect.draw(ColorF{ 0.7, 0.9, 1.0 });
			hole.rect.drawFrame(2, Palette::Black);
		}

		//スコア系
		if (!gameOver)
		{
			FontAsset(U"GameFont")(U"Score:{}"_fmt(score)).draw(20, 20, Palette::White);
			FontAsset(U"GameFont")(U"Kills:{}"_fmt(killEnemys)).draw(20, 60, Palette::White);
			FontAsset(U"GameFont")(U"Time:{}"_fmt(static_cast<int>(survivalTime))).draw(20, 100, Palette::White);
			FontAsset(U"GameFont")(U"Speed:{:.1f}"_fmt(scrollSpeed)).draw(20, 140, Palette::White);
		}

		//ゲームオーバー
		if (gameOver)
		{
			FontAsset(U"GameFont")(U"GAME OVER").drawAt(Scene::Center().movedBy(0, -80), Palette::White);

			FontAsset(U"GameFont")(U"Score:{}"_fmt(score)).drawAt(Scene::Center().movedBy(0, -20), Palette::White);

			FontAsset(U"GameFont")(U"Kills:{}"_fmt(killEnemys)).drawAt(Scene::Center().movedBy(0, 20), Palette::White);

			FontAsset(U"GameFont")(U"Time:{}"_fmt(static_cast<int>(survivalTime))).drawAt(Scene::Center().movedBy(0, 60), Palette::White);

			FontAsset(U"GameFont")(U"PRESS R TO RESTART").drawAt(Scene::Center().movedBy(0, 120), Palette::Yellow);

			if (KeyR.down())
			{
				// 初期化
				player = RectF{ 100,400,40,60 };
				velocity = Vec2{ 0,0 };

				enemies.clear();
				holes.clear();
				obstacles.clear();

				killEnemys = 0;
				survivalTime = 0.0;
				spawnTimer = 0.0;

				gameOver = false;
				falling = false;
			}
		}
	}
}
